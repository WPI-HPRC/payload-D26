#include "../State.h"
#include "../multi-state-utils/AntennaConnector/AntennaConnectorInterface.h"
#include "../multi-state-utils/AntennaConnector/AntennaSerialTransmitter.h"
#include "../multi-state-utils/ScrewDrive/ScrewDriveInterface.h"
#include "../multi-state-utils/ImageTransfers/OpenMVReceiver.h"

extern AntennaConnectorInterface antennaConnector;
extern AntennaSerialTransmitter antennaSerialTransmitter;
extern ScrewDriveInterface screwDrive;
extern OpenMVReceiver openMVReceiver;
extern HardwareSerial CAMERA_SERIAL;

static constexpr bool ENABLE_ROV_IMAGE_TRANSMISSION = true;
static constexpr uint32_t DRIVE_DEBUG_INTERVAL_MS = 250;
static constexpr bool ENABLE_ROV_DEBUG = false;


/// debug that only prints if ENABLE_ROV_DEBUG is true
void printDriveDebugJson(bool armed, const AntennaConnectorInterface::DriveData& driveData) {
    if (!ENABLE_ROV_DEBUG) {
        return;
    }

    Serial.print("{\"type\":\"debug\",\"source\":\"rovDrive\",\"armed\":");
    Serial.print(armed ? "true" : "false");
    Serial.print(",\"speed\":");
    Serial.print(driveData.speed, 3);
    Serial.print(",\"turn\":");
    Serial.print(driveData.turn, 3);
    Serial.print(",\"leftEffort\":");
    Serial.print(screwDrive.getLastLeftEffort(), 3);
    Serial.print(",\"rightEffort\":");
    Serial.print(screwDrive.getLastRightEffort(), 3);
    Serial.print(",\"leftPulseUs\":");
    Serial.print(screwDrive.getLastLeftPulseUs());
    Serial.print(",\"rightPulseUs\":");
    Serial.print(screwDrive.getLastRightPulseUs());
    Serial.println("}");
}


/// handles serial input via USB for testing, and local connection to the rover (do not use for antenna-bound communication)
void handleRovSerialInput() {
    if(!Serial.available()) {
        return;
    }

    String input = Serial.readStringUntil('\n');
    input.trim();

    if(input.length() == 0) {
        return;
    }

    if(input.startsWith("{")) {
        bool handled = antennaSerialTransmitter.handleIncomingPacket(input);
        (void)handled;
        return;
    }

    if(input.startsWith("simulate_connection")) {
        uint32_t duration = input.substring(input.indexOf(' ') + 1).toInt();
        antennaConnector.simulateConnectionFor(duration);

        if (ENABLE_ROV_DEBUG) {
            Serial.print("{\"type\":\"debug\",\"source\":\"rovCommand\",\"command\":\"simulate_connection\",\"durationMs\":");
            Serial.print(duration);
            Serial.println("}");
        }
        return;
    }

    if(input.startsWith("drive ")) {
        int firstSpace = input.indexOf(' ');
        int secondSpace = input.indexOf(' ', firstSpace + 1);

        if(secondSpace > firstSpace) {
            float speed = input.substring(firstSpace + 1, secondSpace).toFloat();
            float turn = input.substring(secondSpace + 1).toFloat();
            antennaConnector.setDriveData(speed, turn);

            if (ENABLE_ROV_DEBUG) {
                Serial.print("{\"type\":\"debug\",\"source\":\"manualDrive\",\"speed\":");
                Serial.print(speed, 3);
                Serial.print(",\"turn\":");
                Serial.print(turn, 3);
                Serial.println("}");
            }
        } else {
            if (ENABLE_ROV_DEBUG) {
                Serial.println("{\"type\":\"debug\",\"source\":\"manualDrive\",\"error\":\"invalid_format\"}");
            }
        }

        return;
    }

    if(input.startsWith("sensor ")) {
        int firstSpace = input.indexOf(' ');
        int secondSpace = input.indexOf(' ', firstSpace + 1);

        if(secondSpace > firstSpace) {
            String parameterName = input.substring(firstSpace + 1, secondSpace);
            float value = input.substring(secondSpace + 1).toFloat();
            bool ok = antennaConnector.setSensorValue(parameterName, value);

            if (ENABLE_ROV_DEBUG) {
                Serial.print("{\"type\":\"debug\",\"source\":\"sensorCommand\",\"ok\":");
                Serial.print(ok ? "true" : "false");
                Serial.print(",\"name\":\"");
                Serial.print(parameterName);
                Serial.print("\",\"value\":");
                Serial.print(value, 3);
                Serial.println("}");
            }
        } else {
            if (ENABLE_ROV_DEBUG) {
                Serial.println("{\"type\":\"debug\",\"source\":\"sensorCommand\",\"error\":\"invalid_format\"}");
            }
        }

        return;
    }

    if(input == "sensor_json") {
        Serial.println(antennaConnector.getAllSensorDataJson());
        return;
    }

    if(input.startsWith("sensor_get ")) {
        String parameterName = input.substring(input.indexOf(' ') + 1);
        float value = 0.0f;
        bool ok = antennaConnector.getSensorValue(parameterName, value);

        if (ENABLE_ROV_DEBUG) {
            Serial.print("{\"type\":\"debug\",\"source\":\"sensorGet\",\"ok\":");
            Serial.print(ok ? "true" : "false");
            Serial.print(",\"name\":\"");
            Serial.print(parameterName);
            Serial.print("\",\"value\":");
            Serial.print(value, 3);
            Serial.println("}");
        }
    }
}


void handleConnectionLost() {
    if (ENABLE_ROV_DEBUG) {
        Serial.println("{\"type\":\"debug\",\"source\":\"rovState\",\"event\":\"connectionLost\"}");
    }
}

/**
 * grabs drive commands from the antenna connector and sends them to the screw drive and handles logging if enabled.
 */
void driveBehavior() {
    AntennaConnectorInterface::DriveData driveData = antennaConnector.getDriveData();
    bool armed = screwDrive.updateArm();

    if (armed) {
        screwDrive.drive(driveData.speed, driveData.turn);
    }

    static uint32_t lastDriveDebugAt = 0;
    static float lastSpeed = 999.0f;
    static float lastTurn = 999.0f;
    static uint16_t lastLeftPulse = 0;
    static uint16_t lastRightPulse = 0;

    uint32_t now = millis();
    bool commandChanged = abs(driveData.speed - lastSpeed) > 0.001f ||
                          abs(driveData.turn - lastTurn) > 0.001f;
    bool pulseChanged = screwDrive.getLastLeftPulseUs() != lastLeftPulse ||
                        screwDrive.getLastRightPulseUs() != lastRightPulse;

    if (commandChanged || pulseChanged || now - lastDriveDebugAt >= DRIVE_DEBUG_INTERVAL_MS) {
        printDriveDebugJson(armed, driveData);
        lastDriveDebugAt = now;
        lastSpeed = driveData.speed;
        lastTurn = driveData.turn;
        lastLeftPulse = screwDrive.getLastLeftPulseUs();
        lastRightPulse = screwDrive.getLastRightPulseUs();
    }
}

void payloadROVInit(StateData *data) {
    if (ENABLE_ROV_DEBUG) {
        Serial.println("{\"type\":\"debug\",\"source\":\"rovState\",\"event\":\"entered\"}");
    }

    // attach the screw drive and start the arming process immediately so it's ready to go by the time we get drive commands
    screwDrive.attach(LEFT_SCREW_PWM, RIGHT_SCREW_PWM);
    screwDrive.beginArm();

    // set up the antenna serial transmitter and OpenMV receiver input stream
    CAMERA_SERIAL.begin(115200);
    openMVReceiver.setInputStream(&CAMERA_SERIAL);

    // for local connection (no antenna) setup the serial antenna/debug transmitter
    antennaSerialTransmitter.setOutputStream(&Serial);
    antennaSerialTransmitter.setAntennaConnector(&antennaConnector);

    // antenna setup can go here
}

StateID payloadROVLoop(StateData *data, Context *ctx) {

    // local serial input handling for testing and local control (can run in parallel with antenna commands, but intended for use when not connected to the antenna)
    handleRovSerialInput();

    if (ENABLE_ROV_IMAGE_TRANSMISSION) {
        String imageData = "";
        int byteCount = 0;


        // handle image receive from openMV over serial and publish to antenna connector if we're ready to accept images (not currently in the middle of a transmission)
        if (openMVReceiver.runReceiver() && antennaConnector.canAcceptImages()) {
            if (openMVReceiver.getImage(imageData, byteCount)) {
                antennaConnector.intakeImageData(imageData, byteCount);
            }
        }

        // run the local connection serial transmiter (handles sending debug and telemetry to the PC and receiving commands from the PC, does not handle antenna communication)
        antennaSerialTransmitter.runTransmitter();
    }

    // handle drive commands and logging
    driveBehavior();

    // check for connection loss and transition to autonomous state if connection is lost
    if (antennaConnector.onConnectionLost()) {
        handleConnectionLost();
        return PAYLOAD_AUTONOMOUS;
    }

    return PAYLOAD_ROV;
}

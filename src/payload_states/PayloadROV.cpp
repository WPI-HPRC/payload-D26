#include "../State.h"
#include "../multi-state-utils/AntennaConnector/AntennaConnectorInterface.h"
#include "../multi-state-utils/AntennaConnector/AntennaSerialTransmitter.h"
#include "../multi-state-utils/ScrewDrive/ScrewDriveInterface.h"
#include "../multi-state-utils/ImageTransfers/OpenMVReceiver.h"
#include <SoftwareSerial.h>

extern AntennaConnectorInterface antennaConnector;
extern AntennaSerialTransmitter antennaSerialTransmitter;
extern ScrewDriveInterface screwDrive;
extern OpenMVReceiver openMVReceiver;
extern HardwareSerial CAMERA_SERIAL; // not available on real wiring layout
extern SoftwareSerial SOFT_CAM_SERIAL;

static constexpr bool ENABLE_OPENMV_RAW_MONITOR = true;
static constexpr bool ENABLE_OPENMV_RAW_MONITOR_STATUS = true;
static constexpr bool ENABLE_OPENMV_RX_PIN_MONITOR = true;
static constexpr bool ENABLE_OPENMV_IMAGE_RECEIVER = false;
static constexpr bool ENABLE_ROV_TELEMETRY_TRANSMISSION = false;
static constexpr uint32_t OPENMV_RAW_STATUS_INTERVAL_MS = 1000;
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
 * Mirrors every byte received from the OpenMV camera stream to USB Serial.
 *
 * This is a diagnostic path for camera serial bring-up. It intentionally does
 * not parse image framing or emit rover JSON so the PC sees the raw OpenMV
 * output with minimal firmware-added noise.
 */
void handleOpenMVRawMonitor() {
    static uint32_t lastStatusAt = 0;
    static uint32_t totalBytesSeen = 0;
    static uint32_t totalBytesAtLastStatus = 0;
    static bool rxMonitorInitialized = false;
    static int lastRxLevel = HIGH;
    static uint32_t rxTransitionsSeen = 0;
    static uint32_t rxTransitionsAtLastStatus = 0;
    uint32_t bytesThisLoop = 0;

    if (ENABLE_OPENMV_RX_PIN_MONITOR) {
        int rxLevel = digitalRead(CAMERA_SERIAL_RX);

        if (!rxMonitorInitialized) {
            lastRxLevel = rxLevel;
            rxMonitorInitialized = true;
        } else if (rxLevel != lastRxLevel) {
            rxTransitionsSeen++;
            lastRxLevel = rxLevel;
        }
    }

    while (SOFT_CAM_SERIAL.available() > 0) {
        Serial.write(static_cast<uint8_t>(SOFT_CAM_SERIAL.read()));
        totalBytesSeen++;
        bytesThisLoop++;
    }

    if (!ENABLE_OPENMV_RAW_MONITOR_STATUS) {
        return;
    }

    uint32_t now = millis();
    if (now - lastStatusAt < OPENMV_RAW_STATUS_INTERVAL_MS) {
        return;
    }

    if (totalBytesSeen == totalBytesAtLastStatus) {
        Serial.print("DBG_OPENMV_RAW_NO_BYTES rx_level=");
        Serial.print(digitalRead(CAMERA_SERIAL_RX));
        Serial.print(" rx_transitions_last_interval=");
        Serial.println(rxTransitionsSeen - rxTransitionsAtLastStatus);
    } else {
        Serial.print("DBG_OPENMV_RAW_BYTES total=");
        Serial.print(totalBytesSeen);
        Serial.print(" last_interval=");
        Serial.print(totalBytesSeen - totalBytesAtLastStatus);
        Serial.print(" rx_transitions_last_interval=");
        Serial.println(rxTransitionsSeen - rxTransitionsAtLastStatus);
    }

    (void)bytesThisLoop;
    totalBytesAtLastStatus = totalBytesSeen;
    rxTransitionsAtLastStatus = rxTransitionsSeen;
    lastStatusAt = now;
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
    //CAMERA_SERIAL.begin(115200);
    pinMode(CAMERA_SERIAL_RX, INPUT_PULLUP);
    SOFT_CAM_SERIAL.begin(57600);
    SOFT_CAM_SERIAL.listen();
    openMVReceiver.setInputStream(&SOFT_CAM_SERIAL);

    if (ENABLE_OPENMV_RAW_MONITOR && ENABLE_OPENMV_RAW_MONITOR_STATUS) {
        Serial.print("DBG_OPENMV_RAW_MONITOR_READY baud=57600 rx_pin=");
        Serial.print(CAMERA_SERIAL_RX);
        Serial.print(" tx_pin=");
        Serial.print(CAMERA_SERIAL_TX);
        Serial.print(" initial_rx_level=");
        Serial.println(digitalRead(CAMERA_SERIAL_RX));
    }

    // for local connection (no antenna) setup the serial antenna/debug transmitter
    antennaSerialTransmitter.setOutputStream(&Serial);
    antennaSerialTransmitter.setAntennaConnector(&antennaConnector);

    // antenna setup can go here
}

StateID payloadROVLoop(StateData *data, Context *ctx) {

    // local serial input handling for testing and local control (can run in parallel with antenna commands, but intended for use when not connected to the antenna)
    handleRovSerialInput();

    if (ENABLE_OPENMV_RAW_MONITOR) {
        handleOpenMVRawMonitor();
    } else if (ENABLE_OPENMV_IMAGE_RECEIVER) {
        String imageData = "";
        int byteCount = 0;


        // handle image receive from openMV over serial and publish to antenna connector if we're ready to accept images (not currently in the middle of a transmission)
        if (openMVReceiver.runReceiver() && antennaConnector.canAcceptImages()) {
            if (openMVReceiver.getImage(imageData, byteCount)) {
                antennaConnector.intakeImageData(imageData, byteCount);
            }
        }

    }

    if (ENABLE_ROV_TELEMETRY_TRANSMISSION) {
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

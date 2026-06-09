#include "../State.h"
#include "../multi-state-utils/AntennaConnector/AntennaConnectorInterface.h"
#include "../multi-state-utils/ScrewDrive/ScrewDriveInterface.h"


extern AntennaConnectorInterface antennaConnector; // Create an instance of the antenna connector interface
extern ScrewDriveInterface screwDrive;


void handleConnectionLost() {
    // Placeholder for actions to take once connection is lost
    // This might involve activating certain hardware or sending a message
    Serial.println("Connection lost! Performing post-connection lost actions...");
    Serial.println("Exiting Payload ROV State...");
}

void driveBehavior() {
    AntennaConnectorInterface::DriveData driveData = antennaConnector.getDriveData();

    if (screwDrive.updateArm()) {
        screwDrive.drive(driveData.speed, driveData.turn);
    }
}

void getAmperageInfo() {
    // TODO: update antennaConnector sensor values from real current and voltage sensors.
}

/**
 * Nic - this simulates antenna behavior grabbing images for processing and transmission
 */
void passImages() {
    // Placeholder for actions to take to pass images from the ROV
    // This might involve activating certain hardware or sending a message
}


void payloadROVInit(StateData *data) {
    Serial.println("Entered Payload ROV State...");
    screwDrive.attach(LEFT_SCREW_PWM, RIGHT_SCREW_PWM);
    screwDrive.beginArm();
}

StateID payloadROVLoop(StateData *data, Context *ctx) {

    String input = "";
    
    if(Serial.available()) {
        input = Serial.readStringUntil('\n');
        input.trim();
    }

    if(input.startsWith("simulate_connection")) {

        uint32_t duration = input.substring(input.indexOf(' ') + 1).toInt(); // Get the duration from the command

        if(duration > 0) {
            Serial.print("Simulating connection for ");
            Serial.print(duration);
            Serial.println(" ms...");
        } else {
            Serial.println("Invalid duration for simulate_connection command. Please provide a positive integer value in milliseconds.");
        }

        antennaConnector.simulateConnectionFor(duration); // Simulate a connection for the specified duration
    }

    if(input.startsWith("drive ")) {
        int firstSpace = input.indexOf(' ');
        int secondSpace = input.indexOf(' ', firstSpace + 1);

        if(secondSpace > firstSpace) {
            float speed = input.substring(firstSpace + 1, secondSpace).toFloat();
            float turn = input.substring(secondSpace + 1).toFloat();
            antennaConnector.setDriveData(speed, turn);

            Serial.print("Drive data set speed=");
            Serial.print(speed, 3);
            Serial.print(" turn=");
            Serial.println(turn, 3);
        } else {
            Serial.println("Invalid drive command. Use: drive <speed> <turn>");
        }
    }

    if(input.startsWith("sensor ")) {
        int firstSpace = input.indexOf(' ');
        int secondSpace = input.indexOf(' ', firstSpace + 1);

        if(secondSpace > firstSpace) {
            String parameterName = input.substring(firstSpace + 1, secondSpace);
            float value = input.substring(secondSpace + 1).toFloat();

            if(antennaConnector.setSensorValue(parameterName, value)) {
                Serial.print("Sensor value set ");
                Serial.print(parameterName);
                Serial.print("=");
                Serial.println(value, 3);
            } else {
                Serial.print("Unknown sensor parameter: ");
                Serial.println(parameterName);
            }
        } else {
            Serial.println("Invalid sensor command. Use: sensor <name> <value>");
        }
    }

    if(input == "sensor_json") {
        Serial.println(antennaConnector.getAllSensorDataJson());
    }

    if(input.startsWith("sensor_get ")) {
        String parameterName = input.substring(input.indexOf(' ') + 1);
        float value = 0.0f;

        if(antennaConnector.getSensorValue(parameterName, value)) {
            Serial.print(parameterName);
            Serial.print("=");
            Serial.println(value, 3);
        } else {
            Serial.print("Unknown sensor parameter: ");
            Serial.println(parameterName);
        }
    }

    driveBehavior();
    getAmperageInfo();
    passImages();

    if (antennaConnector.onConnectionLost()) {
        handleConnectionLost();
        return PAYLOAD_AUTONOMOUS;
    }

    return PAYLOAD_ROV;
}

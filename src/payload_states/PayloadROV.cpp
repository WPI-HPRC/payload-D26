#include "../State.h"
#include "../multi-state-utils/AntennaConnector/AntennaConnectorInterface.h"


extern AntennaConnectorInterface antennaConnector; // Create an instance of the antenna connector interface


bool onConnectionLost() {
    // Placeholder for actual connection lost logic
    // In a real implementation, this might check sensor data or a timer
    return false; // Change to true to simulate connection being lost
}

void handleConnectionLost() {
    // Placeholder for actions to take once connection is lost
    // This might involve activating certain hardware or sending a message
    Serial.println("Connection lost! Performing post-connection lost actions...");
    Serial.println("Exiting Payload ROV State...");
}

void driveBehavior() {
    // Placeholder for actions to take to drive the ROV
    // This might involve activating certain hardware or sending a message
}

void getAmperageInfo() {
    // Placeholder for actions to take to get amperage info from the ROV
    // This might involve activating certain hardware or sending a message
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

    driveBehavior();
    getAmperageInfo();
    passImages();

    if (antennaConnector.onConnectionLost()) {
        handleConnectionLost();
        return PAYLOAD_AUTONOMOUS;
    }

    return PAYLOAD_ROV;
}

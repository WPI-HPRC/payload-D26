#include "../State.h"
#include "../multi-state-utils/AntennaConnector/AntennaConnectorInterface.h"


void handleAntennaConnectionConfirmed() {
    // Placeholder for actions to take once connection is confirmed
    // This might involve activating certain hardware or sending a message
    Serial.println("Connection confirmed! Performing post-connection actions...");
    Serial.println("Exiting Payload Autonomous State...");
}

void autonomousBehavior() {
    // Placeholder for the autonomous behavior of the payload
    // This might involve activating certain hardware or performing specific tasks
}

extern AntennaConnectorInterface antennaConnector; // Create an instance of the antenna connector interface

void payloadAutonomousInit(StateData *data) {
    Serial.println("Entered Payload Autonomous State...");
    Serial.println("Performing autonomous actions...");
}

StateID payloadAutonomousLoop(StateData *data, Context *ctx) {

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

   
    if(antennaConnector.onConnectionGained()) {
        handleAntennaConnectionConfirmed();
        return PAYLOAD_ROV;
    }

    autonomousBehavior();


    return PAYLOAD_AUTONOMOUS;
}

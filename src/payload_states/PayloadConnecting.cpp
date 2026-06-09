#include "../State.h"
#include "../multi-state-utils/AntennaConnector/AntennaConnectorInterface.h"
#include "../multi-state-utils/AntennaConnector/AntennaSerialTransmitter.h"

void handleAntennaConnectionEstablished() {
    // Placeholder for actions to take once connection is established
    // This might involve activating certain hardware or sending a message
    Serial.println("Connection established! Performing post-connection actions...");
    Serial.println("Exiting Payload Connecting State...");
}

bool checkConnectionTimeout(StateData *data, uint32_t timeoutDuration) {

    // check if no connection has been established within the timeout duration
    if(data->currentTime > timeoutDuration) {
        return true;
    }
    return false; // Change to true to simulate connection timeout
}

void handleConnectionTimeout() {
    // Placeholder for actions to take once connection times out
    // This might involve activating certain hardware or sending a message
    Serial.println("Connection timed out! Performing timeout actions...");
    Serial.println("Exiting Payload Connecting State...");
}

extern AntennaConnectorInterface antennaConnector; // Create an instance of the antenna connector interface
extern AntennaSerialTransmitter antennaSerialTransmitter;
uint32_t timeoutDuration = 8000; // Set a timeout duration (e.g., 10 seconds)


void payloadConnectingInit(StateData *data) {
    Serial.println("Entered Payload Connecting State...");
    Serial.println("Waiting for connection to be established...");
    antennaSerialTransmitter.setOutputStream(&Serial);

}

StateID payloadConnectingLoop(StateData *data, Context *ctx) {

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

   
    antennaSerialTransmitter.updateConnectionStatus();

    if(antennaConnector.onConnectionGained()) {
        handleAntennaConnectionEstablished();
        return PAYLOAD_ROV;
    }

    if (checkConnectionTimeout(data, timeoutDuration)) {
        handleConnectionTimeout();
        return PAYLOAD_AUTONOMOUS; // Transition to autonomous mode if connection fails
    }

    return PAYLOAD_CONNECTING;
}

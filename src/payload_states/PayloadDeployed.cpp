#include "../State.h"


void raiseAntenna() {
    // Placeholder for actions to take to raise the antenna
    // This might involve activating certain hardware or sending a message
}

bool checkAntennaRaised(String input) {
    // Placeholder for actual antenna raised logic
    // In a real implementation, this might check sensor data or a timer

    if(input == "antenna_raised") {
        return true;
    }

    return false; // Change to true to simulate antenna being raised
}

void handleAntennaRaised() {
    // Placeholder for actions to take once the antenna is raised
    // This might involve activating certain hardware or sending a message
    Serial.println("Antenna raised successfully!");
    Serial.println("Exiting Payload Deployed State...");
}

void payloadDeployedInit(StateData *data) {

    Serial.println("Entered Payload Deployed State...");
}

StateID payloadDeployedLoop(StateData *data, Context *ctx) {

    String input = "";

    if(Serial.available()) {
        input = Serial.readStringUntil('\n');
        input.trim();
    }

    if (checkAntennaRaised(input)) {
        handleAntennaRaised();
        return PAYLOAD_CONNECTING;
    } else {
        raiseAntenna();
    }

    input = ""; // reset input string for next loop iteration
    return PAYLOAD_DEPLOYED;
}

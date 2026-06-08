#include "../State.h"

bool checkLatchReleaseComplete() {
    // Placeholder for actual latch release completion logic
    // In a real implementation, this might check sensor data or a timer

    if(Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if(input == "latch_release_complete") {
            return true;
        }
    }
    return false; // Change to true to simulate latch release completion
}

void handleLatchReleaseComplete() {
    // Placeholder for actions to take once latch release is complete
    // This might involve activating certain hardware or sending a message
    Serial.println("Latch release complete! Performing post-release actions...");
    Serial.println("Exiting Payload Latch Releasing State...");
}

void payloadLatchReleasingInit(StateData *data) {

    Serial.println("Entered Payload Latch Releasing State...");
}

StateID payloadLatchReleasingLoop(StateData *data, Context *ctx) {
    if (checkLatchReleaseComplete()) {
        handleLatchReleaseComplete();

        return PAYLOAD_DEPLOYING; // Transition to the next state (deploying)
    }
    return PAYLOAD_LATCH_RELEASING;
}

#include "../State.h"

void payloadSelfRightingInit(StateData *data) {

    Serial.println("Entered Payload Self-Righting State...");
}

StateID payloadSelfRightingLoop(StateData *data, Context *ctx) {

    if(data->currentTime > 1000) { // after 1 second, transition to the next state (deploying)

        // Just for the purposes of simulating the self-righting process
        Serial.println("Exiting Payload Self-Righting State...");
        return PAYLOAD_LATCH_RELEASING;
    }

    return PAYLOAD_SELF_RIGHTING;
}

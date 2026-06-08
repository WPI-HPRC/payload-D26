#include "../State.h"

void payloadIdleInit(StateData *data) {
    Serial.println("Entered Payload Idle State...");
}

StateID payloadIdleLoop(StateData *data, Context *ctx) {

    // for now
    Serial.println("In Payload Idle State... Transitioning to self-righting");
    
    return PAYLOAD_SELF_RIGHTING; // Transition to the next state (self-righting)
}

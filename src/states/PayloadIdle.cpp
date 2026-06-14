#include "../State.h"

void *payloadIdleInit(StateData const *data) {
    Serial.println("Entered Payload Idle State...");
    return nullptr;
}

StateID payloadIdleLoop(StateData const *data, Context *ctx, void *_localData) {

    // for now
    Serial.println("In Payload Idle State... Transitioning to self-righting");
    
    return PAYLOAD_SELF_RIGHTING; // Transition to the next state (self-righting)
}

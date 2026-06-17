#include "../State.h"

void *payloadIdleInit(StateData const *data) {
    Serial.println("Entered Payload Idle State...");
    return nullptr;
}

StateID payloadIdleLoop(StateData const *data, Context *ctx, void *_localData) {

    // for now
    Serial.println("In Payload Idle State... Transitioning to RRocket Timer in 5 seconds");

    if (data->currentTime > 5000) {
        return ROCKET_TIMER; // Transition to the next state (Rocket Timer)
    }
    
    return ROCKET_TIMER; // Transition to the next state (Rocket Timer)
}

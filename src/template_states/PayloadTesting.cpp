#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"

void payloadTestingInit(StateData *data) {}

StateID payloadTestingLoop (StateData* data, Context* ctx) {
    Serial.println("TESTING PAYLOAD");
    digitalWrite(LED_BLUE, true);
    return PAYLOAD_TESTING;
}

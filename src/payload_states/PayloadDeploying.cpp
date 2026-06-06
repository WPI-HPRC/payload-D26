#include "../State.h"

void payloadDeployingInit(StateData *data) {}

StateID payloadDeployingLoop(StateData *data, Context *ctx) {
    return PAYLOAD_DEPLOYING;
}

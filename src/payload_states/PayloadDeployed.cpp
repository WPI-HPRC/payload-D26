#include "../State.h"

void payloadDeployedInit(StateData *data) {}

StateID payloadDeployedLoop(StateData *data, Context *ctx) {
    return PAYLOAD_DEPLOYED;
}

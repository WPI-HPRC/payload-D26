#include "../State.h"

void *conventionDemoInit(StateData const *data) { return nullptr; }

StateID conventionDemoLoop(StateData const *data, Context *ctx, void *_localData) {
    return CONVENTION_DEMO;
}

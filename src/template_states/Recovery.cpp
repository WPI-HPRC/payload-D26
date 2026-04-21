#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"

void recoveryInit(StateData *data) {}

StateID recoveryLoop (StateData* data, Context* ctx) {
    return RECOVERY; 
}

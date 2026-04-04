#include "../State.h"
#include "StateMachineConstants.h"

void drogueDescentInit(StateData *data) {}

StateID drogueDescentLoop (StateData* data, Context* ctx) {

    // under main descent if velocity down is between 16 to 30 fps (abt 5m/s to 9m/s)
    // poll velocity data (velocity down will be positive)
    const auto vel_vec = ctx->estimator.get_vel_prev_ned();

    if(data->accelDebouncer.update(abs(vel_vec(2, 0)) > MAIN_MIN_VEL, millis()) && abs(vel_vec(2, 0)) < MAIN_MAX_VEL, millis()) {
        // check that the velocity is greater than the min velocity (5) for main descent, and
        // less than the max velocity (9) using debouncer
        return MAIN_DESCENT;
    }

    return DROGUE_DESCENT;
}

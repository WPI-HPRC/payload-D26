#include "../State.h"
#include "StateMachineConstants.h"
//#include "../qmekf.h"

void boostInit (StateData* data) {}

StateID boostLoop (StateData* data, Context* ctx) {
    // poll acceleration data from state estimator in ctx
    const auto acc_vec = ctx->estimator.get_accel_prev();

    if (data->accelDebouncer.update(abs(acc_vec(0, 0)) < COAST_THRESHOLD, millis()) ||  data->currentTime > 2000) {
        // check that acceleration up is less than coast threshold (0.3) for long enough using debouncer, or
        // current time > 2000 (maximum boost time)
        return COAST;
    }

    return BOOST;
}

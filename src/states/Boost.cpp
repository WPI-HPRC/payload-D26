#include "../State.h"
#include "StateMachineConstants.h"

void boostInit (StateData* data) {}

StateID boostLoop (StateData* data, Context* ctx) {
    /*
    - Poll acceleration data from ctx
    - Check acceleration to detect coast stage
    - Check if maximum boost time is exceeded
    - Check if need to abort
    - Update sensor data and ctx for next iteration?
    */
    const auto &accel_desc = ctx->accel.get_descriptor();
    if (accel_desc.getLastUpdated() != data->lastAccelReadingTime && data->currentTime > 2000) {
        data->lastAccelReadingTime = accel_desc.getLastUpdated();
        if(data->accelDebouncer.update(accel_desc.data.accel0 < COAST_THRESHOLD , millis())) {
            return COAST;
        }
    }

    return BOOST;
}

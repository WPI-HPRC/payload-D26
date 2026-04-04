#include "../State.h"

void coastInit (StateData* data) { }

/*
- Poll acceleration data from ctx
- Check acceleration to detect drouge deployment
- Check if maximum coast time is exceeded
- Check if need to abort
- Update sensor data and ctx for next iteration?
*/

StateID coastLoop (StateData* data, Context* ctx) {
    static bool airBrakesOut = false;
    static bool airBrakesDone = false;
    static double prevAltitude = 0;

    // poll acceleration data in NED frame
    const auto acc_vec = ctx->estimator.get_vel_prev_ned();
    
    if(acc_vec(2, 0) < 0.2 && acc_vec(2, 0) > -0.2 && airBrakesDone) {
        // check acceleration down in NED frame is between 0.2 and -0.2
        // and airbreaks done
        return DROGUE_DESCENT;
    }

    return COAST;
}
#include "../State.h"

void abortInit (StateData* data) {}

// returns an abort state
// takes in a pointer that holds timing data (defined in State.h) and a pointer that holds log file and sensor data (Context.h)
StateID abortLoop (StateData *data, Context *ctx) {
    return ABORT;
}
#pragma once

#include "Context.h"
#include "debouncer.h"

enum StateID {
    PRELAUNCH,
    BOOST,
    COAST,
    DROGUE_DESCENT,
    MAIN_DESCENT,
    RECOVERY,
    ABORT,
    NUM_STATES
};

struct StateData {
    long long currentTime;
    long long deltaTime;
    long long loopCount;
    long long startTime;
    long long lastLoopTime;
    uint32_t lastAccelReadingTime;
    uint32_t lastBaroReadingTime;
    Debouncer baroDebouncer = Debouncer(20);
    Debouncer accelDebouncer = Debouncer(20);
    Debouncer velDebouncer = Debouncer(20);
};

// ABORT
void abortInit (StateData* data);
StateID abortLoop (StateData *data, Context *ctx);

// BOOST
void boostInit (StateData* data);
StateID boostLoop (StateData* data, Context* ctx);

// COAST
void coastInit (StateData* data);
StateID coastLoop (StateData* data, Context* ctx);

// DROGUE_DESCENT
void drogueDescentInit(StateData *data);
StateID drogueDescentLoop (StateData* data, Context* ctx);

// MAIN_DESCENT
void mainDescentInit(StateData *data);
StateID mainDescentLoop (StateData* data, Context* ctx);

// PRELAUNCH
void prelaunchInit (StateData* data);
StateID prelaunchLoop (StateData* data, Context* ctx);

// RECOVERY
void recoveryInit(StateData *data);
StateID recoveryLoop (StateData* data, Context* ctx);

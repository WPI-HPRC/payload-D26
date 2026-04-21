#pragma once

#include "Context.h"
#include "debouncer.h"

enum StateID {
    FORWARD,
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
    Servo servo1;
    Servo servo2;
    float linear_vel;
    float angular_vel;
};


// FORWARD
void forwardInit(StateData *data);
StateID recoveryLoop (StateData* data, Context* ctx);



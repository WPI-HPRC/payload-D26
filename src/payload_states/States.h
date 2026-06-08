#pragma once

#include "Context.h"
#include "debouncer.h"

enum StateID {
    PAYLOAD_SELF_RIGHTING,
    PAYLOAD_LATCH_RELEASING,
    PAYLOAD_DEPLOYING,
    PAYLOAD_DEPLOYED,
    PAYLOAD_CONNECTING,
    PAYLOAD_ROV,
    PAYLOAD_AUTONOMOUS,
    CONVENTION_DEMO,
    PAYLOAD_IDLE,
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

// PAYLOAD_SELF_RIGHTING
void payloadSelfRightingInit(StateData *data);
StateID payloadSelfRightingLoop(StateData *data, Context *ctx);

// PAYLOAD_LATCH_RELEASING
void payloadLatchReleasingInit(StateData *data);
StateID payloadLatchReleasingLoop(StateData *data, Context *ctx);

// PAYLOAD_DEPLOYING
void payloadDeployingInit(StateData *data);
StateID payloadDeployingLoop(StateData *data, Context *ctx);

// PAYLOAD_DEPLOYED
void payloadDeployedInit(StateData *data);
StateID payloadDeployedLoop(StateData *data, Context *ctx);

// PAYLOAD_CONNECTING
void payloadConnectingInit(StateData *data);
StateID payloadConnectingLoop(StateData *data, Context *ctx);

// PAYLOAD_ROV
void payloadROVInit(StateData *data);
StateID payloadROVLoop(StateData *data, Context *ctx);

// PAYLOAD_AUTONOMOUS
void payloadAutonomousInit(StateData *data);
StateID payloadAutonomousLoop(StateData *data, Context *ctx);

// CONVENTION_DEMO
void conventionDemoInit(StateData *data);
StateID conventionDemoLoop(StateData *data, Context *ctx);

// PAYLOAD_IDLE
void payloadIdleInit(StateData *data);
StateID payloadIdleLoop(StateData *data, Context *ctx);

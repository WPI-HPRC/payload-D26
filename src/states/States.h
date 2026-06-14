#pragma once

#include "States_generated.h"

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

hprc::States stateToTelemState(StateID state);

struct StateData {
    long long currentTime;
    long long deltaTime;
    long long loopCount;
    long long startTime;
    long long lastLoopTime;
};

struct Context;

void initStateData(StateData *data);

void updateStateData(StateData *data);

// PAYLOAD_SELF_RIGHTING
void *payloadSelfRightingInit(StateData const *data);
StateID payloadSelfRightingLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_LATCH_RELEASING
void *payloadLatchReleasingInit(StateData const *data);
StateID payloadLatchReleasingLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_DEPLOYING
void *payloadDeployingInit(StateData const *data);
StateID payloadDeployingLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_DEPLOYED
void *payloadDeployedInit(StateData const *data);
StateID payloadDeployedLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_CONNECTING
void *payloadConnectingInit(StateData const *data);
StateID payloadConnectingLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_ROV
void *payloadROVInit(StateData const *data);
StateID payloadROVLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_AUTONOMOUS
void *payloadAutonomousInit(StateData const *data);
StateID payloadAutonomousLoop(StateData const *data, Context *ctx, void *localData);

// CONVENTION_DEMO
void *conventionDemoInit(StateData const *data);
StateID conventionDemoLoop(StateData const *data, Context *ctx, void *localData);

// PAYLOAD_IDLE
void *payloadIdleInit(StateData const *data);
StateID payloadIdleLoop(StateData const *data, Context *ctx, void *localData);

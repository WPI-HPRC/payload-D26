#include "States.h"
#include "../State.h"

StateInitFunc initFuncs[NUM_STATES] = {};
StateLoopFunc loopFuncs[NUM_STATES] = {};

void initStateMap() {
    initFuncs[PAYLOAD_SELF_RIGHTING] = &payloadSelfRightingInit;
    initFuncs[PAYLOAD_LATCH_RELEASING] = &payloadLatchReleasingInit;
    initFuncs[PAYLOAD_DEPLOYING] = &payloadDeployingInit;
    initFuncs[PAYLOAD_DEPLOYED] = &payloadDeployedInit;
    initFuncs[PAYLOAD_CONNECTING] = &payloadConnectingInit;
    initFuncs[PAYLOAD_ROV] = &payloadROVInit;
    initFuncs[PAYLOAD_AUTONOMOUS] = &payloadAutonomousInit;
    initFuncs[CONVENTION_DEMO] = &conventionDemoInit;
    initFuncs[PAYLOAD_IDLE] = &payloadIdleInit;

    loopFuncs[PAYLOAD_SELF_RIGHTING] = &payloadSelfRightingLoop;
    loopFuncs[PAYLOAD_LATCH_RELEASING] = &payloadLatchReleasingLoop;
    loopFuncs[PAYLOAD_DEPLOYING] = &payloadDeployingLoop;
    loopFuncs[PAYLOAD_DEPLOYED] = &payloadDeployedLoop;
    loopFuncs[PAYLOAD_CONNECTING] = &payloadConnectingLoop;
    loopFuncs[PAYLOAD_ROV] = &payloadROVLoop;
    loopFuncs[PAYLOAD_AUTONOMOUS] = &payloadAutonomousLoop;
    loopFuncs[CONVENTION_DEMO] = &conventionDemoLoop;
    loopFuncs[PAYLOAD_IDLE] = &payloadIdleLoop;
}

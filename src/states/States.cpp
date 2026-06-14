#include "States.h"
#include "../State.h"
#include "States_generated.h"

hprc::States stateToTelemState(StateID state) {
  switch (state) {
  case PAYLOAD_SELF_RIGHTING:
      return hprc::States_SelfRighting;
  case PAYLOAD_LATCH_RELEASING:
      return hprc::States_LatchReleasing;
  case PAYLOAD_DEPLOYING:
      return hprc::States_Deploying;
  case PAYLOAD_DEPLOYED:
      return hprc::States_Deployed;
  case PAYLOAD_CONNECTING:
      return hprc::States_Connecting;
  case PAYLOAD_ROV:
      return hprc::States_ROV;
  case PAYLOAD_AUTONOMOUS:
      return hprc::States_Autonomous;
  case CONVENTION_DEMO:
      return hprc::States_Start;
  case PAYLOAD_IDLE:
      return hprc::States_Idle;
  case NUM_STATES:
      return hprc::States_Start;
  }
  return hprc::States_Start;
}

void initStateData(StateData *data) {
  data->startTime = millis();
  data->currentTime = 0;
  data->deltaTime = 0;
  data->lastLoopTime = 0;
  data->loopCount = 0;
};

void updateStateData(StateData *data) {
  long long now = millis();
  data->currentTime = now - data->startTime;
  data->deltaTime = now - data->lastLoopTime;
  data->lastLoopTime = now;
  data->loopCount++;
}

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

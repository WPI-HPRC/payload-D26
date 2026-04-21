#include "States.h"
#include "../State.h"


StateInitFunc initFuncs[NUM_STATES] = {};
StateLoopFunc loopFuncs[NUM_STATES] = {};

void initStateMap() {
  initFuncs[FORWARD] = &forwardInit;
}

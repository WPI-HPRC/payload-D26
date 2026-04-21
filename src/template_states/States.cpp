#define TEMPLATE_STATES_OVERRIDE
#include "States.h"
#include "../State.h"


StateInitFunc initFuncs[NUM_STATES] = {};
StateLoopFunc loopFuncs[NUM_STATES] = {};

void initStateMap() {
  initFuncs[PRELAUNCH] = &prelaunchInit;
  initFuncs[BOOST] = &boostInit;
  initFuncs[COAST] = &coastInit;
  initFuncs[DROGUE_DESCENT] = &drogueDescentInit;
  initFuncs[MAIN_DESCENT] = &mainDescentInit;
  initFuncs[RECOVERY] = &recoveryInit;
  initFuncs[ABORT] = &abortInit;

  loopFuncs[PRELAUNCH] = &prelaunchLoop;
  loopFuncs[BOOST] = &boostLoop;
  loopFuncs[COAST] = &coastLoop;
  loopFuncs[DROGUE_DESCENT] = &drogueDescentLoop;
  loopFuncs[MAIN_DESCENT] = &mainDescentLoop;
  loopFuncs[RECOVERY] = &recoveryLoop;
  loopFuncs[ABORT] = &abortLoop;
}

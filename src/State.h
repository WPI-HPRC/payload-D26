#pragma once

#include "Context.h"
#ifdef __has_include
  #if __has_include("states/States.h")
    #include "states/States.h"
  #else
    #include "template_states/States.h"
  #endif
#else
  #include "template_states/States.h"
#endif

typedef void (*StateInitFunc)(StateData *data);
typedef StateID (*StateLoopFunc)(StateData *data, Context *ctx);

extern StateInitFunc initFuncs[NUM_STATES];
extern StateLoopFunc loopFuncs[NUM_STATES];

void initStateMap();

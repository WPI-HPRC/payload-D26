#pragma once
#include "Context.h"

byte imageBuffer[];
int bufferIndex;

void camera_init();
void receiveEvent(int);
void loop();
void checkSPA();
void handleSPA();


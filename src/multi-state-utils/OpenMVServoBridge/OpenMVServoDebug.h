#pragma once

#include <Arduino.h>

class ScrewDriveInterface;

bool handleOpenMVServoDebugCommand(const String& input,
                                   Stream& cameraOutput,
                                   ScrewDriveInterface& screwDrive,
                                   Stream& debugOutput);

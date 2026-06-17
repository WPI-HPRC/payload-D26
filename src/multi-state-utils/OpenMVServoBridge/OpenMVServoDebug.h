#pragma once

#include <Arduino.h>

class ScrewDriveInterface;

bool handleOpenMVServoDebugCommand(const String& input,
                                   HardwareSerial& cameraSerial,
                                   ScrewDriveInterface& screwDrive,
                                   Stream& debugOutput);

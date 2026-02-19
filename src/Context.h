#pragma once

#include <SdFat.h>
#include "Sensors/ASM330.h"
#include "Sensors/ICM20948.h"
#include "Sensors/INA219.h"
#include "Sensors/LSP22.h"
#include "Sensors/MAX10S.h"
#include "Servo.h"
#include "SensorManager/SensorBase.h"

struct ASM330Data;
struct LPS22Data;
struct ICMData;
struct MAX10SData;
struct INA219Data;

struct Context {
    File logFile;
    File errorLogFile;
    SdFs sd;
    bool sdInitialized;

    ASM330 accel;
    LPS22 baro;
    ICM20948 mag;
    MAX10S gps;
    INA219 curr;
    
    Servo airBrakes;
};

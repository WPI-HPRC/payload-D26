#pragma once

#include <stdint.h>
//#include "Sensors/ASM330.h"
//#include "Sensors/LSP22.h"
//#include "Sensors/ICM20948.h"
//#include "Sensors/MAX10S.h"
//#include "Sensors/INA219.h"
#include "Context.h"

enum SensorType {
    ASM330_TAG = 0,
    LPS22_TAG = 1,
    ICM20948_TAG = 2,
    MAX10S_TAG = 3,
    INA219_TAG = 4
};

union LogSensorData {
    ASM330Data asm330;
    LPS22Data lps22;
    ICMData icm20948;
    MAX10SData max10s;
    INA219Data ina219;
};


#pragma pack(push, 1)
struct Packet {
    uint8_t id; // NOTE: should identify the sensor and the length of this packet
    uint32_t timeStamp;
    LogSensorData data;
};
#pragma pack(pop)

bool initializeLogging(Context *ctx);

void loggingLoop(Context *ctx);

void writePacket(Context *ctx, uint32_t timestamp, LogSensorData data, SensorType type);

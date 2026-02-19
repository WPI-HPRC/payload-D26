#include <SdFat.h>
#include <stdint.h>
#include "logging.h"
//#include "Sensors/ASM330.h"
#include "config.h"

size_t dataLengths[] = {
    sizeof(ASM330Data),
    sizeof(LPS22Data), // Length of s2 data in Bytes
    sizeof(ICMData),
    sizeof(MAX10SData),
    sizeof(INA219Data)
};

bool initializeLogging(Context *ctx) {
    Serial.print("Initailizing SD... ");
    SPI.setSCLK(SD_SCLK);
    SPI.setMISO(SD_MISO);
    SPI.setMOSI(SD_MOSI);
    SPI.begin();

    if (ctx->sd.begin(SD_CS, SD_SPI_SPEED)) { 
        // TODO: Define these values
        int fileIdx = 0;
        char filename[100];
        char errorFilename[100];
        while (fileIdx < 100)
        {
            sprintf(filename, "flightData%d.csv", fileIdx);
            sprintf(errorFilename, "errorLog%d.txt", fileIdx++);

            Serial.printf("Trying files `%s/%s`\n", filename, errorFilename);
            if (!ctx->sd.exists(filename))
            {
                ctx->logFile = ctx->sd.open(filename, O_RDWR | O_CREAT | O_TRUNC);
                ctx->errorLogFile = ctx->sd.open(errorFilename, O_RDWR | O_CREAT | O_TRUNC);
                break;
            }
        }
        return true;
    }
    else
    {
        // NOTE: SD initialization failed
        // Do something about that probably
        Serial.println("FAILED");
        return false;
    }
}

void loggingLoop(Context *ctx) {
    static long lastTimeFlushedFiles = 0;

    if (millis() - lastTimeFlushedFiles >= 2000) {
        lastTimeFlushedFiles = millis();
        ctx->errorLogFile.flush();
        ctx->logFile.flush();
    }

    static long lastAccelDataAt = 0;
    const auto &accel_desc = ctx->accel.get_descriptor();
    if (accel_desc.getLastUpdated() > lastAccelDataAt) {
        lastAccelDataAt = accel_desc.getLastUpdated();
        LogSensorData d = {
            .asm330 = accel_desc.data
        };
        writePacket(ctx, lastAccelDataAt, d, ASM330_TAG);
    }

    static long lastBaroDataAt = 0;
    const auto &baro_desc = ctx->baro.get_descriptor();
    if (baro_desc.getLastUpdated() > lastBaroDataAt) {
        lastBaroDataAt = baro_desc.getLastUpdated();
        LogSensorData d = {
            .lps22 = baro_desc.data
        };
        writePacket(ctx, lastBaroDataAt, d, LPS22_TAG);
    }

    static long lastMagDataAt = 0;
    const auto &mag_desc = ctx->mag.get_descriptor();
    if (mag_desc.getLastUpdated() > lastMagDataAt) {
        lastMagDataAt = mag_desc.getLastUpdated();
        LogSensorData d = {
            .icm20948 = mag_desc.data
        };
        writePacket(ctx, lastMagDataAt, d, ICM20948_TAG);
    }

    static long lastGpsDataAt = 0;
    const auto &gps_desc = ctx->gps.get_descriptor();
    if (gps_desc.getLastUpdated() > lastGpsDataAt) {
        lastGpsDataAt = gps_desc.getLastUpdated();
        LogSensorData d = {
            .max10s = gps_desc.data
        };
        writePacket(ctx, lastGpsDataAt, d, MAX10S_TAG);
    }

    static long lastCurrentDataAt = 0;
    const auto &curr_desc = ctx->curr.get_descriptor();
    if (curr_desc.getLastUpdated() > lastCurrentDataAt) {
        lastCurrentDataAt = curr_desc.getLastUpdated();
        LogSensorData d = {
            .ina219 = curr_desc.data
        };
        writePacket(ctx, lastCurrentDataAt, d, INA219_TAG);
    }
}

void writePacket(Context *ctx, uint32_t timestamp, LogSensorData data, SensorType type) {
    Packet packetToWrite = { type, timestamp, data};

    size_t length = sizeof(uint8_t) + sizeof(uint32_t) + dataLengths[type];

    ctx->logFile.write(&packetToWrite, length);
}


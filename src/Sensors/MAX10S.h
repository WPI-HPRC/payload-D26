#pragma once

#include <stdint.h>
#include "../SensorManager/SensorBase.h"
#include <Arduino.h>
#include <SparkFun_u-blox_GNSS_v3.h>

struct MAX10SData {
    float lat;
    float lon;
    float altMSL;
    float altEllipsoid;
    int32_t velN;
    int32_t velE;
    int32_t velD;
    uint32_t epochTime;
    uint8_t satellites;
    uint8_t gpsLockType;
};

class MAX10S: public Sensor<MAX10S, MAX10SData> {
public:
    using DataType = MAX10SData;
    // static constexpr SensorDataType TYPE = SensorDataType::GPS;

    MAX10S() // 25
        : Sensor(40),
          GPS()//, last_update_ms_(0), poll_interval_ms_(1000 / info_.poll_rate_hz) {}
          {}

    bool init_impl() {
        Serial.print("Initializing MAX10S...");
        if (GPS.begin()) {
            GPS.setNavigationFrequency(25);
            GPS.setAutoPVT(true);
            Serial.println("OK");
            return true;
        } else {
            Serial.println("FAILED");
            return false;
        }
    }

    void poll_impl(uint32_t now_ms,  MAX10SData &out) {
        out.gpsLockType = GPS.getFixType();
        out.lat = (float)GPS.getLatitude() / 1e7;
        out.lon = (float)GPS.getLongitude() / 1e7;
        out.altMSL = (float) GPS.getAltitudeMSL() / 1000.0;
        out.altEllipsoid = (float) GPS.getAltitude() / 1000.0;
        out.velN = GPS.getNedNorthVel();
        out.velE = GPS.getNedEastVel();
        out.velD = GPS.getNedDownVel();
        out.epochTime = GPS.getUnixEpoch();
        out.satellites = GPS.getSIV(); // sat in view
    }
    
private:
    SFE_UBLOX_GNSS GPS;
};

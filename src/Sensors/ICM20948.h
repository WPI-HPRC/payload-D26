#pragma once

#include "../SensorManager/SensorBase.h"
#include "config.h"
#include <Adafruit_ICM20948.h>
#include <Arduino.h>
#include <Wire.h>

// #define ICM20948_POLLING_RATE
#define ODR 40
// is this the polling rate but named different

struct ICMData {
  float accel0, accel1, accel2;
  float gyr0, gyr1, gyr2;
  float mag0, mag1, mag2;
};

class ICM20948 : public Sensor<ICM20948, ICMData> {
public:
  ICM20948()
      : Sensor(ODR), // assuming this is the polling rate
        icm() {}
  // this doesnt seem done but whatever

  bool init_impl() {
    Serial.print("Initializing ICM20948...");

    if (!icm.begin_I2C(0x68)) {
      Serial.println("FAILED");
      return false;
    }

    icm.setAccelRange(ICM20948_ACCEL_RANGE_16_G);
    icm.setGyroRange(ICM20948_GYRO_RANGE_2000_DPS);
    uint16_t accelRateDiv =
        1125 / ODR - 1; // Per datasheet: ODR = 1125 / (1 + div)
    uint8_t gyrRateDiv =
        1100 / ODR - 1; // Per datasheet: ODR = 1100 / (1 + div)
    icm.setAccelRateDivisor(accelRateDiv);
    icm.setGyroRateDivisor(gyrRateDiv);
    Serial.println("OK");

    // init seems weird, will look at later
    return true;
  }

  void poll_impl(uint32_t now_ms, ICMData &out) {
    sensors_event_t accel, gyr, mag;
    icm.getEvent(&accel, &gyr, &mag);

    out.accel0 = accel.acceleration.x;
    out.accel1 = accel.acceleration.y;
    out.accel2 = accel.acceleration.z;

    out.gyr0 = gyr.gyro.x;
    out.gyr1 = -gyr.gyro.y;
    out.gyr2 = gyr.gyro.z;

    out.mag0 = mag.magnetic.x;
    out.mag1 = mag.magnetic.y;
    out.mag2 = mag.magnetic.z;
  }

private:
  Adafruit_ICM20948 icm;
};

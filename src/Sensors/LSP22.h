#pragma once

#include "../SensorManager/SensorBase.h"
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Wire.h>

struct LPS22Data {
  double pressure;
  double temp;
};

class LPS22 : public Sensor<LPS22, LPS22Data> {
public:
  LPS22() // 50
      : Sensor(50), lps() {}

  bool init_impl() {
    Serial.print("Initializing LPS22... ");

    if (!lps.begin_I2C(0x5C)) {
      Serial.println("FAILED");
      return false;
    }

    lps.setDataRate(LPS22_RATE_50_HZ);
    //poll_interval_ms_ = 1000 / info_.poll_rate_hz;
    Serial.println("OK");

    return true;
  }

  void poll_impl(uint32_t now_ms, LPS22Data &out) {
    sensors_event_t pressure, temperature;
    if (lps.getEvent(&pressure, &temperature)) {
      out.pressure = pressure.pressure;
      out.temp = temperature.temperature;
    }
  }

private:
  Adafruit_LPS22 lps;
};

#pragma once

#include "Adafruit_INA219.h"
#include "../SensorManager/SensorBase.h"
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_INA219.h"

// #define INA219_POLLING_RATE

struct INA219Data {
  float curr;
};

class INA219 : public Sensor<INA219, INA219Data> {
public:
  // using DataType = INA219Data;
  // static constexpr SensorDataType TYPE = SensorDataType::CURR;

  INA219()
      : Sensor(10), // not sure if 10 is the right time
        ina219() {};

  // INA219()
  //     : SensorBase<INA219, INA219Data>({TYPE, "INA219", 100}), ina219(),
  //       last_update_ms_(0), poll_interval_ms_(1000 / info_.poll_rate_hz) {}

  bool init_impl() {
    Serial.print("Initializing for INA219... ");

    if (!ina219.begin()) {
      Serial.println("FAILED");
      return false;
    }
    Serial.println("OK");
    return true;
  }

  void poll_impl(uint32_t now_ms, INA219Data &out) {
    float current_mA = ina219.getCurrent_mA();

    out.curr = current_mA;
  }

private:
  Adafruit_INA219 ina219;
};

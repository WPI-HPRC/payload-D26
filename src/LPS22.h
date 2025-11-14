#pragma once

#include "SensorManager.h"
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Wire.h>

struct LPS22Data {
  double pressure;
  double temperature;
};

class LPS22 : public SensorBase<LPS22, LPS22Data>, public ISensor {
public:
  using DataType = LPS22Data;
  static constexpr SensorDataType TYPE = SensorDataType::BARO;

  LPS22()
      : SensorBase<LPS22, LPS22Data>({TYPE, "LPS22", 20}), lps(),
        last_update_ms_(0), poll_interval_ms_(1000 / info_.poll_rate_hz) {}

  void init_impl() {
    Serial.print("Initializing LPS22... ");

    if (!lps.begin_I2C(0x5C)) {
      Serial.println("FAILED");
      return;
    }

    lps.setDataRate(LPS22_RATE_50_HZ);
    poll_interval_ms_ = 1000 / info_.poll_rate_hz;
    Serial.println("OK");
  }

  void update_impl(SensorDataDescriptor<DataType> &desc) {
    unsigned long now = millis();

    if (last_update_ms_ == 0) {
      last_update_ms_ = now;
    }

    if (now - last_update_ms_ < poll_interval_ms_) {
      return;
    }
    last_update_ms_ = now;

    sensors_event_t pressure, temperature;
    if (lps.getEvent(&pressure, &temperature)) {
      desc.data.pressure = pressure.pressure;
      desc.data.temperature = temperature.temperature;
      desc.timestamp = now;
    }
  }

  // ISensor interface implementation
  void init() override { init_impl(); }
  void update() override { update_impl(descriptor_); }
  SensorDataType type() const override { return TYPE; }
  const char *name() const override { return info_.name; }
  const void* get_descriptor_ptr() const override { return &descriptor_; }

private:
  Adafruit_LPS22 lps;
  unsigned long last_update_ms_;
  unsigned long poll_interval_ms_;
};

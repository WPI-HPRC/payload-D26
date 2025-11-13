#pragma once

#include "ASM330LHHSensor.h"
#include "SensorManager.h"
#include <Arduino.h>
#include <Wire.h>

struct ASM330Data {
  float accelX, accelY, accelZ, gyrX, gyrY, gyrZ;
};

class ASM330 : public SensorBase<ASM330, ASM330Data>, public ISensor {
public:
  using DataType = ASM330Data;
  static constexpr SensorDataType TYPE = SensorDataType::ACCEL;

  ASM330()
      : SensorBase<ASM330, ASM330Data>({TYPE, "ASM330", 26}),
        AccGyr(&Wire, ASM330LHH_I2C_ADD_H), last_update_ms_(0),
        poll_interval_ms_(1000 / info_.poll_rate_hz) {}

  void init_impl() {
    Serial.print("Initializing ASM330... ");

    if (AccGyr.begin() != 0) {
      Serial.println("FAILED");
      return;
    }

    AccGyr.Set_X_ODR(26.0f);
    AccGyr.Set_G_ODR(26.0f);
    AccGyr.Set_X_FS(16);
    AccGyr.Set_G_FS(2000);
    AccGyr.Enable_X();
    AccGyr.Enable_G();

    float odr = 0.0f;
    AccGyr.Get_X_ODR(&odr);
    poll_interval_ms_ = (odr > 0.0f) ? 1000.0f / odr : 40.0f;
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

    int32_t accel[3] = {0};
    int32_t gyro[3] = {0};

    AccGyr.Get_X_Axes(accel);
    AccGyr.Get_G_Axes(gyro);

    desc.data.accelX = -(float)accel[1] / 1000.0f;
    desc.data.accelY = (float)accel[0] / 1000.0f;
    desc.data.accelZ = (float)accel[2] / 1000.0f;
    desc.data.gyrX = -(float)gyro[1] / 1000.0f;
    desc.data.gyrY = (float)gyro[0] / 1000.0f;
    desc.data.gyrZ = (float)gyro[2] / 1000.0f;
    desc.timestamp = now;

    // Debug output
    Serial.print("ASM330 UPDATE - Accel: ");
    Serial.print(desc.data.accelX, 4);
    Serial.print(", ");
    Serial.print(desc.data.accelY, 4);
    Serial.print(", ");
    Serial.print(desc.data.accelZ, 4);
    Serial.print(" | Gyro: ");
    Serial.print(desc.data.gyrX, 4);
    Serial.print(", ");
    Serial.print(desc.data.gyrY, 4);
    Serial.print(", ");
    Serial.print(desc.data.gyrZ, 4);
    Serial.println();
  }

  // ISensor interface implementation
  void init() override { init_impl(); }
  void update() override { update_impl(descriptor_); }
  SensorDataType type() const override { return TYPE; }
  const char *name() const override { return info_.name; }

private:
  ASM330LHHSensor AccGyr;
  unsigned long last_update_ms_;
  unsigned long poll_interval_ms_;
};

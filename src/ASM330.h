#pragma once

#include "ASM330LHHSensor.h"
#include "SensorManager.h"
#include < Wire.h>
#include <Arduino.h>

struct ASM330Data {
  float accelX, accelY, accelZ, gyrX, gyrY, gyrZ;
};

class ASM330 : public SensorBase<ASM330, ASM330Data> {
public:
  using DataType = ASM330Data;
  static constexpr SensorDataType TYPE = SensorDataType::ACCEL;

  // construct
  ASM330()
      : SensorBase<ASM330, ASM330Data>({TYPE, "ASM330", 26}),
        AccGyr(&Wire, ASM330LHH_I2C_ADD_H), last_update_ms_(0),
        poll_interval_ms_(1000 / info_.poll_rate_hz) {}

  void init_impl() {
    if (AccGyr.begin() != 0) {
      Serial.println("ASM330 init failed :(");
      return;
    }

    AccGyr.Set_X_ODR(26.0f); // Hz
    AccGyr.Set_G_ODR(26.0f);
    AccGyr.Set_X_FS(16);   // ±16g
    AccGyr.Set_G_FS(2000); // ±2000 dps
    AccGyr.Enable_X();
    AccGyr.Enable_G();

    float odr = 0.0f;
    AccGyr.Get_X_ODR(&odr);
    poll_interval_ms_ = (odr > 0.0f) ? 1000.0f / odr : 40.0f; // fallback
    Serial.println("ASM330 initialized OK");
  }

  void update_impl(SensorDataDescriptor<DataType> &desc) {
    unsigned long now = millis();
    if (now - last_update_ms_ < poll_interval_ms_) {
      return; // skip if not time yet
    }
    last_update_ms_ = now;

    int32_t accel[3] = {0}, gyro[3] = {0};
    AccGyr.Get_X_Axes(accel);
    AccGyr.Get_G_Axes(gyro);

    // Rotation fix: remap axes to match ICM orientation
    desc.data.accelX = -(float)accel[1] / 1000.0f;
    desc.data.accelY = (float)accel[0] / 1000.0f;
    desc.data.accelZ = (float)accel[2] / 1000.0f;

    desc.data.gyrX = -(float)gyro[1] / 1000.0f;
    desc.data.gyrY = (float)gyro[0] / 1000.0f;
    desc.data.gyrZ = (float)gyro[2] / 1000.0f;

    desc.timestamp = now; // record update time

    // Serial.printf("ASM330: %.2f %.2f %.2f | %.2f %.2f %.2f\n",
    //   desc.data.accelX, desc.data.accelY, desc.data.accelZ,
    //   desc.data.gyrX, desc.data.gyrY, desc.data.gyrZ);
  }

private:
  ASM330LHHSensor AccGyr;
  unsigned long last_update_ms_;
  unsigned long poll_interval_ms_;
};

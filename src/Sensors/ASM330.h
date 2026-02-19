#pragma once

#include "../SensorManager/SensorBase.h"
#include "ASM330LHHSensor.h"
#include <Arduino.h>
#include <Wire.h>

struct ASM330Data {
  float accel0, accel1, accel2, gyr0, gyr1, gyr2;
};

#define ASM330_POLLING_RATE 26

class ASM330 : public Sensor<ASM330, ASM330Data> {
public:
  ASM330()
      : Sensor(ASM330_POLLING_RATE),
        AccGyr(&Wire, ASM330LHH_I2C_ADD_H) {}

  bool init_impl() {
    Serial.print("Initializing ASM330... ");

    if (AccGyr.begin() != 0) {
      Serial.println("FAILED");
      return false;
    }

    AccGyr.Set_X_ODR(26.0f);
    AccGyr.Set_G_ODR(26.0f);
    AccGyr.Set_X_FS(16);
    AccGyr.Set_G_FS(2000);
    AccGyr.Enable_X();
    AccGyr.Enable_G();

    float odr = 0.0f;
    AccGyr.Get_X_ODR(&odr);
    //this->pollingPeriodMs_ = (odr > 0.0f) ? 1000.0f / odr : 40.0f; // is this needed?
    Serial.println("OK");
    return true;
  }

  void poll_impl(uint32_t now_ms, ASM330Data &out) {
    // unsigned long now = millis();

    int32_t accel[3] = {0};
    int32_t gyro[3] = {0};

    AccGyr.Get_X_Axes(accel);
    AccGyr.Get_G_Axes(gyro);

    out.accel0 = (float)accel[0];
    out.accel1 = (float)accel[1]; 
    out.accel2 = (float)accel[2];    
    out.gyr0 = (float)gyro[0];
    out.gyr1 = (float)gyro[1];
    out.gyr2 = (float)gyro[2]; 
  }

private:
  ASM330LHHSensor AccGyr;
};

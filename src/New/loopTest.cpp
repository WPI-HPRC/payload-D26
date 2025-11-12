#include "ASM330.h"

ASM330 asm330;

SensorManager<ASM330> manager(&asm330);

void setup() {
  Serial.begin(115200);
  Wire.begin();

  manager.init_all();
}

void loop() {
  manager.update_all();

  const auto &desc = manager.get_descriptor<SensorDataType::ACCEL>();
  const auto &data = desc.data;

  Serial.printf("Accel: %.2f %.2f %.2f | Gyro: %.2f %.2f %.2f\n", data.accelX,
                data.accelY, data.accelZ, data.gyrX, data.gyrY, data.gyrZ);

  delay(20);
}

#include "ASM330.h"
#include "LPS22.h"
#include "config.h"

// Create sensor objects
ASM330 asm330;
LPS22 lps22;

// Create manager and add sensors (we'll still use it for init/update)
SensorManager manager;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("Starting MARS board initialization...");

  Wire.setSDA(SENSOR_SDA);
  Wire.setSCL(SENSOR_SCL);
  Wire.begin();

  Serial.print("I2C initialized on SDA: ");
  Serial.print(SENSOR_SDA);
  Serial.print(", SCL: ");
  Serial.println(SENSOR_SCL);

  // Add sensors to manager
  manager.add_sensor(&asm330);
  manager.add_sensor(&lps22);

  // Initialize all sensors
  manager.init_all();

  Serial.println("\n=== Sensor Initialization Summary ===");
  Serial.print("Total sensors: ");
  Serial.println(manager.count());
  Serial.println("=== Starting main loop ===\n");

  delay(2000);
}

void loop() {
  static unsigned long last_print = 0;
  static int loop_count = 0;

  // Update all sensors through manager
  manager.update_all();

  // manager is not being used here to get data
  if (millis() - last_print > 1000) {
    last_print = millis();
    loop_count++;

    Serial.print("\n=== Loop ");
    Serial.print(loop_count);
    Serial.println(" ===");

    // DIRECT ACCESS to sensor data - this is guaranteed to work
    const auto &accel_desc = asm330.descriptor();
    const auto &baro_desc = lps22.descriptor();

    bool has_data = false;

    // Print ASM330 data
    if (accel_desc.timestamp > 0) {
      Serial.print("ASM330 - Accel: ");
      Serial.print(accel_desc.data.accelX, 4);
      Serial.print(", ");
      Serial.print(accel_desc.data.accelY, 4);
      Serial.print(", ");
      Serial.print(accel_desc.data.accelZ, 4);
      Serial.print(" | Gyro: ");
      Serial.print(accel_desc.data.gyrX, 4);
      Serial.print(", ");
      Serial.print(accel_desc.data.gyrY, 4);
      Serial.print(", ");
      Serial.print(accel_desc.data.gyrZ, 4);
      Serial.println();
      has_data = true;
    } else {
      Serial.println("ASM330: No data (timestamp = 0)");
    }

    // Print LPS22 data
    if (baro_desc.timestamp > 0) {
      Serial.print("LPS22 - Pressure: ");
      Serial.print(baro_desc.data.pressure, 4);
      Serial.print(" hPa, Temp: ");
      Serial.print(baro_desc.data.temperature, 4);
      Serial.println(" C");
      has_data = true;
    } else {
      Serial.println("LPS22: No data (timestamp = 0)");
    }

    if (!has_data) {
      Serial.println("No sensor data received yet...");
    }

    Serial.println("======================");
  }
}

#include "ASM330.h"
#include "LPS22.h"
#include "config.h"

// Create sensor objects
ASM330 asm330;
LPS22 lps22;

// Create manager and add sensors
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
  
  // Update all sensors
  manager.update_all();

  // Print data every second
  if (millis() - last_print > 1000) {
    last_print = millis();
    loop_count++;
    
    Serial.print("\n=== Loop ");
    Serial.print(loop_count);
    Serial.println(" ===");
    
    bool has_data = false;
    
    // Read and print accelerometer data
    if (manager.has_sensor<ASM330>()) {
      const auto &accel_desc = manager.get_descriptor<ASM330>();
      
      if (accel_desc.timestamp > 0) {
        const auto &accel_data = accel_desc.data;
        Serial.printf("ASM330 - Accel: %.4f, %.4f, %.4f | Gyro: %.4f, %.4f, %.4f\n", 
                      accel_data.accelX, accel_data.accelY, accel_data.accelZ, 
                      accel_data.gyrX, accel_data.gyrY, accel_data.gyrZ);
        has_data = true;
      } else {
        Serial.println("ASM330: No data (timestamp = 0)");
      }
    }

    // Read and print barometer data
    if (manager.has_sensor<LPS22>()) {
      const auto &baro_desc = manager.get_descriptor<LPS22>();
      
      if (baro_desc.timestamp > 0) {
        const auto &baro_data = baro_desc.data;
        Serial.printf("LPS22 - Pressure: %.4f hPa | Temp: %.4f C\n", 
                      baro_data.pressure, baro_data.temperature);
        has_data = true;
      } else {
        Serial.println("LPS22: No data (timestamp = 0)");
      }
    }
    
    if (!has_data) {
      Serial.println("No sensor data received yet...");
    }
    
    Serial.println("======================");
  }
  
  delay(100);
}

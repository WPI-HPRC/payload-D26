#pragma once

#include <Arduino.h>

// Sensor types
enum class SensorDataType : uint8_t {
  ACCEL = 0,
  BARO = 1,
  GPS = 2,
};

// Forward declarations
class ASM330;
class LPS22;

// Sensor data descriptor
template <typename T> 
struct SensorDataDescriptor {
  SensorDataType type;
  unsigned long timestamp;
  T data;
};

// Sensor info
struct SensorInfo {
  SensorDataType type;
  const char *name;
  uint8_t poll_rate_hz;
};

// Sensor base class
template <class Derived, class Data>
class SensorBase {
public:
  using DataType = Data;
  
  SensorBase(const SensorInfo &info) : info_(info) {
    descriptor_.type = info.type;
    descriptor_.timestamp = 0;
  }

  void init() {
    static_cast<Derived *>(this)->init_impl();
  }

  void update() {
    static_cast<Derived *>(this)->update_impl(descriptor_);
  }

  const SensorDataDescriptor<DataType> &descriptor() const {
    return descriptor_;
  }

  SensorDataType type() const { return info_.type; }
  const char* name() const { return info_.name; }
  uint8_t poll_rate_hz() const { return info_.poll_rate_hz; }

protected:
  SensorInfo info_;
  SensorDataDescriptor<DataType> descriptor_;
};

// Simple non-template sensor interface
class ISensor {
public:
  virtual ~ISensor() = default;
  virtual void init() = 0;
  virtual void update() = 0;
  virtual SensorDataType type() const = 0;
  virtual const char* name() const = 0;
};

// Simple SensorManager that can handle multiple sensors
class SensorManager {
public:
  SensorManager() = default;

  // Add a sensor to the manager
  template <typename Sensor>
  void add_sensor(Sensor* sensor) {
    if (sensor_count_ < MAX_SENSORS) {
      sensors_[sensor_count_++] = sensor;
    }
  }

  // Initialize all sensors
  void init_all() {
    for (int i = 0; i < sensor_count_; i++) {
      sensors_[i]->init();
    }
  }

  // Update all sensors
  void update_all() {
    for (int i = 0; i < sensor_count_; i++) {
      sensors_[i]->update();
    }
  }

  // Get descriptor by type - you need to specify the sensor type
  template <typename Sensor>
  const auto& get_descriptor() {
    for (int i = 0; i < sensor_count_; i++) {
      if (sensors_[i]->type() == Sensor::TYPE) {
        return static_cast<Sensor*>(sensors_[i])->descriptor();
      }
    }
    
    // Fallback
    static typename Sensor::DataType dummy{};
    static SensorDataDescriptor<typename Sensor::DataType> dummy_desc{Sensor::TYPE, 0, dummy};
    return dummy_desc;
  }

  // Check if sensor type exists
  template <typename Sensor>
  bool has_sensor() const {
    for (int i = 0; i < sensor_count_; i++) {
      if (sensors_[i]->type() == Sensor::TYPE) {
        return true;
      }
    }
    return false;
  }

  // Get number of sensors
  int count() const { return sensor_count_; }

private:
  static constexpr int MAX_SENSORS = 8;
  ISensor* sensors_[MAX_SENSORS] = {nullptr};
  int sensor_count_ = 0;
};

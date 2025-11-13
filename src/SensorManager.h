#pragma once

#include <Arduino.h>
#include <tuple>
#include <type_traits>
#include <utility> 

// sensor types
enum class SensorDataType : uint8_t {
  ACCEL = 0,
  BARO = 1,
};

// sensor type, timestamp, data
template <typename T> 
struct SensorDataDescriptor {
  SensorDataType type;
  unsigned long timestamp;
  T data;
};

struct SensorInfo {
  SensorDataType type;
  const char *name;
  uint8_t poll_rate_hz;
};

template <class Derived, class Data>
class SensorBase {
public:
  using DataType = Data;
  
  constexpr SensorBase(const SensorInfo &info) : info_(info) {
    descriptor_.type = info.type;
    descriptor_.timestamp = 0;
  }

  inline void init() {
    static_cast<Derived *>(this)->init_impl();
  }

  inline void update() {
    static_cast<Derived *>(this)->update_impl(descriptor_);
  }

  inline const SensorDataDescriptor<DataType> &descriptor() const {
    return descriptor_;
  }

  inline SensorDataType type() const { return info_.type; }

  inline uint8_t poll_rate_hz() const { return info_.poll_rate_hz; }

protected:
  SensorInfo info_;
  SensorDataDescriptor<DataType> descriptor_;
};

template <class... Sensors> 
class SensorManager {
public:
  explicit SensorManager(Sensors *...sensors) : sensors_(sensors...) {}

  void init_all() { 
    std::apply([](auto*... s) { (s->init(), ...); }, sensors_);
  }

  void update_all() { 
    std::apply([](auto*... s) { (s->update(), ...); }, sensors_);
  }

  // Get descriptor by sensor type - compile-time checked
  template <SensorDataType Target>
  const auto& get_descriptor() const {
    return get_descriptor_impl<Target>(std::make_index_sequence<sizeof...(Sensors)>{});
  }

private:
  std::tuple<Sensors *...> sensors_;

  // Helper to find and return the right descriptor
  template <SensorDataType Target, size_t... Is>
  const auto& get_descriptor_impl(std::index_sequence<Is...>) const {
    const auto* sensor = find_sensor<Target>(std::get<Is>(sensors_)...);
    
    if (!sensor) {
      // This should never happen if static checks pass, but provide a fallback
      static typename std::tuple_element_t<0, std::tuple<Sensors...>>::DataType dummy{};
      static SensorDataDescriptor<decltype(dummy)> dummy_desc{Target, 0, dummy};
      return dummy_desc;
    }
    
    return sensor->descriptor();
  }

  // Recursive function to find sensor by type
  template <SensorDataType Target, class First, class... Rest>
  static const First* find_sensor(const First* first, const Rest*... rest) {
    if (first->type() == Target) {
      return first;
    }
    
    if constexpr (sizeof...(Rest) > 0) {
      return find_sensor<Target>(rest...);
    } else {
      return nullptr;
    }
  }
};

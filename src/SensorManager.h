#pragma once

#include <Arduino.h>
#include <tuple>
#include <type_traits>

// sensor types
enum class SensorDataType : uint8_t {
  ACCEL = 0,
  BARO = 1,
  // ... they go here
};

// sensor type, timestamp, data
template <typename T> struct SensorDataDescriptor {
  SensorDataType type;
  unsigned long timestamp;
  T data;
};

struct SensorInfo {
  SensorDataType type;
  const char *name; // is this needed
  uint8_t poll_rate_hz;
};

template <class Derived, class Data>
class SensorBase {
public:
  // force dervied to define a class
  using DataType = Data;
  
  // data at construction
  constexpr SensorBase(const SensorInfo &info) : info_(info) {
    descriptor_.type = info.type;
    descriptor_.timestamp = 0;
  }

  inline void init() {
    // crtp downcast, call derived impl
    static_cast<Derived *>(this)->init_impl();
  }

  inline void update() {
    static_cast<Derived *>(this)->update_impl(descriptor_);
  }

  // accessors
  inline const SensorDataDescriptor<DataType> &descriptor() const {
    return descriptor_;
  }

  inline SensorDataType type() const { return info_.type; }

  inline uint8_t poll_rate_hz() const { return info_.poll_rate_hz; }

protected:
  SensorInfo info_;
  SensorDataDescriptor<DataType> descriptor_;
};

// find by type at compile time
template <SensorDataType Target, class... Sensors>
struct find_sensor_by_type_impl;

// base: no more to look at
template <SensorDataType Target> 
struct find_sensor_by_type_impl<Target> {
  // This will cause substitution failure but won't trigger static_assert immediately
  // The error will occur when someone tries to use ::type
};

// recur: check if first sensor match
template <SensorDataType Target, class First, class... Rest>
struct find_sensor_by_type_impl<Target, First, Rest...> {
  using type = typename std::conditional<
      (First::TYPE == Target), 
      First,
      typename find_sensor_by_type_impl<Target, Rest...>::type
  >::type;
};

// alias
template <SensorDataType Target, class... Sensors>
using find_sensor_by_type = typename find_sensor_by_type_impl<Target, Sensors...>::type;

template <class... Sensors> class SensorManager {
public:
  // already constructed sensor obj
  explicit SensorManager(Sensors *...sensors) : sensors_(sensors...) {}

  // init all
  void init_all() { init_all_impl<0>(); }

  void update_all() { update_all_impl<0>(); }

  template <SensorDataType Target>
  const SensorDataDescriptor<
      typename find_sensor_by_type<Target, Sensors...>::DataType> &
  get_descriptor() const {
    auto *sensor =
        std::get<find_sensor_by_type<Target, Sensors...> *>(sensors_);
    return sensor->descriptor();
  }

private:
  std::tuple<Sensors *...> sensors_;

  // recursive init
  template <size_t I = 0>
  typename std::enable_if<I == sizeof...(Sensors), void>::type init_all_impl() {
  }
  // base case, do nothing

  template <size_t I = 0>
      typename std::enable_if <
      I<sizeof...(Sensors), void>::type init_all_impl() {
    std::get<I>(sensors_)->init();
    init_all_impl<I + 1>();
  }

  // recur update
  template <size_t I = 0>
  typename std::enable_if<I == sizeof...(Sensors), void>::type
  update_all_impl() {}
  // base case, do nothing

  template <size_t I = 0>
      typename std::enable_if <
      I<sizeof...(Sensors), void>::type update_all_impl() {
    std::get<I>(sensors_)->update();
    update_all_impl<I + 1>();
  }
};

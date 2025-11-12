#include <Arduino.h>
//tuple ?
enum SensorDataType : uint8_t {
  DATA_TYPE_IMU_1 = 0,
  DATA_TYPE_IMU_2 = 1,
};

template <typename T>
struct SensorDataDescriptor {
  SensorDataType type;
  size_t size;
  unsigned long time_stamp;
  T* data;
};

struct SensorInfo {
  SensorDataType type;
  const char* name;
  uint8_t poll_rate_hz;
}

// base for sensors
template <class Derived>
class SensorBase {
public:
  SensorBase(
      const SensorDataType& dataType
      const SensorInfo& info
  ) : 
    m_dataType(dataType) 
    m_info(info)
  {}

  inline void init() {
    static_cast<Derived*>(this)->init_impl();
  }

  inline long poll_rate() const {
    static_cast<Derived*>(this)->poll_rate_impl();
  }
  
  inline uint8_t poll_rate_hz() const {
    return m_info.poll_rate_hz;
  }

  inline size_t data_size() const {
    return m_dataDescriptor == NULL ? -1 : m_dataDescriptor.size; 
  }

private:
  SensorDataType m_dataType;
  SensorInfo m_info;
  SensorDataDescriptor<typename Derived::DataType> m_dataDescriptor = NULL;
};

// might want helpers for allocating and stuff?
// want some way to find what the larges allocation will be


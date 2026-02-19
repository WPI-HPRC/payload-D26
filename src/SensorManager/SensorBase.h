// Sensor.h
#pragma once
#include "SensorData.h"

template <typename Derived, typename DataT>
class Sensor {
public:
    using data_type = DataT;
    using descriptor_type = SensorData<DataT>;

    bool init() {
        initStatus_ = derived().init_impl();
        return initStatus_;
    }

    bool getInitStatus() const { return initStatus_; }

    uint32_t getPollingPeriod() const { return pollingPeriodMs_; }

    uint32_t getLastTimePolled() const { 
        return data_.getLastUpdated(); 
    }

    // Called by SensorManager
    void poll(uint32_t now_ms) {
        // let the derived class fill in data_.value
        derived().poll_impl(now_ms, data_.data);
        data_.markUpdated(now_ms);
    }

    // Access to the data
    // const version allows for read only access to the data
    // non-const version allows for read and write access to the data
    const descriptor_type& get_descriptor() const { return data_; }
    descriptor_type& get_descriptor() { return data_; }

protected:
    explicit Sensor(uint32_t polling_ms) : pollingPeriodMs_(polling_ms) {}

    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

    descriptor_type data_; // contains {value, lastUpdated}
    uint32_t pollingPeriodMs_;
    bool initStatus_ = false;
};

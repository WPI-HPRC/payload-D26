#pragma once
#include <cstdint>

template<typename T>
struct SensorData {
    T data{}; // actual sensor data
    uint32_t lastUpdated{0}; // use millis()

    uint32_t getLastUpdated() const { return lastUpdated; }

    void markUpdated(uint32_t now_ms) {
        lastUpdated = now_ms;
    }
};

#pragma once

#include <Arduino.h>

#include "../AntennaConnector/AntennaConnectorInterface.h"

class VoltageSensorInterface {
  public:
    VoltageSensorInterface(
        int positivePin,
        int negativePin,
        float adcReferenceVoltage = 3.3f,
        float dividerRatio = 5.0f,
        uint16_t adcMaxReading = 1023,
        uint32_t pollIntervalMs = 100
    );

    void begin();
    void poll();

    void setAntennaConnector(AntennaConnectorInterface* antennaConnector);
    void setDebugOutput(Stream* debugOutput);
    void setDebugEnabled(bool enabled);
    void setPollInterval(uint32_t intervalMs);

    float getVoltage() const;
    int getRawPositiveReading() const;
    int getRawNegativeReading() const;
    int getRawDifferentialReading() const;

  private:
    int positivePin;
    int negativePin;
    float adcReferenceVoltage;
    float dividerRatio;
    uint16_t adcMaxReading;
    uint32_t pollIntervalMs;

    AntennaConnectorInterface* antennaConnector = nullptr;
    Stream* debugOutput = nullptr;
    bool debugEnabled = false;
    uint32_t lastPollAt = 0;
    bool hasPolled = false;

    int rawPositiveReading = 0;
    int rawNegativeReading = 0;
    int rawDifferentialReading = 0;
    float voltage = 0.0f;

    void publishReading();
    void printDebugReading();
};

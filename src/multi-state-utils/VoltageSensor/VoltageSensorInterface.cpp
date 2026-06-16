#include "VoltageSensorInterface.h"

VoltageSensorInterface::VoltageSensorInterface(
    int positivePin,
    int negativePin,
    float adcReferenceVoltage,
    float dividerRatio,
    uint16_t adcMaxReading,
    uint32_t pollIntervalMs
) :
    positivePin(positivePin),
    negativePin(negativePin),
    adcReferenceVoltage(adcReferenceVoltage),
    dividerRatio(dividerRatio),
    adcMaxReading(adcMaxReading),
    pollIntervalMs(pollIntervalMs) {}

void VoltageSensorInterface::begin() {
    pinMode(positivePin, INPUT);
    pinMode(negativePin, INPUT);
}

void VoltageSensorInterface::poll() {
    uint32_t now = millis();

    if (hasPolled && now - lastPollAt < pollIntervalMs) {
        return;
    }

    lastPollAt = now;
    hasPolled = true;

    rawPositiveReading = analogRead(positivePin);
    rawNegativeReading = analogRead(negativePin);
    rawDifferentialReading = rawPositiveReading - rawNegativeReading;

    if (rawDifferentialReading < 0) {
        rawDifferentialReading = 0;
    }

    voltage = (static_cast<float>(rawDifferentialReading) / static_cast<float>(adcMaxReading)) *
              adcReferenceVoltage *
              dividerRatio;

    publishReading();
    printDebugReading();
}

void VoltageSensorInterface::setAntennaConnector(AntennaConnectorInterface* antennaConnector) {
    this->antennaConnector = antennaConnector;
}

void VoltageSensorInterface::setDebugOutput(Stream* debugOutput) {
    this->debugOutput = debugOutput;
}

void VoltageSensorInterface::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

void VoltageSensorInterface::setPollInterval(uint32_t intervalMs) {
    pollIntervalMs = intervalMs;
}

float VoltageSensorInterface::getVoltage() const {
    return voltage;
}

int VoltageSensorInterface::getRawPositiveReading() const {
    return rawPositiveReading;
}

int VoltageSensorInterface::getRawNegativeReading() const {
    return rawNegativeReading;
}

int VoltageSensorInterface::getRawDifferentialReading() const {
    return rawDifferentialReading;
}

void VoltageSensorInterface::publishReading() {
    if (antennaConnector == nullptr) {
        return;
    }

    antennaConnector->setSensorValue("batteryVoltage", voltage);
}

void VoltageSensorInterface::printDebugReading() {
    if (!debugEnabled || debugOutput == nullptr) {
        return;
    }

    debugOutput->print("{\"type\":\"debug\",\"source\":\"voltageSensor\",\"rawPositive\":");
    debugOutput->print(rawPositiveReading);
    debugOutput->print(",\"rawNegative\":");
    debugOutput->print(rawNegativeReading);
    debugOutput->print(",\"rawDifferential\":");
    debugOutput->print(rawDifferentialReading);
    debugOutput->print(",\"voltage\":");
    debugOutput->print(voltage, 3);
    debugOutput->println("}");
}

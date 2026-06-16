#include "VoltageSensorInterface.h"

VoltageSensorInterface::VoltageSensorInterface(
    int positivePin,
    int negativePin,
    float adcReferenceVoltage,
    float dividerRatio,
    uint16_t adcMaxReading,
    uint32_t pollIntervalMs,
    float voltageDropThreshold,
    float zeroVoltageThreshold
) :
    positivePin(positivePin),
    negativePin(negativePin),
    adcReferenceVoltage(adcReferenceVoltage),
    dividerRatio(dividerRatio),
    adcMaxReading(adcMaxReading),
    pollIntervalMs(pollIntervalMs),
    voltageDropThreshold(voltageDropThreshold),
    zeroVoltageThreshold(zeroVoltageThreshold) {}

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

    updateVoltageDropReference();
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

void VoltageSensorInterface::setVoltageDropThreshold(float threshold) {
    voltageDropThreshold = max(0.0f, threshold);
}

void VoltageSensorInterface::setZeroVoltageThreshold(float threshold) {
    zeroVoltageThreshold = max(0.0f, threshold);
}

void VoltageSensorInterface::resetVoltageDropReference() {
    if (isApproxZeroVoltage(voltage)) {
        hasVoltageDropReference = false;
        voltageDropReference = 0.0f;
        return;
    }

    voltageDropReference = voltage;
    hasVoltageDropReference = true;
}

bool VoltageSensorInterface::checkVoltageDrop() const {
    if (!hasVoltageDropReference || isApproxZeroVoltage(voltage)) {
        return false;
    }

    return getVoltageDrop() >= voltageDropThreshold;
}

float VoltageSensorInterface::getVoltage() const {
    return voltage;
}

float VoltageSensorInterface::getVoltageDropReference() const {
    return voltageDropReference;
}

float VoltageSensorInterface::getVoltageDrop() const {
    if (!hasVoltageDropReference || voltage >= voltageDropReference) {
        return 0.0f;
    }

    return voltageDropReference - voltage;
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

bool VoltageSensorInterface::isApproxZeroVoltage(float voltage) const {
    return voltage <= zeroVoltageThreshold;
}

void VoltageSensorInterface::updateVoltageDropReference() {
    if (isApproxZeroVoltage(voltage)) {
        return;
    }

    if (!hasVoltageDropReference || voltage > voltageDropReference) {
        voltageDropReference = voltage;
        hasVoltageDropReference = true;
    }
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
    debugOutput->print(",\"dropReference\":");
    debugOutput->print(voltageDropReference, 3);
    debugOutput->print(",\"drop\":");
    debugOutput->print(getVoltageDrop(), 3);
    debugOutput->print(",\"dropDetected\":");
    debugOutput->print(checkVoltageDrop() ? "true" : "false");
    debugOutput->println("}");
}

#include "AntennaConnectorInterface.h"

bool AntennaConnectorInterface::SensorData::setValue(const String &parameterName, float value) {
    if (parameterName == "leftScrewCurrent") {
        leftScrewCurrent = value;
        return true;
    }

    if (parameterName == "rightScrewCurrent") {
        rightScrewCurrent = value;
        return true;
    }

    if (parameterName == "batteryVoltage") {
        batteryVoltage = value;
        return true;
    }

    return false;
}

bool AntennaConnectorInterface::SensorData::getValue(const String &parameterName, float &value) const {
    if (parameterName == "leftScrewCurrent") {
        value = leftScrewCurrent;
        return true;
    }

    if (parameterName == "rightScrewCurrent") {
        value = rightScrewCurrent;
        return true;
    }

    if (parameterName == "batteryVoltage") {
        value = batteryVoltage;
        return true;
    }

    return false;
}

String AntennaConnectorInterface::SensorData::getAllJson() const {
    String json = "{";
    json += "\"leftScrewCurrent\":";
    json += String(leftScrewCurrent, 3);
    json += ",\"rightScrewCurrent\":";
    json += String(rightScrewCurrent, 3);
    json += ",\"batteryVoltage\":";
    json += String(batteryVoltage, 3);
    json += "}";
    return json;
}

void AntennaConnectorInterface::DriveData::set(float speedInput, float turnInput) {
    speed = constrain(speedInput, -1.0f, 1.0f);
    turn = constrain(turnInput, -1.0f, 1.0f);
}

void AntennaConnectorInterface::DriveData::get(float &speedOutput, float &turnOutput) const {
    speedOutput = speed;
    turnOutput = turn;
}

bool AntennaConnectorInterface::hasConnection() {

    /// real entry point
    /**
     * Nic - this is where GNC can interface 
     */


    /// test hook: if simulated connection is enabled, return true until the duration has passed
    if (simulatedConnectionEnabled) {
        uint32_t now = millis();

        if (now - simulatedConnectionStartedAt < simulatedConnectionDuration) {
            return true;
        }

        simulatedConnectionEnabled = false;
    }

    return false;
}

bool AntennaConnectorInterface::onConnectionGained() {
    uint32_t now = millis();

    updateRawConnection(hasConnection(), now);

    if (!confirmedConnection &&
        rawConnection &&
        now - rawConnectionChangedAt >= connectionHeldTime) {
        confirmedConnection = true;
        return true;
    }

    return false;
}

bool AntennaConnectorInterface::onConnectionLost() {
    uint32_t now = millis();

    updateRawConnection(hasConnection(), now);

    if (confirmedConnection &&
        !rawConnection &&
        now - rawConnectionChangedAt >= connectionLostTime) {
        confirmedConnection = false;
        return true;
    }

    return false;
}

bool AntennaConnectorInterface::isConnectionConfirmed() {
    return confirmedConnection;
}

void AntennaConnectorInterface::simulateConnectionFor(uint32_t durationMs) {
    simulatedConnectionEnabled = true;
    simulatedConnectionStartedAt = millis();
    simulatedConnectionDuration = durationMs;
}

void AntennaConnectorInterface::clearSimulatedConnection() {
    simulatedConnectionEnabled = false;
}

void AntennaConnectorInterface::reset() {
    rawConnection = false;
    confirmedConnection = false;
    rawConnectionChangedAt = millis();
    clearSimulatedConnection();
}

bool AntennaConnectorInterface::setSensorValue(const String &parameterName, float value) {
    return sensorData.setValue(parameterName, value);
}

bool AntennaConnectorInterface::getSensorValue(const String &parameterName, float &value) const {
    return sensorData.getValue(parameterName, value);
}

String AntennaConnectorInterface::getAllSensorDataJson() const {
    return sensorData.getAllJson();
}

void AntennaConnectorInterface::setDriveData(float speed, float turn) {
    driveData.set(speed, turn);
}

AntennaConnectorInterface::DriveData AntennaConnectorInterface::getDriveData() const {
    return driveData;
}

void AntennaConnectorInterface::updateRawConnection(bool currentConnection, uint32_t now) {
    if (currentConnection == rawConnection) {
        return;
    }

    rawConnection = currentConnection;
    rawConnectionChangedAt = now;
}

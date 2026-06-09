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
    if (simulatedConnectionEnabled) {
        uint32_t now = millis();

        if (now - simulatedConnectionStartedAt < simulatedConnectionDuration) {
            return true;
        }

        simulatedConnectionEnabled = false;
    }

    return transmitterConnectionStatus;
}

void AntennaConnectorInterface::setTransmitterConnectionStatus(bool connected) {
    transmitterConnectionStatus = connected;
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
    transmitterConnectionStatus = false;
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

bool AntennaConnectorInterface::accessImageData(const String*& base64Data, int& byteCount) {
    if (!pendingImageAvailable) {
        base64Data = nullptr;
        byteCount = 0;
        return false;
    }

    pendingImageLocked = true;
    base64Data = &pendingImageData;
    byteCount = pendingImageByteCount;
    return true;
}

bool AntennaConnectorInterface::accessImageData(String& base64Data, int& byteCount) {
    const String* imageData = nullptr;

    if (!accessImageData(imageData, byteCount) || imageData == nullptr) {
        base64Data = "";
        return false;
    }

    base64Data = *imageData;
    return true;
}

void AntennaConnectorInterface::antennaReadyForImageData() {
    pendingImageData = "";
    pendingImageByteCount = 0;
    pendingImageAvailable = false;
    pendingImageLocked = false;
}

bool AntennaConnectorInterface::canAcceptImages() {
    return !pendingImageAvailable && !pendingImageLocked;
}

bool AntennaConnectorInterface::canAcceptImageData() {
    return canAcceptImages();
}

bool AntennaConnectorInterface::intakeImageData(const String& base64Data, int byteCount) {
    if (!canAcceptImages()) {
        return false;
    }

    pendingImageData = base64Data;
    pendingImageByteCount = byteCount;
    pendingImageAvailable = true;
    pendingImageLocked = false;

    return true;
}

void AntennaConnectorInterface::updateRawConnection(bool currentConnection, uint32_t now) {
    if (currentConnection == rawConnection) {
        return;
    }

    rawConnection = currentConnection;
    rawConnectionChangedAt = now;
}

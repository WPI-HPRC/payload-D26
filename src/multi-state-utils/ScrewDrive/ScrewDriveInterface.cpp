#include "ScrewDriveInterface.h"

void ScrewDriveInterface::attach(int leftPin, int rightPin) {
    leftEsc.attach(leftPin);
    rightEsc.attach(rightPin);
    attached = true;
    armed = false;
    arming = false;
    stop();
}

void ScrewDriveInterface::detach() {
    armed = false;
    arming = false;
    leftEsc.detach();
    rightEsc.detach();
    attached = false;
}

void ScrewDriveInterface::beginArm(uint32_t armingDurationMs) {
    armed = false;
    arming = true;
    armStartedAt = millis();
    armDurationMs = armingDurationMs;
    stop();
}

bool ScrewDriveInterface::updateArm() {
    if (armed) {
        return true;
    }

    if (!arming) {
        return false;
    }

    stop();

    if (millis() - armStartedAt >= armDurationMs) {
        armed = true;
        arming = false;
        return true;
    }

    return false;
}

bool ScrewDriveInterface::isArmed() const {
    return armed;
}

void ScrewDriveInterface::stop() {
    writeEfforts(0.0f, 0.0f);
}

void ScrewDriveInterface::drive(float speed, float turn) {
    if (!armed) {
        stop();
        return;
    }

    speed = clampUnit(speed);
    turn = clampUnit(turn);

    float leftEffort = 0.0f;
    float rightEffort = 0.0f;

    switch (driveStrategy) {
        case ARCADE:
        default:
            leftEffort = speed + (turnScale * turn);
            rightEffort = speed - (turnScale * turn);
            break;
    }

    float maxMagnitude = max(abs(leftEffort), abs(rightEffort));
    if (maxMagnitude > 1.0f) {
        leftEffort /= maxMagnitude;
        rightEffort /= maxMagnitude;
    }

    writeEfforts(leftEffort, rightEffort);
}

void ScrewDriveInterface::setDriveStrategy(DriveControlStrategy strategy) {
    driveStrategy = strategy;
}

void ScrewDriveInterface::setMaxEffort(float effort) {
    maxEffort = constrain(effort, 0.0f, 1.0f);
}

void ScrewDriveInterface::setMotorCorrections(float leftScale, float rightScale) {
    leftCorrection = max(0.0f, leftScale);
    rightCorrection = max(0.0f, rightScale);
}

void ScrewDriveInterface::setMotorInversions(bool leftInverted, bool rightInverted) {
    invertLeft = leftInverted;
    invertRight = rightInverted;
}

float ScrewDriveInterface::getLastLeftEffort() const {
    return lastLeftEffort;
}

float ScrewDriveInterface::getLastRightEffort() const {
    return lastRightEffort;
}

float ScrewDriveInterface::clampUnit(float value) const {
    return constrain(value, -1.0f, 1.0f);
}

float ScrewDriveInterface::applyOutputScaling(float effort, float correction, bool inverted) const {
    effort = clampUnit(effort) * maxEffort * correction;
    effort = clampUnit(effort);

    if (inverted) {
        effort = -effort;
    }

    return effort;
}

uint16_t ScrewDriveInterface::effortToPulseUs(float effort) const {
    effort = clampUnit(effort);

    if (effort >= 0.0f) {
        return neutralPulseUs + static_cast<uint16_t>((maxPulseUs - neutralPulseUs) * effort);
    }

    return neutralPulseUs - static_cast<uint16_t>((neutralPulseUs - minPulseUs) * -effort);
}

void ScrewDriveInterface::writeEfforts(float leftEffort, float rightEffort) {
    lastLeftEffort = applyOutputScaling(leftEffort, leftCorrection, invertLeft);
    lastRightEffort = applyOutputScaling(rightEffort, rightCorrection, invertRight);

    if (!attached) {
        return;
    }

    leftEsc.writeMicroseconds(effortToPulseUs(lastLeftEffort));
    rightEsc.writeMicroseconds(effortToPulseUs(lastRightEffort));
}

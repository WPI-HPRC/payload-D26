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
    neutralArmCalibrationActive = false;
    leftEsc.detach();
    rightEsc.detach();
    attached = false;
}

void ScrewDriveInterface::beginArm(uint32_t armingDurationMs) {
    armed = false;
    arming = true;
    neutralArmCalibrationActive = false;
    armStartedAt = millis();
    armDurationMs = armingDurationMs;
    stop();
}

void ScrewDriveInterface::beginNeutralArmCalibration(Stream* debugOutput,
                                                     uint16_t stepUs,
                                                     uint16_t minNeutralUs,
                                                     uint16_t maxNeutralUs) {
    neutralArmDebugOutput = debugOutput;
    neutralArmStepUs = max<uint16_t>(1, stepUs);
    neutralArmMinUs = min(minNeutralUs, maxNeutralUs);
    neutralArmMaxUs = max(minNeutralUs, maxNeutralUs);

    armed = false;
    arming = true;
    neutralArmCalibrationActive = true;
    setNeutralPulseConstrained(neutralPulseUs);
    stop();

    printNeutralArmCalibrationHelp();
    printNeutralArmCalibrationPulse();
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

bool ScrewDriveInterface::updateNeutralArmCalibration(const String& input) {
    if (armed) {
        return true;
    }

    if (!neutralArmCalibrationActive) {
        return updateArm();
    }

    stop();

    String command = input;
    command.trim();
    command.toLowerCase();

    if (command.length() == 0) {
        return false;
    }

    if (command == "armed" || command == "arm" || command == "done") {
        neutralArmCalibrationActive = false;
        arming = false;
        armed = true;
        stop();

        if (neutralArmDebugOutput != nullptr) {
            neutralArmDebugOutput->print("ESC neutral calibration accepted. Armed at ");
            neutralArmDebugOutput->print(neutralPulseUs);
            neutralArmDebugOutput->println(" us.");
        }

        return true;
    }

    if (command == "status" || command == "?") {
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "help") {
        printNeutralArmCalibrationHelp();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "+" || command == "inc" || command == "increase") {
        setNeutralPulseConstrained(neutralPulseUs + neutralArmStepUs);
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "++") {
        setNeutralPulseConstrained(neutralPulseUs + (neutralArmStepUs * 10));
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "-" || command == "dec" || command == "decrease") {
        setNeutralPulseConstrained(neutralPulseUs - min(neutralPulseUs, neutralArmStepUs));
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "--") {
        uint16_t delta = min<uint16_t>(neutralPulseUs, neutralArmStepUs * 10);
        setNeutralPulseConstrained(neutralPulseUs - delta);
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command.startsWith("neutral ") || command.startsWith("n ")) {
        int spaceIndex = command.indexOf(' ');
        int pulse = command.substring(spaceIndex + 1).toInt();
        if (pulse > 0) {
            setNeutralPulseConstrained(static_cast<uint16_t>(pulse));
            stop();
            printNeutralArmCalibrationPulse();
        }
        return false;
    }

    if (command.startsWith("step ")) {
        int step = command.substring(command.indexOf(' ') + 1).toInt();
        if (step > 0) {
            neutralArmStepUs = static_cast<uint16_t>(step);
            if (neutralArmDebugOutput != nullptr) {
                neutralArmDebugOutput->print("ESC neutral calibration step: ");
                neutralArmDebugOutput->print(neutralArmStepUs);
                neutralArmDebugOutput->println(" us.");
            }
        }
        return false;
    }

    if (neutralArmDebugOutput != nullptr) {
        neutralArmDebugOutput->print("Unknown ESC neutral calibration command: ");
        neutralArmDebugOutput->println(command);
        printNeutralArmCalibrationHelp();
    }

    return false;
}

bool ScrewDriveInterface::isArmed() const {
    return armed;
}

bool ScrewDriveInterface::isNeutralArmCalibrationActive() const {
    return neutralArmCalibrationActive;
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

uint16_t ScrewDriveInterface::getLastLeftPulseUs() const {
    return lastLeftPulseUs;
}

uint16_t ScrewDriveInterface::getLastRightPulseUs() const {
    return lastRightPulseUs;
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
    lastLeftPulseUs = effortToPulseUs(lastLeftEffort);
    lastRightPulseUs = effortToPulseUs(lastRightEffort);

    if (!attached) {
        return;
    }

    leftEsc.writeMicroseconds(lastLeftPulseUs);
    rightEsc.writeMicroseconds(lastRightPulseUs);
}

void ScrewDriveInterface::setNeutralPulseConstrained(uint16_t pulseUs) {
    neutralPulseUs = constrain(pulseUs, neutralArmMinUs, neutralArmMaxUs);
}

void ScrewDriveInterface::printNeutralArmCalibrationHelp() const {
    if (neutralArmDebugOutput == nullptr) {
        return;
    }

    neutralArmDebugOutput->println("ESC neutral calibration active.");
    neutralArmDebugOutput->println("Commands: +, -, ++, --, neutral <us>, n <us>, step <us>, status, armed.");
}

void ScrewDriveInterface::printNeutralArmCalibrationPulse() const {
    if (neutralArmDebugOutput == nullptr) {
        return;
    }

    neutralArmDebugOutput->print("ESC neutral calibration pulse: ");
    neutralArmDebugOutput->print(neutralPulseUs);
    neutralArmDebugOutput->println(" us.");
}

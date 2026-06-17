#include "ScrewDriveInterface.h"

void ScrewDriveInterface::attach(int leftPin, int rightPin) {
    leftSignalPin = leftPin;
    rightSignalPin = rightPin;

    if (outputMode == SCREW_DRIVE_DIRECT_SERVO_PWM) {
        leftEsc.attach(leftSignalPin);
        rightEsc.attach(rightSignalPin);
    } else {
        leftEsc.detach();
        rightEsc.detach();
        forceNextOpenMVUartWrite();
    }

    attached = true;
    armed = false;
    arming = false;
    stop();
}

void ScrewDriveInterface::useOpenMVUartOutput(Stream* output, uint32_t keepaliveMs) {
    outputMode = SCREW_DRIVE_OPENMV_UART_PWM;
    openMVUartOutput = output;
    openMVUartKeepaliveMs = keepaliveMs;
    hasSentOpenMVUartCommand = false;
    forceNextOpenMVUartWrite();

    if (attached) {
        leftEsc.detach();
        rightEsc.detach();
        stop();
    }
}

void ScrewDriveInterface::useDirectServoOutput() {
    outputMode = SCREW_DRIVE_DIRECT_SERVO_PWM;
    openMVUartOutput = nullptr;
    hasSentOpenMVUartCommand = false;
    forceOpenMVUartWrite = false;

    if (attached && leftSignalPin >= 0 && rightSignalPin >= 0) {
        leftEsc.attach(leftSignalPin);
        rightEsc.attach(rightSignalPin);
        stop();
    }
}

bool ScrewDriveInterface::isUsingOpenMVUartOutput() const {
    return outputMode == SCREW_DRIVE_OPENMV_UART_PWM;
}

void ScrewDriveInterface::detach() {
    armed = false;
    arming = false;
    neutralArmCalibrationActive = false;
    leftEsc.detach();
    rightEsc.detach();
    attached = false;
    hasSentOpenMVUartCommand = false;
}

void ScrewDriveInterface::beginArm(uint32_t armingDurationMs) {
    armed = false;
    arming = true;
    neutralArmCalibrationActive = false;
    armStartedAt = millis();
    armDurationMs = armingDurationMs;
    forceNextOpenMVUartWrite();
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
    forceNextOpenMVUartWrite();
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
        forceNextOpenMVUartWrite();
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
        forceNextOpenMVUartWrite();
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "++") {
        setNeutralPulseConstrained(neutralPulseUs + (neutralArmStepUs * 10));
        forceNextOpenMVUartWrite();
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "-" || command == "dec" || command == "decrease") {
        setNeutralPulseConstrained(neutralPulseUs - min(neutralPulseUs, neutralArmStepUs));
        forceNextOpenMVUartWrite();
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command == "--") {
        uint16_t delta = min<uint16_t>(neutralPulseUs, neutralArmStepUs * 10);
        setNeutralPulseConstrained(neutralPulseUs - delta);
        forceNextOpenMVUartWrite();
        stop();
        printNeutralArmCalibrationPulse();
        return false;
    }

    if (command.startsWith("neutral ") || command.startsWith("n ")) {
        int spaceIndex = command.indexOf(' ');
        int pulse = command.substring(spaceIndex + 1).toInt();
        if (pulse > 0) {
            setNeutralPulseConstrained(static_cast<uint16_t>(pulse));
            forceNextOpenMVUartWrite();
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

    float leftMagnitude = leftEffort < 0.0f ? -leftEffort : leftEffort;
    float rightMagnitude = rightEffort < 0.0f ? -rightEffort : rightEffort;
    float maxMagnitude = max(leftMagnitude, rightMagnitude);
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

    if (outputMode == SCREW_DRIVE_OPENMV_UART_PWM) {
        writeOpenMVUartCommand();
        return;
    }

    leftEsc.writeMicroseconds(lastLeftPulseUs);
    rightEsc.writeMicroseconds(lastRightPulseUs);
}

void ScrewDriveInterface::writeOpenMVUartCommand() {
    if (openMVUartOutput == nullptr) {
        return;
    }

    const uint32_t now = millis();
    const bool pulsesChanged = !hasSentOpenMVUartCommand ||
                               lastLeftPulseUs != lastOpenMVUartLeftPulseUs ||
                               lastRightPulseUs != lastOpenMVUartRightPulseUs;
    const bool keepaliveElapsed = hasSentOpenMVUartCommand &&
                                  openMVUartKeepaliveMs > 0 &&
                                  now - lastOpenMVUartCommandAt >= openMVUartKeepaliveMs;

    if (!forceOpenMVUartWrite && !pulsesChanged && !keepaliveElapsed) {
        return;
    }

    openMVUartOutput->print("SERVO L ");
    openMVUartOutput->print(lastLeftPulseUs);
    openMVUartOutput->print(" R ");
    openMVUartOutput->println(lastRightPulseUs);

    lastOpenMVUartLeftPulseUs = lastLeftPulseUs;
    lastOpenMVUartRightPulseUs = lastRightPulseUs;
    lastOpenMVUartCommandAt = now;
    hasSentOpenMVUartCommand = true;
    forceOpenMVUartWrite = false;
}

void ScrewDriveInterface::forceNextOpenMVUartWrite() {
    forceOpenMVUartWrite = true;
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

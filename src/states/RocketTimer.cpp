#include "../State.h"

#include <math.h>
#include <stdlib.h>

static constexpr float LAUNCH_ACCEL_THRESHOLD_RAW = 15.0f;
static constexpr float LAUNCH_ACCEL_THRESHOLD_RAW_SQUARED =
    LAUNCH_ACCEL_THRESHOLD_RAW * LAUNCH_ACCEL_THRESHOLD_RAW;
static constexpr unsigned long long ROCKET_FLIGHT_DURATION_MS = 15*60*1000; // 15 minutes, the time from launch to when the rover should be fully submerged based on simulations and test launches
static constexpr unsigned long long SENSOR_DEBUG_INTERVAL_MS = 200;
static constexpr unsigned long long LAUNCH_ARM_DELAY_MS = 2000;
static constexpr uint8_t LAUNCH_CONFIRMATION_SAMPLE_COUNT = 5;

struct RocketTimerLocalData {
    bool launchDetected;
    unsigned long long launchTime;
    unsigned long long lastSensorDebugTime;
    uint32_t lastCheckedAsm330UpdateTime;
    uint8_t consecutiveLaunchSamples;
};

static String normalizedStateCommand(String input) {
    input.trim();
    input.toLowerCase();
    input.replace("-", "_");
    input.replace(" ", "_");
    return input;
}

static StateID stateFromSerialCommand(String input, StateID currentState, bool &recognized) {
    input = normalizedStateCommand(input);
    recognized = true;

    switch (input.charAt(0)) {
    case 'r':
        if (input == "rocket_timer") {
            return ROCKET_TIMER;
        }
        if (input == "rov") {
            return PAYLOAD_ROV;
        }
        break;
    case 's':
        if (input == "self_righting") {
            return PAYLOAD_SELF_RIGHTING;
        }
        break;
    case 'l':
        if (input == "latch_releasing") {
            return PAYLOAD_LATCH_RELEASING;
        }
        break;
    case 'd':
        if (input == "deploying") {
            return PAYLOAD_DEPLOYING;
        }
        if (input == "deployed") {
            return PAYLOAD_DEPLOYED;
        }
        break;
    case 'c':
        if (input == "connecting") {
            return PAYLOAD_CONNECTING;
        }
        if (input == "convention_demo") {
            return CONVENTION_DEMO;
        }
        break;
    case 'a':
        if (input == "autonomous") {
            return PAYLOAD_AUTONOMOUS;
        }
        break;
    case 'i':
        if (input == "idle") {
            return PAYLOAD_IDLE;
        }
        break;
    case 'p':
        if (input == "payload_self_righting") {
            return PAYLOAD_SELF_RIGHTING;
        }
        if (input == "payload_latch_releasing") {
            return PAYLOAD_LATCH_RELEASING;
        }
        if (input == "payload_deploying") {
            return PAYLOAD_DEPLOYING;
        }
        if (input == "payload_deployed") {
            return PAYLOAD_DEPLOYED;
        }
        if (input == "payload_connecting") {
            return PAYLOAD_CONNECTING;
        }
        if (input == "payload_rov") {
            return PAYLOAD_ROV;
        }
        if (input == "payload_autonomous") {
            return PAYLOAD_AUTONOMOUS;
        }
        if (input == "payload_idle") {
            return PAYLOAD_IDLE;
        }
        break;
    default:
        break;
    }

    recognized = false;
    return currentState;
}

static StateID checkSerialStateOverride(StateID currentState) {
    if (!Serial.available()) {
        return currentState;
    }

    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) {
        return currentState;
    }

    bool recognized = false;
    StateID requestedState = stateFromSerialCommand(input, currentState, recognized);
    if (requestedState != currentState) {
        Serial.print("Rocket timer serial override to state: ");
        Serial.println(input);
    } else if (!recognized) {
        Serial.print("Unknown rocket timer state command: ");
        Serial.println(input);
    }

    return requestedState;
}

static bool getAsm330AccelMagnitude(Context *ctx, float &accelMagnitude,
                                    float &accelMagnitudeSquared,
                                    uint32_t &lastUpdated) {
    const auto &asm330_desc = ctx->asm330.get_descriptor();
    lastUpdated = asm330_desc.getLastUpdated();
    if (lastUpdated == 0) {
        return false;
    }

    float accelX = asm330_desc.data.accel0;
    float accelY = asm330_desc.data.accel1;
    float accelZ = asm330_desc.data.accel2;
    accelMagnitudeSquared = accelX * accelX + accelY * accelY + accelZ * accelZ;
    accelMagnitude = sqrtf(accelMagnitudeSquared);

    return true;
}

static bool launchAccelThresholdExceeded(StateData const *data, Context *ctx,
                                         RocketTimerLocalData *localData,
                                         float &accelMagnitude,
                                         float &accelMagnitudeSquared,
                                         uint32_t &lastUpdated) {
    if (!getAsm330AccelMagnitude(ctx, accelMagnitude, accelMagnitudeSquared,
                                 lastUpdated)) {
        return false;
    }

    if (lastUpdated == localData->lastCheckedAsm330UpdateTime) {
        return false;
    }

    localData->lastCheckedAsm330UpdateTime = lastUpdated;
    bool aboveThreshold = accelMagnitude > LAUNCH_ACCEL_THRESHOLD_RAW;
    if (data->currentTime < LAUNCH_ARM_DELAY_MS || !aboveThreshold) {
        localData->consecutiveLaunchSamples = 0;
        return false;
    }

    if (localData->consecutiveLaunchSamples < LAUNCH_CONFIRMATION_SAMPLE_COUNT) {
        localData->consecutiveLaunchSamples++;
    }

    return localData->consecutiveLaunchSamples >= LAUNCH_CONFIRMATION_SAMPLE_COUNT;
}

static void printTeleplotValue(const char *name, float value) {
    Serial.print(">");
    Serial.print(name);
    Serial.print(":");
    Serial.println(value);
}

static void printTeleplotValue(const char *name, unsigned long long value) {
    Serial.print(">");
    Serial.print(name);
    Serial.print(":");
    Serial.println(value);
}

static void printSensorTeleplot(StateData const *data, Context *ctx, RocketTimerLocalData *localData) {
    if (data->currentTime - localData->lastSensorDebugTime < SENSOR_DEBUG_INTERVAL_MS) {
        return;
    }

    localData->lastSensorDebugTime = data->currentTime;

    const auto &asm330_desc = ctx->asm330.get_descriptor();
    float accelX = asm330_desc.data.accel0;
    float accelY = asm330_desc.data.accel1;
    float accelZ = asm330_desc.data.accel2;
    float accelMagnitudeSquared =
        accelX * accelX + accelY * accelY + accelZ * accelZ;
    float accelMagnitude = sqrtf(accelMagnitudeSquared);
    float launchCandidate = accelMagnitude > LAUNCH_ACCEL_THRESHOLD_RAW ? 1.0f : 0.0f;
    float launchArmed = data->currentTime >= LAUNCH_ARM_DELAY_MS ? 1.0f : 0.0f;
    unsigned long long timerElapsed =
        localData->launchDetected ? data->currentTime - localData->launchTime : 0;

    printTeleplotValue("rocket_timer_time_ms", static_cast<unsigned long long>(data->currentTime));
    printTeleplotValue("rocket_timer_asm330_updated_ms", static_cast<unsigned long long>(asm330_desc.getLastUpdated()));
    printTeleplotValue("rocket_timer_accel_x", accelX);
    printTeleplotValue("rocket_timer_accel_y", accelY);
    printTeleplotValue("rocket_timer_accel_z", accelZ);
    printTeleplotValue("rocket_timer_accel_mag", accelMagnitude);
    printTeleplotValue("rocket_timer_accel_mag_sq", accelMagnitudeSquared);
    printTeleplotValue("rocket_timer_launch_threshold", LAUNCH_ACCEL_THRESHOLD_RAW);
    printTeleplotValue("rocket_timer_launch_threshold_sq", LAUNCH_ACCEL_THRESHOLD_RAW_SQUARED);
    printTeleplotValue("rocket_timer_launch_candidate", launchCandidate);
    printTeleplotValue("rocket_timer_launch_armed", launchArmed);
    printTeleplotValue("rocket_timer_launch_consecutive_samples",
                       static_cast<unsigned long long>(localData->consecutiveLaunchSamples));
    printTeleplotValue("rocket_timer_launch_required_samples",
                       static_cast<unsigned long long>(LAUNCH_CONFIRMATION_SAMPLE_COUNT));
    printTeleplotValue("rocket_timer_launch_detected", localData->launchDetected ? 1.0f : 0.0f);
    printTeleplotValue("rocket_timer_elapsed_ms", timerElapsed);
}

static void printLaunchTriggerTeleplot(StateData const *data, float accelMagnitude,
                                       float accelMagnitudeSquared,
                                       uint32_t asm330UpdatedAt) {
    printTeleplotValue("rocket_timer_launch_event", 1.0f);
    printTeleplotValue("rocket_timer_launch_time_ms",
                       static_cast<unsigned long long>(data->currentTime));
    printTeleplotValue("rocket_timer_launch_asm330_updated_ms",
                       static_cast<unsigned long long>(asm330UpdatedAt));
    printTeleplotValue("rocket_timer_launch_accel_mag", accelMagnitude);
    printTeleplotValue("rocket_timer_launch_accel_mag_sq", accelMagnitudeSquared);
    printTeleplotValue("rocket_timer_launch_threshold", LAUNCH_ACCEL_THRESHOLD_RAW);
    printTeleplotValue("rocket_timer_launch_detected", 1.0f);
}

void *rocketTimerInit(StateData const *data) {
    Serial.println("Entered Rocket Timer State...");

    RocketTimerLocalData *localData =
        static_cast<RocketTimerLocalData *>(malloc(sizeof(RocketTimerLocalData)));

    if (localData == nullptr) {
        Serial.println("Rocket Timer failed to allocate local data.");
        return nullptr;
    }

    localData->launchDetected = false;
    localData->launchTime = 0;
    localData->lastSensorDebugTime = 0;
    localData->lastCheckedAsm330UpdateTime = 0;
    localData->consecutiveLaunchSamples = 0;
    return localData;
}

StateID rocketTimerLoop(StateData const *data, Context *ctx, void *_localData) {
    StateID serialOverride = checkSerialStateOverride(ROCKET_TIMER);
    if (serialOverride != ROCKET_TIMER) {
        return serialOverride;
    }

    RocketTimerLocalData *localData =
        static_cast<RocketTimerLocalData *>(_localData);

    if (localData == nullptr) {
        return ROCKET_TIMER;
    }

    printSensorTeleplot(data, ctx, localData);

    float launchAccelMagnitude = 0.0f;
    float launchAccelMagnitudeSquared = 0.0f;
    uint32_t launchAsm330UpdatedAt = 0;
    if (!localData->launchDetected &&
        launchAccelThresholdExceeded(data, ctx, localData, launchAccelMagnitude,
                                     launchAccelMagnitudeSquared,
                                     launchAsm330UpdatedAt)) {
        localData->launchDetected = true;
        localData->launchTime = data->currentTime;
        printLaunchTriggerTeleplot(data, launchAccelMagnitude,
                                   launchAccelMagnitudeSquared,
                                   launchAsm330UpdatedAt);
        Serial.println("Launch detected. Rocket flight timer started.");
    }

    if (localData->launchDetected &&
        data->currentTime - localData->launchTime >= ROCKET_FLIGHT_DURATION_MS) {
        Serial.println("Rocket flight timer complete. Entering self-righting.");
        return PAYLOAD_SELF_RIGHTING;
    }

    return ROCKET_TIMER;
}

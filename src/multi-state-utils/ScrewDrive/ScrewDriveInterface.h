#pragma once

#include <Arduino.h>
#include <Servo.h>

enum DriveControlStrategy {
    ARCADE
};

class ScrewDriveInterface {
  public:
    /**
     * Neutral ESC pulse width in microseconds. For bidirectional ESC setups this
     * should represent zero motor effort.
     */
    uint16_t neutralPulseUs = 1500;

    /**
     * Minimum ESC pulse width in microseconds. This is the full reverse command
     * limit used after effort scaling and corrections.
     */
    uint16_t minPulseUs = 1000;

    /**
     * Maximum ESC pulse width in microseconds. This is the full forward command
     * limit used after effort scaling and corrections.
     */
    uint16_t maxPulseUs = 2000;

    /**
     * Maximum normalized effort applied to either screw after drive mixing.
     * Keep this below 1.0 to limit motor speed during early testing.
     */
    float maxEffort = 0.25f;

    /**
     * Turn contribution used by the ARCADE drive strategy. Lower values produce
     * wider turns and reduce the chance of counter-rotating the screws.
     */
    float turnScale = 0.4f;

    /**
     * Attach the left and right ESC signal pins and immediately command neutral.
     */
    void attach(int leftPin, int rightPin);

    /**
     * Detach both ESC Servo outputs. Call stop() before detach() when possible.
     */
    void detach();

    /**
     * Start a non-blocking arming period by commanding neutral and recording
     * the current time. Call updateArm() from loop code until it returns true.
     */
    void beginArm(uint32_t armingDurationMs = 1000);

    /**
     * Continue the non-blocking arming period. This keeps commanding neutral and
     * returns true once the configured arming time has elapsed.
     */
    bool updateArm();

    /**
     * Return true once the most recent non-blocking arming period has completed.
     */
    bool isArmed() const;

    /**
     * Command neutral effort to both ESCs immediately.
     */
    void stop();

    /**
     * Drive the screws with normalized inputs in the range [-1.0, 1.0].
     * In ARCADE mode, speed is forward/reverse effort and turn is differential
     * steering effort.
     */
    void drive(float speed, float turn);

    /**
     * Select the drive mixing strategy. Only ARCADE is currently implemented.
     */
    void setDriveStrategy(DriveControlStrategy strategy);

    /**
     * Set the maximum normalized motor effort applied after drive mixing.
     * Values are clamped to [0.0, 1.0].
     */
    void setMaxEffort(float effort);

    /**
     * Set per-side effort correction factors. Use this to compensate for motor,
     * ESC, screw, or friction mismatch so equal commands drive straight.
     */
    void setMotorCorrections(float leftScale, float rightScale);

    /**
     * Set per-side inversions. Inversion flips effort around neutral without
     * changing the other side.
     */
    void setMotorInversions(bool leftInverted, bool rightInverted);

    /**
     * Return the last normalized effort written to the left ESC after scaling,
     * correction, and inversion.
     */
    float getLastLeftEffort() const;

    /**
     * Return the last normalized effort written to the right ESC after scaling,
     * correction, and inversion.
     */
    float getLastRightEffort() const;

  private:
    Servo leftEsc;
    Servo rightEsc;

    DriveControlStrategy driveStrategy = ARCADE;
    float leftCorrection = 1.0f;
    float rightCorrection = 1.0f;
    bool invertLeft = false;
    bool invertRight = false;
    float lastLeftEffort = 0.0f;
    float lastRightEffort = 0.0f;
    bool attached = false;
    bool armed = false;
    bool arming = false;
    uint32_t armStartedAt = 0;
    uint32_t armDurationMs = 0;

    float clampUnit(float value) const;
    float applyOutputScaling(float effort, float correction, bool inverted) const;
    uint16_t effortToPulseUs(float effort) const;
    void writeEfforts(float leftEffort, float rightEffort);
};

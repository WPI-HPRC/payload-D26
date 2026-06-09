#pragma once

#include <Arduino.h>

class AntennaConnectorInterface {
  public:
    struct SensorData {
        float leftScrewCurrent = 0.0f;
        float rightScrewCurrent = 0.0f;
        float batteryVoltage = 0.0f;

        /**
         * Set one rover-to-antenna sensor value by parameter name.
         * Supported names: leftScrewCurrent, rightScrewCurrent, batteryVoltage.
         */
        bool setValue(const String &parameterName, float value);

        /**
         * Read one rover-to-antenna sensor value by parameter name.
         * Returns false if the parameter name is not supported.
         */
        bool getValue(const String &parameterName, float &value) const;

        /**
         * Return all rover-to-antenna sensor values as a compact JSON object.
         */
        String getAllJson() const;
    };

    struct DriveData {
        float speed = 0.0f;
        float turn = 0.0f;

        /**
         * Set antenna-to-rover drive inputs. Values are normalized to [-1.0, 1.0].
         */
        void set(float speedInput, float turnInput);

        /**
         * Read the current antenna-to-rover drive inputs.
         */
        void get(float &speedOutput, float &turnOutput) const;
    };

    SensorData sensorData;
    DriveData driveData;

    /**
     * Minimum amount of time, in milliseconds, that hasConnection() must stay true
     * before onConnectionGained() confirms the connection.
     */
    uint32_t connectionHeldTime = 500;

    /**
     * Minimum amount of time, in milliseconds, that hasConnection() must stay false
     * before onConnectionLost() confirms the connection has been lost.
     */
    uint32_t connectionLostTime = 500;

    /**
     * Raw antenna connection input.
     *
     * This is intentionally a placeholder until the antenna hardware team wires in
     * the real implementation. For now it returns true only while a simulated
     * connection started with simulateConnectionFor() is still active.
     */
    bool hasConnection();

    /**
     * Polls the raw connection input and returns true exactly once when the
     * connection has remained present for connectionHeldTime.
     *
     * Call this repeatedly from state loop code when waiting for a stable
     * antenna connection. This method advances the internal denoising state.
     */
    bool onConnectionGained();

    /**
     * Polls the raw connection input and returns true exactly once when a
     * previously confirmed connection has remained absent for connectionLostTime.
     *
     * Call this repeatedly from state loop code when a state needs to react to a
     * stable antenna disconnect. This method advances the internal denoising state.
     */
    bool onConnectionLost();

    /**
     * Returns the last confirmed denoised connection state without polling
     * hasConnection() or advancing any timing logic.
     *
     * Use this only for passive status checks. Use onConnectionGained() or
     * onConnectionLost() when the caller needs to detect transition events.
     */
    bool isConnectionConfirmed();

    /**
     * Test hook that makes hasConnection() return true for durationMs milliseconds.
     *
     * This does not immediately mark the connection confirmed. The caller still
     * needs to poll onConnectionGained() until connectionHeldTime has elapsed.
     */
    void simulateConnectionFor(uint32_t durationMs);

    /**
     * Stops any active simulated connection. This only affects the raw simulated
     * input; connection loss still needs to be confirmed through onConnectionLost().
     */
    void clearSimulatedConnection();

    /**
     * Clears raw input tracking, confirmed connection state, and any active
     * simulated connection.
     */
    void reset();

    /**
     * Set one rover-to-antenna sensor value by parameter name.
     * Supported names: leftScrewCurrent, rightScrewCurrent, batteryVoltage.
     */
    bool setSensorValue(const String &parameterName, float value);

    /**
     * Read one rover-to-antenna sensor value by parameter name.
     * Returns false if the parameter name is not supported.
     */
    bool getSensorValue(const String &parameterName, float &value) const;

    /**
     * Return all rover-to-antenna sensor values as a compact JSON object.
     */
    String getAllSensorDataJson() const;

    /**
     * Set antenna-to-rover drive inputs. Values are normalized to [-1.0, 1.0].
     */
    void setDriveData(float speed, float turn);

    /**
     * Read the current antenna-to-rover drive inputs.
     */
    DriveData getDriveData() const;

  private:
    bool rawConnection = false;
    bool confirmedConnection = false;
    uint32_t rawConnectionChangedAt = 0;

    bool simulatedConnectionEnabled = false;
    uint32_t simulatedConnectionStartedAt = 0;
    uint32_t simulatedConnectionDuration = 0;

    void updateRawConnection(bool currentConnection, uint32_t now);
};

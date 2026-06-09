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
     * This reports the latest connection status published by the active
     * transmitter. A simulated connection started with simulateConnectionFor()
     * acts as a test override.
     */
    bool hasConnection();

    /**
     * Publish the latest transmitter-checked connection status.
     *
     * Transmitter implementations call this after checking their own hardware
     * or serial link. State code should read hasConnection(),
     * onConnectionGained(), or onConnectionLost() instead of setting this
     * directly.
     */
    void setTransmitterConnectionStatus(bool connected);

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

    /**
     * Gives antenna/transmitter-side code a stable pointer to the pending image.
     *
     * The returned pointer references private storage owned by this interface.
     * Once this method succeeds, canAcceptImages() returns false until
     * antennaReadyForImageData() is called, so receiver-side queue wraparound
     * cannot overwrite the image while it is being transmitted.
     */
    bool accessImageData(const String*& base64Data, int& byteCount);

    /**
     * Copy-based compatibility accessor for pending image data.
     *
     * Prefer the pointer overload for transmitters so large image data is not
     * copied before chunking.
     */
    bool accessImageData(String& base64Data, int& byteCount);

    /**
     * Mark the currently accessed image as fully transmitted and clear the
     * private pending-image slot.
     */
    void antennaReadyForImageData();

    /**
     * Returns true when receiver-side code may hand this interface a new image.
     *
     * This is false while an image is pending or actively being transmitted.
     */
    bool canAcceptImages();

    /**
     * Backward-compatible spelling for existing receiver code.
     */
    bool canAcceptImageData();

    /**
     * Copy a newly received image into private interface storage.
     *
     * Returns false if an image is already pending or being transmitted.
     */
    bool intakeImageData(const String& base64Data, int byteCount);

  private:
    bool rawConnection = false;
    bool confirmedConnection = false;
    uint32_t rawConnectionChangedAt = 0;

    bool simulatedConnectionEnabled = false;
    uint32_t simulatedConnectionStartedAt = 0;
    uint32_t simulatedConnectionDuration = 0;
    bool transmitterConnectionStatus = false;

    String pendingImageData = "";
    int pendingImageByteCount = 0;
    bool pendingImageAvailable = false;
    bool pendingImageLocked = false;

    void updateRawConnection(bool currentConnection, uint32_t now);
};

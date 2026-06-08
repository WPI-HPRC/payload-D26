#pragma once

#include <Arduino.h>

class AntennaConnectorInterface {
  public:
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

  private:
    bool rawConnection = false;
    bool confirmedConnection = false;
    uint32_t rawConnectionChangedAt = 0;

    bool simulatedConnectionEnabled = false;
    uint32_t simulatedConnectionStartedAt = 0;
    uint32_t simulatedConnectionDuration = 0;

    void updateRawConnection(bool currentConnection, uint32_t now);
};

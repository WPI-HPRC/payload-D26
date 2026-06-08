#include "AntennaConnectorInterface.h"

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

void AntennaConnectorInterface::updateRawConnection(bool currentConnection, uint32_t now) {
    if (currentConnection == rawConnection) {
        return;
    }

    rawConnection = currentConnection;
    rawConnectionChangedAt = now;
}

#pragma once

#include <Arduino.h>
#include "AntennaConnectorInterface.h"

class AntennaSerialTransmitter {
  public:
    /**
     * Create a non-blocking serial transmitter for antenna-bound rover data.
     *
     * outputStream is the serial-like stream to write JSON packets to.
     * antennaConnector is the shared data interface that owns sensor values,
     * image storage, and raw connection status.
     */
    AntennaSerialTransmitter(
        Stream* outputStream = nullptr,
        AntennaConnectorInterface* antennaConnector = nullptr
    );

    /**
     * Set or replace the serial-like output stream used for JSON packets.
     */
    void setOutputStream(Stream* outputStream);

    /**
     * Set or replace the antenna connector interface used for data access.
     */
    void setAntennaConnector(AntennaConnectorInterface* antennaConnector);

    /**
     * Run one non-blocking transmitter step.
     *
     * Returns true exactly when an image transfer has completed. Each call sends
     * at most one JSON packet. The transmitter checks its own connection status
     * and publishes that result to the antenna connector before sending.
     */
    bool runTransmitter();

    /**
     * Check this transmitter's current connection status and publish it to the
     * antenna connector without sending telemetry or image data.
     */
    bool updateConnectionStatus();

    /**
     * Parse one incoming serial JSON packet from the external application.
     *
     * Currently supports drive commands in the form:
     * {"type":"driveCommand","strategy":"arcade","values":[speed,turn]}
     * Returns true when the packet was recognized and applied.
     */
    bool handleIncomingPacket(const String& packet);

    /**
     * Return the latest connection status checked by this transmitter.
     */
    bool isConnected() const;

    /**
     * Set the maximum base64 characters sent in one JSON image packet.
     */
    void setImageChunkSize(size_t chunkSize);

    /**
     * Set the minimum time between JSON packets in milliseconds.
     */
    void setTransmitInterval(uint32_t intervalMs);

    /**
     * Returns true while an image is being chunked and transmitted.
     */
    bool isBusy() const;

    /**
     * Cancel the current transfer without clearing the antenna connector image.
     *
     * The image remains available to be accessed again by a later transfer.
     */
    void resetTransfer();

  private:
    enum Status {
        IDLE,
        SENDING_IMAGE
    };

    Stream* outputStream = nullptr;
    AntennaConnectorInterface* antennaConnector = nullptr;

    Status status = IDLE;
    const String* activeImageData = nullptr;
    int activeImageByteCount = 0;
    size_t activeImageIndex = 0;
    size_t activeChunkIndex = 0;
    uint32_t lastTransmitAt = 0;
    bool connectedStatus = false;
    String outgoingPacket = "";
    size_t outgoingPacketIndex = 0;

    size_t imageChunkSize = 64;
    uint32_t transmitIntervalMs = 5;

    bool beginImageTransfer();
    bool sendNextImagePacket();
    bool sendTelemetryOnlyPacket();
    bool writePacket(const String& packet);
    bool flushOutgoingPacket();
    size_t writableByteCount() const;
    bool checkConnectionStatus() const;
    bool parseDriveCommand(const String& packet, float& speed, float& turn) const;
    void finishImageTransfer();
    void printDebugThrottled(const char* message);

    String buildPacket(const String* imageChunk, bool finalChunk);
    void appendSensorJson(String& json) const;
};

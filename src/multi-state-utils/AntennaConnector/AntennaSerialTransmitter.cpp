#include "AntennaSerialTransmitter.h"

AntennaSerialTransmitter::AntennaSerialTransmitter(
    Stream* outputStream,
    AntennaConnectorInterface* antennaConnector
) :
    outputStream(outputStream),
    antennaConnector(antennaConnector)
{
}

void AntennaSerialTransmitter::setOutputStream(Stream* outputStream) {
    this->outputStream = outputStream;
}

void AntennaSerialTransmitter::setAntennaConnector(AntennaConnectorInterface* antennaConnector) {
    this->antennaConnector = antennaConnector;
    resetTransfer();
}

bool AntennaSerialTransmitter::runTransmitter() {
    if (!updateConnectionStatus()) {
        printDebugThrottled("DBG_ANT_TX_NO_CONNECTOR");
        return false;
    }

    if (outputStream == nullptr) {
        printDebugThrottled("DBG_ANT_TX_NO_OUTPUT_STREAM");
        return false;
    }

    if (!connectedStatus) {
        printDebugThrottled("DBG_ANT_TX_NOT_CONNECTED");
        return false;
    }

    uint32_t now = millis();
    if (now - lastTransmitAt < transmitIntervalMs) {
        return false;
    }

    if (status == IDLE) {
        if (beginImageTransfer()) {
            return sendNextImagePacket();
        }

        return sendTelemetryOnlyPacket();
    }

    return sendNextImagePacket();
}

bool AntennaSerialTransmitter::updateConnectionStatus() {
    if (antennaConnector == nullptr) {
        connectedStatus = false;
        return false;
    }

    connectedStatus = checkConnectionStatus();
    antennaConnector->setTransmitterConnectionStatus(connectedStatus);

    return true;
}

bool AntennaSerialTransmitter::handleIncomingPacket(const String& packet) {
    if (antennaConnector == nullptr) {
        printDebugThrottled("DBG_ANT_RX_NO_CONNECTOR");
        return false;
    }

    float speed = 0.0f;
    float turn = 0.0f;

    if (!parseDriveCommand(packet, speed, turn)) {
        Serial.println("{\"type\":\"debug\",\"source\":\"antennaRx\",\"event\":\"unrecognizedJson\"}");
        return false;
    }

    antennaConnector->setDriveData(speed, turn);

    Serial.print("{\"type\":\"debug\",\"source\":\"antennaRx\",\"event\":\"driveCommand\",\"speed\":");
    Serial.print(speed, 3);
    Serial.print(",\"turn\":");
    Serial.print(turn, 3);
    Serial.println("}");

    return true;
}

void AntennaSerialTransmitter::setImageChunkSize(size_t chunkSize) {
    if (chunkSize == 0) {
        return;
    }

    imageChunkSize = chunkSize;
}

void AntennaSerialTransmitter::setTransmitInterval(uint32_t intervalMs) {
    transmitIntervalMs = intervalMs;
}

bool AntennaSerialTransmitter::isBusy() const {
    return status != IDLE;
}

bool AntennaSerialTransmitter::isConnected() const {
    return connectedStatus;
}

void AntennaSerialTransmitter::resetTransfer() {
    status = IDLE;
    activeImageData = nullptr;
    activeImageByteCount = 0;
    activeImageIndex = 0;
    activeChunkIndex = 0;
    outgoingPacket = "";
    outgoingPacketIndex = 0;
}

bool AntennaSerialTransmitter::beginImageTransfer() {
    int byteCount = 0;
    const String* imageData = nullptr;

    if (!antennaConnector->accessImageData(imageData, byteCount) || imageData == nullptr) {
        return false;
    }

    activeImageData = imageData;
    activeImageByteCount = byteCount;
    activeImageIndex = 0;
    activeChunkIndex = 0;
    status = SENDING_IMAGE;

    return true;
}

bool AntennaSerialTransmitter::sendNextImagePacket() {
    if (activeImageData == nullptr) {
        resetTransfer();
        return false;
    }

    size_t imageLength = activeImageData->length();
    size_t remaining = imageLength - activeImageIndex;
    size_t chunkLength = remaining < imageChunkSize ? remaining : imageChunkSize;
    bool finalChunk = activeImageIndex + chunkLength >= imageLength;

    String chunk = activeImageData->substring(activeImageIndex, activeImageIndex + chunkLength);
    String packet = buildPacket(&chunk, finalChunk);

    if (!writePacket(packet)) {
        return false;
    }

    activeImageIndex += chunkLength;
    activeChunkIndex++;

    if (finalChunk) {
        finishImageTransfer();
        return true;
    }

    return false;
}

bool AntennaSerialTransmitter::sendTelemetryOnlyPacket() {
    String packet = buildPacket(nullptr, false);
    return writePacket(packet);
}

bool AntennaSerialTransmitter::writePacket(const String& packet) {
    if (outgoingPacket.length() == 0) {
        outgoingPacket = packet;
        outgoingPacket += "\n";
        outgoingPacketIndex = 0;
    }

    return flushOutgoingPacket();
}

bool AntennaSerialTransmitter::flushOutgoingPacket() {
    if (outputStream == nullptr || outgoingPacket.length() == 0) {
        return false;
    }

    size_t writable = writableByteCount();

    if (writable == 0) {
        printDebugThrottled("DBG_ANT_TX_WAIT_WRITE_SPACE");
        return false;
    }

    size_t remaining = outgoingPacket.length() - outgoingPacketIndex;
    size_t writeLength = remaining < writable ? remaining : writable;

    outputStream->print(outgoingPacket.substring(outgoingPacketIndex, outgoingPacketIndex + writeLength));
    outgoingPacketIndex += writeLength;

    if (outgoingPacketIndex < static_cast<size_t>(outgoingPacket.length())) {
        return false;
    }

    outgoingPacket = "";
    outgoingPacketIndex = 0;
    lastTransmitAt = millis();

    return true;
}

size_t AntennaSerialTransmitter::writableByteCount() const {
    int availableBytes = outputStream->availableForWrite();

    if (availableBytes < 0) {
        return 0;
    }

    if (availableBytes == 0) {
        return 16;
    }

    return static_cast<size_t>(availableBytes);
}

bool AntennaSerialTransmitter::checkConnectionStatus() const {
    return outputStream != nullptr;
}

bool AntennaSerialTransmitter::parseDriveCommand(const String& packet, float& speed, float& turn) const {
    int typeIndex = packet.indexOf("\"type\"");
    int strategyIndex = packet.indexOf("\"strategy\"");
    int valuesIndex = packet.indexOf("\"values\"");

    if (typeIndex < 0 || strategyIndex < 0 || valuesIndex < 0) {
        return false;
    }

    if (packet.indexOf("\"driveCommand\"", typeIndex) < 0) {
        return false;
    }

    if (packet.indexOf("\"arcade\"", strategyIndex) < 0) {
        return false;
    }

    int openBracket = packet.indexOf('[', valuesIndex);
    int comma = packet.indexOf(',', openBracket + 1);
    int closeBracket = packet.indexOf(']', comma + 1);

    if (openBracket < 0 || comma < 0 || closeBracket < 0) {
        return false;
    }

    String speedText = packet.substring(openBracket + 1, comma);
    String turnText = packet.substring(comma + 1, closeBracket);

    speedText.trim();
    turnText.trim();

    if (speedText.length() == 0 || turnText.length() == 0) {
        return false;
    }

    speed = constrain(speedText.toFloat(), -1.0f, 1.0f);
    turn = constrain(turnText.toFloat(), -1.0f, 1.0f);

    return true;
}

void AntennaSerialTransmitter::printDebugThrottled(const char* message) {
    static uint32_t lastDebugPrintAt = 0;
    uint32_t now = millis();

    if (now - lastDebugPrintAt < 1000) {
        return;
    }

    Serial.println(message);
    lastDebugPrintAt = now;
}

void AntennaSerialTransmitter::finishImageTransfer() {
    antennaConnector->antennaReadyForImageData();
    resetTransfer();
}

String AntennaSerialTransmitter::buildPacket(const String* imageChunk, bool finalChunk) {
    String json = "{";
    json += "\"type\":\"roverTelemetry\",";
    json += "\"connection\":";
    json += antennaConnector->hasConnection() ? "true" : "false";
    json += ",\"sensors\":";
    appendSensorJson(json);
    json += ",\"image\":";

    if (imageChunk == nullptr || activeImageData == nullptr) {
        json += "null";
    } else {
        size_t imageLength = activeImageData->length();
        size_t chunkCount = imageLength == 0 ? 1 : ((imageLength + imageChunkSize - 1) / imageChunkSize);

        json += "{";
        json += "\"byteCount\":";
        json += activeImageByteCount;
        json += ",\"base64Length\":";
        json += imageLength;
        json += ",\"chunkIndex\":";
        json += activeChunkIndex;
        json += ",\"chunkCount\":";
        json += chunkCount;
        json += ",\"final\":";
        json += finalChunk ? "true" : "false";
        json += ",\"data\":\"";
        json += *imageChunk;
        json += "\"}";
    }

    json += "}";
    return json;
}

void AntennaSerialTransmitter::appendSensorJson(String& json) const {
    json += "{";
    json += "\"leftScrewCurrent\":";
    json += String(antennaConnector->sensorData.leftScrewCurrent, 3);
    json += ",\"rightScrewCurrent\":";
    json += String(antennaConnector->sensorData.rightScrewCurrent, 3);
    json += ",\"batteryVoltage\":";
    json += String(antennaConnector->sensorData.batteryVoltage, 3);
    json += "}";
}

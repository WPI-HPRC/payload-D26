#include "boardToPCConnector.h"

bool boardToPCConnector::startSendingImageData(
    const String& base64Image,
    size_t decodedBytes
) {
    if (currentStatus != IDLE) {
        return false;
    }

    currentStatus = SENDING;
    outgoingBase64 = &base64Image;
    outgoingDecodedBytes = decodedBytes;
    outgoingIndex = 0;

    Serial.print("IMG_BEGIN ");
    Serial.println(outgoingDecodedBytes);

    return true;
}

bool boardToPCConnector::updateSendImageData() {
    if (currentStatus != SENDING || outgoingBase64 == nullptr) {
        return false;
    }

    if (outgoingIndex < outgoingBase64->length()) {
        size_t remaining = outgoingBase64->length() - outgoingIndex;
        size_t len = min(SEND_CHUNK_SIZE, remaining);

        Serial.println(outgoingBase64->substring(outgoingIndex, outgoingIndex + len));

        outgoingIndex += len;

        return false; // still sending
    }

    Serial.println("IMG_END");

    outgoingBase64 = nullptr;
    outgoingDecodedBytes = 0;
    outgoingIndex = 0;
    currentStatus = IDLE;

    return true; // finished
}

bool boardToPCConnector::isBusy() const {
    return currentStatus != IDLE;
}

bool boardToPCConnector::receiveImageData(String& base64Image, int& expectedBytes) {
    // This function should be called repeatedly in the main loop to check for incoming data
    static bool receivingImage = false;
    static String incomingBase64 = "";
    static int bytesExpected = 0;
    bool transferComplete = false;

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();

        if (line.startsWith("IMG_BEGIN")) {
            receivingImage = true;
            incomingBase64 = "";
            currentStatus = RECEIVING;

            int spaceIndex = line.indexOf(' ');
            bytesExpected = line.substring(spaceIndex + 1).toInt();
        }
        else if (line == "IMG_END") {
            receivingImage = false;
            transferComplete = true;
            base64Image = incomingBase64;
            expectedBytes = bytesExpected;
            currentStatus = IDLE;
        }
        else if (receivingImage) {
            incomingBase64 += line;
        }
    }

    bool onReceivedImage = transferComplete;

    return onReceivedImage;
}
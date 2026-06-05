#include "boardToPCConnector.h"

size_t boardToPCConnector::expectedBase64Chars(size_t decodedBytes) const {
    if (decodedBytes == 0) {
        return 0;
    }

    return 4 * ((decodedBytes + 2) / 3);
}

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
    outgoingChunksSent = 0;

    Serial.print("DBG_MARS_PC_BEGIN expected_jpeg_bytes=");
    Serial.print(outgoingDecodedBytes);
    Serial.print(" base64_chars=");
    Serial.print(outgoingBase64->length());
    Serial.print(" expected_base64_chars=");
    Serial.print(expectedBase64Chars(outgoingDecodedBytes));
    Serial.print(" base64_delta=");
    Serial.print(static_cast<long>(outgoingBase64->length()) - static_cast<long>(expectedBase64Chars(outgoingDecodedBytes)));
    Serial.print(" chunk_size=");
    Serial.println(SEND_CHUNK_SIZE);

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
        outgoingChunksSent++;

        return false; // still sending
    }

    Serial.println("IMG_END");

    Serial.print("DBG_MARS_PC_END expected_jpeg_bytes=");
    Serial.print(outgoingDecodedBytes);
    Serial.print(" base64_chars_sent=");
    Serial.print(outgoingIndex);
    Serial.print(" chunks_sent=");
    Serial.print(outgoingChunksSent);
    Serial.print(" expected_base64_chars=");
    Serial.print(expectedBase64Chars(outgoingDecodedBytes));
    Serial.print(" base64_delta=");
    Serial.println(static_cast<long>(outgoingIndex) - static_cast<long>(expectedBase64Chars(outgoingDecodedBytes)));

    outgoingBase64 = nullptr;
    outgoingDecodedBytes = 0;
    outgoingIndex = 0;
    outgoingChunksSent = 0;
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

            int spaceIndex = line.indexOf(' ');
            bytesExpected = line.substring(spaceIndex + 1).toInt();
        }
        else if (line == "IMG_END") {
            receivingImage = false;
            transferComplete = true;
            base64Image = incomingBase64;
            expectedBytes = bytesExpected;
        }
        else if (receivingImage) {
            incomingBase64 += line;
        }
    }

    bool onReceivedImage = transferComplete;

    return onReceivedImage;
}

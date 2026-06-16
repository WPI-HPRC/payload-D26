#include "OpenMVReceiver.h"
#include <string.h>
#include <math.h>

OpenMVReceiver::OpenMVReceiver(Stream* inputStream) :
    inputStream(inputStream)
{
    streamLineBuffer.reserve(maxLineLength);
}

void OpenMVReceiver::setInputStream(Stream* inputStream) {
    this->inputStream = inputStream;
    streamLineBuffer = "";
    streamLineBuffer.reserve(maxLineLength);
}

bool OpenMVReceiver::runReceiver() {
    String receivedData = "";
    int receivedByteCount = 0;

    if(receiveData(receivedData, receivedByteCount)) {
        if(checkForDiagnosticLine(receivedData)) {
            Serial.println(receivedData);
            return false;
        }

        // check for the start of a transmission and start the receiving chain writing to the next open spot in the queue
        if(checkForTransmissionStart(receivedData)) {
            handleTransmissionStart(receivedData);
        }

        if(checkForTransmissionEnd(receivedData)) {
            handleTransmissionEnd(receivedData);
            return true;
        }

        if(receiving) {
            uint8_t queueLoc = currentQueueSize % maxQueueSize;
            handleTransmission(receivedData, imageQueue[queueLoc], imageSizes[queueLoc]);
        }
    }

    return false;
}

void OpenMVReceiver::testInput(const String& testInput, int& inputLength) {
    testInputData = testInput;
    inputLength = testInput.length();
}

bool OpenMVReceiver::getImage(String& outBase64Data, int& outByteCount) {
    if(currentQueueSize == 0) {
        return false;
    }

    outBase64Data = imageQueue[0];
    outByteCount = imageSizes[0];

    for(int i = 1; i < currentQueueSize; i++) {
        imageQueue[i - 1] = imageQueue[i];
        imageSizes[i - 1] = imageSizes[i];
    }

    uint8_t lastIndex = currentQueueSize - 1;
    imageQueue[lastIndex] = "";
    imageSizes[lastIndex] = 0;
    currentQueueSize--;

    return true;
}

uint8_t OpenMVReceiver::queueSize() {
    return currentQueueSize;
}

bool OpenMVReceiver::receiveData(String& outData, int& outByteCount) {
    if(testInputData.length() > 0) {
        outData = testInputData;
        outByteCount = testInputData.length();
        testInputData = "";
        return true;
    }

    if(inputStream == nullptr) {
        return false;
    }

    while(inputStream->available() > 0) {
        char nextChar = static_cast<char>(inputStream->read());

        if(nextChar == '\r') {
            continue;
        }

        if(nextChar == '\n') {
            outData = streamLineBuffer;
            outByteCount = streamLineBuffer.length();
            streamLineBuffer = "";

            return outData.length() > 0;
        }

        streamLineBuffer += nextChar;

        if(streamLineBuffer.length() > maxLineLength) {
            Serial.print("DBG_OPENMV_RX_DROP reason=line_overflow length=");
            Serial.println(streamLineBuffer.length());
            streamLineBuffer = "";
            resetIncomingTransmission();
            return false;
        }
    }

    return false;
}

bool OpenMVReceiver::checkForTransmissionStart(const String& receivedData) {
    return receivedData.startsWith("IMG_BEGIN");
}

bool OpenMVReceiver::checkForTransmissionEnd(const String& receivedData) {
    return receivedData.endsWith("IMG_END");
}

bool OpenMVReceiver::checkForDiagnosticLine(const String& receivedData) {
    return receivedData.startsWith("DBG_");
}

void OpenMVReceiver::handleTransmissionStart(String& receivedData) {
    resetIncomingTransmission();

    int expectedByteCount = parseExpectedByteCount(receivedData);

    if(expectedByteCount <= 0 || expectedByteCount > maxImageByteCount) {
        Serial.print("DBG_OPENMV_RX_DROP reason=invalid_byte_count bytes=");
        Serial.println(expectedByteCount);
        receivedData = "";
        return;
    }

    incomingExpectedByteCount = expectedByteCount;
    incomingBase64CharCount = 0;
    incomingChunkCount = 0;

    receivedData.replace("IMG_BEGIN", "");
    receivedData = "";

    makeRoomForNextImage();
    uint8_t queueLoc = currentQueueSize % maxQueueSize;
    imageQueue[queueLoc] = "";
    imageQueue[queueLoc].reserve(expectedBase64Chars(incomingExpectedByteCount));
    imageSizes[queueLoc] = 0;
    receiving = true;
}

int OpenMVReceiver::parseExpectedByteCount(const String& receivedData) {
    int spaceIndex = receivedData.indexOf(' ');

    if(spaceIndex < 0) {
        return 0;
    }

    return receivedData.substring(spaceIndex + 1).toInt();
}

int OpenMVReceiver::expectedBase64Chars(int decodedByteCount) {
    if(decodedByteCount <= 0) {
        return 0;
    }

    return 4 * ((decodedByteCount + 2) / 3);
}

void OpenMVReceiver::makeRoomForNextImage() {
    if(currentQueueSize >= maxQueueSize) {
        for(int i = 1; i < maxQueueSize; i++) {
            imageQueue[i - 1] = imageQueue[i];
            imageSizes[i - 1] = imageSizes[i];
        }

        imageQueue[maxQueueSize - 1] = "";
        imageSizes[maxQueueSize - 1] = 0;
        currentQueueSize = maxQueueSize - 1;
    }
}

void OpenMVReceiver::handleTransmissionEnd(String& receivedData) {
    if(!receiving) {
        receivedData = "";
        return;
    }

    receiving = false;

    receivedData.replace("IMG_END", "");

    int expectedChars = expectedBase64Chars(incomingExpectedByteCount);

    if(incomingBase64CharCount > maxBase64CharCount || (expectedChars > 0 && incomingBase64CharCount != expectedChars)) {
        Serial.print("DBG_OPENMV_RX_DROP reason=end_size_mismatch base64_chars=");
        Serial.print(incomingBase64CharCount);
        Serial.print(" expected_base64_chars=");
        Serial.println(expectedChars);
        resetIncomingTransmission();
        return;
    }

    // Serial.print("DBG_MARS_CAMERA_END expected_jpeg_bytes=");
    // Serial.print(incomingExpectedByteCount);
    // Serial.print(" base64_chars=");
    // Serial.print(incomingBase64CharCount);
    // Serial.print(" expected_base64_chars=");
    // Serial.print(expectedChars);
    // Serial.print(" base64_delta=");
    // Serial.print(incomingBase64CharCount - expectedChars);
    // Serial.print(" chunks=");
    // Serial.println(incomingChunkCount);

    if(incomingExpectedByteCount > 0) {
        imageSizes[currentQueueSize] = incomingExpectedByteCount;
    }

    currentQueueSize++;
    incomingExpectedByteCount = 0;
    incomingBase64CharCount = 0;
    incomingChunkCount = 0;
}

void OpenMVReceiver::handleTransmission(String& receivedData, String& queueLoc, int& byteCount) {
    if(receivedData.length() == 0) {
        return;
    }

    int nextBase64CharCount = incomingBase64CharCount + receivedData.length();
    int expectedChars = expectedBase64Chars(incomingExpectedByteCount);

    if(nextBase64CharCount > maxBase64CharCount || (expectedChars > 0 && nextBase64CharCount > expectedChars)) {
        Serial.print("DBG_OPENMV_RX_DROP reason=base64_overflow next_base64_chars=");
        Serial.print(nextBase64CharCount);
        Serial.print(" expected_base64_chars=");
        Serial.println(expectedChars);
        resetIncomingTransmission();
        receivedData = "";
        return;
    }
    
    queueLoc += receivedData;
    byteCount += receivedData.length();
    incomingBase64CharCount = nextBase64CharCount;
    incomingChunkCount++;
}

void OpenMVReceiver::resetIncomingTransmission() {
    bool wasReceiving = receiving;
    receiving = false;
    incomingExpectedByteCount = 0;
    incomingBase64CharCount = 0;
    incomingChunkCount = 0;

    if(wasReceiving || currentQueueSize == 0) {
        uint8_t queueLoc = currentQueueSize % maxQueueSize;
        imageQueue[queueLoc] = "";
        imageSizes[queueLoc] = 0;
    }
}

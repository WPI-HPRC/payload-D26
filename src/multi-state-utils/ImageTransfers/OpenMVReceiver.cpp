#include "OpenMVReceiver.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

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

        if(checkForConfigLine(receivedData)) {
            return false;
        }

        if(receiving) {
            if(checkForMLLine(receivedData)) {
                Serial.println("DBG_OPENMV_RX_DROP reason=ml_line_during_image");
                return false;
            }

            if(checkForTransmissionEnd(receivedData)) {
                handleTransmissionEnd(receivedData);
                return true;
            }

            uint8_t queueLoc = currentQueueSize % maxQueueSize;
            handleTransmission(receivedData, imageQueue[queueLoc], imageSizes[queueLoc]);
            return false;
        }

        if(checkForTransmissionStart(receivedData)) {
            handleTransmissionStart(receivedData);
            return false;
        }

        if(checkForMLLine(receivedData)) {
            handleMLLine(receivedData);
            return false;
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

bool OpenMVReceiver::hasMLResult() const {
    return mlResultAvailable;
}

bool OpenMVReceiver::getMLResult(OpenMVMLData& outMLResult) {
    if(!mlResultAvailable) {
        return false;
    }

    outMLResult = pendingMLResult;
    mlResultAvailable = false;
    return true;
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

bool OpenMVReceiver::checkForConfigLine(const String& receivedData) {
    return receivedData.startsWith("CFG");
}

bool OpenMVReceiver::checkForMLLine(const String& receivedData) {
    return receivedData.startsWith("ML_");
}

void OpenMVReceiver::handleTransmissionStart(String& receivedData) {
    resetIncomingTransmission();

    int expectedByteCount = parseExpectedByteCount(receivedData);

    if(expectedByteCount == 0 || expectedByteCount > maxImageByteCount) {
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
        return -1;
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

void OpenMVReceiver::handleMLLine(const String& receivedData) {
    if(receivedData.startsWith("ML_BEGIN")) {
        handleMLBegin(receivedData);
        return;
    }

    if(receivedData.startsWith("ML_HORIZON")) {
        handleMLHorizon(receivedData);
        return;
    }

    if(receivedData.startsWith("ML_BLOB")) {
        handleMLBlob(receivedData);
        return;
    }

    if(receivedData.startsWith("ML_END")) {
        handleMLEnd();
    }
}

void OpenMVReceiver::handleMLBegin(const String& receivedData) {
    unsigned long parsedFrameId = 0;
    unsigned int parsedBlobCount = 0;

    if(sscanf(receivedData.c_str(), "ML_BEGIN %lu %u", &parsedFrameId, &parsedBlobCount) != 2) {
        Serial.println("DBG_OPENMV_ML_DROP reason=bad_begin");
        resetIncomingMLResult();
        return;
    }

    resetIncomingMLResult();
    incomingMLResult.frameId = static_cast<uint32_t>(parsedFrameId);
    incomingMLResult.expectedBlobCount = parsedBlobCount > 255 ? 255 : static_cast<uint8_t>(parsedBlobCount);
    incomingMLResult.droppedBlobs = parsedBlobCount > OPENMV_ML_MAX_BLOBS;
    mlReceiving = true;
}

void OpenMVReceiver::handleMLHorizon(const String& receivedData) {
    if(!mlReceiving) {
        Serial.println("DBG_OPENMV_ML_DROP reason=horizon_without_begin");
        return;
    }

    int valid = 0;
    int yPx = 0;
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;

    if(sscanf(receivedData.c_str(), "ML_HORIZON %d %d %d %d %d %d", &valid, &yPx, &x1, &y1, &x2, &y2) != 6) {
        Serial.println("DBG_OPENMV_ML_DROP reason=bad_horizon");
        resetIncomingMLResult();
        return;
    }

    incomingMLResult.horizonValid = valid != 0;
    incomingMLResult.horizonYPx = static_cast<int16_t>(yPx);
    incomingMLResult.horizonX1 = static_cast<int16_t>(x1);
    incomingMLResult.horizonY1 = static_cast<int16_t>(y1);
    incomingMLResult.horizonX2 = static_cast<int16_t>(x2);
    incomingMLResult.horizonY2 = static_cast<int16_t>(y2);
}

void OpenMVReceiver::handleMLBlob(const String& receivedData) {
    if(!mlReceiving) {
        Serial.println("DBG_OPENMV_ML_DROP reason=blob_without_begin");
        return;
    }

    int index = 0;
    int cx = 0;
    int cy = 0;
    unsigned int pixels = 0;
    int ellipseCx = 0;
    int ellipseCy = 0;
    int ellipseRx = 0;
    int ellipseRy = 0;
    int ellipseRotation = 0;
    int parsed = sscanf(
        receivedData.c_str(),
        "ML_BLOB %d %d %d %u %d %d %d %d %d",
        &index,
        &cx,
        &cy,
        &pixels,
        &ellipseCx,
        &ellipseCy,
        &ellipseRx,
        &ellipseRy,
        &ellipseRotation
    );

    if(parsed < 4 || index < 0) {
        Serial.println("DBG_OPENMV_ML_DROP reason=bad_blob");
        resetIncomingMLResult();
        return;
    }

    if(index >= OPENMV_ML_MAX_BLOBS) {
        incomingMLResult.droppedBlobs = true;
        Serial.println("DBG_OPENMV_ML_DROP reason=too_many_blobs");
        return;
    }

    OpenMVMLBlob& blob = incomingMLResult.blobs[index];
    blob.cx = static_cast<int16_t>(cx);
    blob.cy = static_cast<int16_t>(cy);
    blob.pixels = pixels > 65535 ? 65535 : static_cast<uint16_t>(pixels);
    blob.hasEllipse = parsed >= 9;

    if(blob.hasEllipse) {
        blob.ellipseCx = static_cast<int16_t>(ellipseCx);
        blob.ellipseCy = static_cast<int16_t>(ellipseCy);
        blob.ellipseRx = static_cast<int16_t>(ellipseRx);
        blob.ellipseRy = static_cast<int16_t>(ellipseRy);
        blob.ellipseRotation = static_cast<int16_t>(ellipseRotation);
    }

    if(index + 1 > incomingMLResult.blobCount) {
        incomingMLResult.blobCount = static_cast<uint8_t>(index + 1);
    }
}

void OpenMVReceiver::handleMLEnd() {
    if(!mlReceiving) {
        Serial.println("DBG_OPENMV_ML_DROP reason=end_without_begin");
        return;
    }

    pendingMLResult = incomingMLResult;
    mlResultAvailable = true;
    resetIncomingMLResult();
}

void OpenMVReceiver::resetIncomingMLResult() {
    mlReceiving = false;
    incomingMLResult = {};
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

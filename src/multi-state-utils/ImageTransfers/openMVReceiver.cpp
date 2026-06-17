#include "OpenMVReceiver.h"
#include <string.h>
#include <math.h>

OpenMVReceiver::OpenMVReceiver(Stream* inputStream) :
    inputStream(inputStream)
{
}

void OpenMVReceiver::setInputStream(Stream* inputStream) {
    this->inputStream = inputStream;
    streamLineBuffer = "";
}

bool OpenMVReceiver::runReceiver() {
    String receivedData = "";
    int receivedByteCount = 0;

    if(receiveData(receivedData, receivedByteCount)) {
        if(checkForDiagnosticLine(receivedData)) {
            Serial.println(receivedData);
            return false;
        }

        // detection frames are a separate tagged block from image transmissions;
        // handle them first so their lines are never mistaken for image data
        if(checkForDetectionStart(receivedData)) {
            handleDetectionStart(receivedData);
            return false;
        }

        if(checkForDetectionEnd(receivedData)) {
            handleDetectionEnd();
            return false;
        }

        if(parsingDetections) {
            handleDetectionLine(receivedData);
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
    receiving = true;
    incomingExpectedByteCount = parseExpectedByteCount(receivedData);
    incomingBase64CharCount = 0;
    incomingChunkCount = 0;

    receivedData.replace("IMG_BEGIN", "");
    receivedData = "";

    makeRoomForNextImage();
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
    receiving = false;

    receivedData.replace("IMG_END", "");

    int expectedChars = expectedBase64Chars(incomingExpectedByteCount);

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
}

void OpenMVReceiver::handleTransmission(String& receivedData, String& queueLoc, int& byteCount) {
    if(receivedData.length() == 0) {
        return;
    }

    queueLoc += receivedData;
    byteCount += receivedData.length();
    incomingBase64CharCount += receivedData.length();
    incomingChunkCount++;
}

bool OpenMVReceiver::hasDetections() const {
    return detectionsAvailable;
}

bool OpenMVReceiver::getDetections(VisionDetections& out) {
    if(!detectionsAvailable) {
        return false;
    }

    out = latestDetections;
    detectionsAvailable = false;
    return true;
}

// Detection frames arrive as JSON lines (one flat compact object per line) with
// "type" as the first key, so routing only needs startsWith. Numeric fields are
// parsed positionally in their fixed sender order using the leading-number
// behaviour of toInt(), which stops at the trailing ',' or '}'. See
// VisionDetections.h for the line grammar.

// Confidence is sent as an integer in thousandths (0-1000) so it can be parsed
// with toInt() and scaled back to the 0.0-1.0 float the schema expects.
static const long DETECTION_CONFIDENCE_SCALE = 1000;

bool OpenMVReceiver::checkForDetectionStart(const String& receivedData) {
    return receivedData.startsWith("{\"type\":\"det_begin\"");
}

bool OpenMVReceiver::checkForDetectionEnd(const String& receivedData) {
    return receivedData.startsWith("{\"type\":\"det_end\"");
}

void OpenMVReceiver::handleDetectionStart(const String& receivedData) {
    (void)receivedData;
    parsingDetections = true;
    detectionFrame.blobCount = 0;
    detectionFrame.horizon = VisionHorizon();
}

void OpenMVReceiver::handleDetectionLine(const String& receivedData) {
    if(receivedData.startsWith("{\"type\":\"blob\"")) {
        VisionBlob blob;
        if(parseBlobLine(receivedData, blob) && detectionFrame.blobCount < MAX_VISION_BLOBS) {
            detectionFrame.blobs[detectionFrame.blobCount] = blob;
            detectionFrame.blobCount++;
        }
        return;
    }

    if(receivedData.startsWith("{\"type\":\"horizon\"")) {
        parseHorizonLine(receivedData, detectionFrame.horizon);
    }
}

void OpenMVReceiver::handleDetectionEnd() {
    parsingDetections = false;
    latestDetections = detectionFrame;
    detectionsAvailable = true;
}

// Advances rest past its next ':' so that the returned string begins at the
// following JSON value. toInt() on the result reads that value's leading number
// and ignores the trailing delimiter and remaining fields.
static String advanceToNextValue(String& rest) {
    int colonIndex = rest.indexOf(':');

    if(colonIndex < 0) {
        rest = "";
        return rest;
    }

    rest = rest.substring(colonIndex + 1);
    return rest;
}

bool OpenMVReceiver::parseBlobLine(const String& receivedData, VisionBlob& blob) {
    String rest = receivedData;

    advanceToNextValue(rest); // "type" value, skipped
    blob.index    = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.x        = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.y        = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.width    = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.height   = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.ellipseA = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.ellipseB = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    blob.rotation = static_cast<int16_t>(advanceToNextValue(rest).toInt());

    long confidenceScaled = advanceToNextValue(rest).toInt();
    if(confidenceScaled < 0) confidenceScaled = 0;
    if(confidenceScaled > DETECTION_CONFIDENCE_SCALE) confidenceScaled = DETECTION_CONFIDENCE_SCALE;
    blob.confidence = static_cast<float>(confidenceScaled) / static_cast<float>(DETECTION_CONFIDENCE_SCALE);

    return true;
}

bool OpenMVReceiver::parseHorizonLine(const String& receivedData, VisionHorizon& horizon) {
    String rest = receivedData;

    advanceToNextValue(rest); // "type" value, skipped
    horizon.x1 = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    horizon.y1 = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    horizon.x2 = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    horizon.y2 = static_cast<uint8_t>(advanceToNextValue(rest).toInt());
    horizon.valid = true;
    return true;
}

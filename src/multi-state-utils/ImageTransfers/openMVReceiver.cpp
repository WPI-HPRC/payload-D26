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

        if(checkForVisionStart(receivedData)) {
            handleVisionStart(receivedData);
            return false;
        }

        if(receivingVision) {
            if(checkForVisionEnd(receivedData)) {
                handleVisionEnd();
            } else {
                handleVisionLine(receivedData);
            }

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

bool OpenMVReceiver::getVisionData(RoverVisionFrame& outVisionData) {
    if(!pendingVisionAvailable) {
        return false;
    }

    outVisionData = pendingVisionData;
    pendingVisionData = RoverVisionFrame();
    pendingVisionAvailable = false;

    return true;
}

bool OpenMVReceiver::hasVisionData() const {
    return pendingVisionAvailable;
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

bool OpenMVReceiver::checkForVisionStart(const String& receivedData) {
    return receivedData.startsWith("VISION_BEGIN");
}

bool OpenMVReceiver::checkForVisionEnd(const String& receivedData) {
    return receivedData == "VISION_END";
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

void OpenMVReceiver::handleVisionStart(const String& receivedData) {
    incomingVisionData = RoverVisionFrame();
    incomingExpectedVisionBlobCount = 0;
    receivingVision = true;
    receiving = false;

    int tokenIndex = 0;
    parseNextToken(receivedData, tokenIndex);

    incomingVisionData.frameWidth = parseNextToken(receivedData, tokenIndex).toInt();
    incomingVisionData.frameHeight = parseNextToken(receivedData, tokenIndex).toInt();
    incomingVisionData.timestamp = static_cast<uint32_t>(parseNextToken(receivedData, tokenIndex).toInt());
    incomingExpectedVisionBlobCount = parseNextToken(receivedData, tokenIndex).toInt();
}

void OpenMVReceiver::handleVisionLine(const String& receivedData) {
    if(receivedData.startsWith("VISION_HORIZON")) {
        int tokenIndex = 0;
        parseNextToken(receivedData, tokenIndex);

        incomingVisionData.horizon.x1 = parseNextToken(receivedData, tokenIndex).toInt();
        incomingVisionData.horizon.y1 = parseNextToken(receivedData, tokenIndex).toInt();
        incomingVisionData.horizon.x2 = parseNextToken(receivedData, tokenIndex).toInt();
        incomingVisionData.horizon.y2 = parseNextToken(receivedData, tokenIndex).toInt();
        return;
    }

    if(!receivedData.startsWith("VISION_BLOB")) {
        return;
    }

    if(incomingVisionData.blobCount >= MAX_ROVER_VISION_BLOBS) {
        return;
    }

    int tokenIndex = 0;
    parseNextToken(receivedData, tokenIndex);

    RoverVisionBlob& blob = incomingVisionData.blobs[incomingVisionData.blobCount];
    blob.id = parseNextToken(receivedData, tokenIndex);
    blob.cx = parseNextToken(receivedData, tokenIndex).toInt();
    blob.cy = parseNextToken(receivedData, tokenIndex).toInt();
    blob.a = parseNextToken(receivedData, tokenIndex).toInt();
    blob.b = parseNextToken(receivedData, tokenIndex).toInt();
    blob.rotation = parseNextToken(receivedData, tokenIndex).toInt();
    blob.confidence = parseNextToken(receivedData, tokenIndex).toFloat() / 100.0f;
    blob.pixels = parseNextToken(receivedData, tokenIndex).toInt();
    incomingVisionData.blobCount++;
}

void OpenMVReceiver::handleVisionEnd() {
    receivingVision = false;

    if(incomingExpectedVisionBlobCount < 0) {
        incomingExpectedVisionBlobCount = 0;
    }

    pendingVisionData = incomingVisionData;
    pendingVisionAvailable = true;
    incomingVisionData = RoverVisionFrame();
    incomingExpectedVisionBlobCount = 0;
}

String OpenMVReceiver::parseNextToken(const String& data, int& startIndex) const {
    while(startIndex < data.length() && data.charAt(startIndex) == ' ') {
        startIndex++;
    }

    if(startIndex >= data.length()) {
        return "";
    }

    int endIndex = data.indexOf(' ', startIndex);

    if(endIndex < 0) {
        String token = data.substring(startIndex);
        startIndex = data.length();
        return token;
    }

    String token = data.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    return token;
}

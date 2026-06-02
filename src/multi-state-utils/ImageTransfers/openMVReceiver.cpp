#include "openMVReceiver.h"
#include <string.h>
#include <math.h>

openMVReceiver::openMVReceiver(Stream* inputStream) :
    inputStream(inputStream)
{
}

void openMVReceiver::setInputStream(Stream* inputStream) {
    this->inputStream = inputStream;
    streamLineBuffer = "";
}

bool openMVReceiver::runReceiver() {

    String receivedData = "";
    int receivedByteCount = 0;

    if(receiveData(receivedData, receivedByteCount)) {

        // check for the start of a transmission and start the receiving chain writing to the next open spot in the queue
        if(checkForTransmissionStart(receivedData)) {
            handleTransmissionStart(receivedData);
        }

        if(checkForTransmissionEnd(receivedData)) {
            handleTransmissionEnd(receivedData);

            return true;
        }

        if(receiving) {

            uint8_t queueLoc = currentQueueSize % maxQueueSize; // write to the current open spot in the queue


            handleTransmission(receivedData, imageQueue[queueLoc], imageSizes[queueLoc]);
        }


    }

    return false;

}

void openMVReceiver::testInput(const String& testInput, int& inputLength) {
    testInputData = testInput;
    inputLength = testInput.length();
}

bool openMVReceiver::getImage(String& outBase64Data, int& outByteCount) {
    if(currentQueueSize == 0) {
        return false; // no images in queue
    }

    outBase64Data = imageQueue[0]; // get the oldest image in the queue
    outByteCount = imageSizes[0];

    
    // shift remaining images forward in the queue
    for(int i = 1; i < currentQueueSize; i++) {
        imageQueue[i - 1] = imageQueue[i];
        imageSizes[i - 1] = imageSizes[i];
    }

    uint8_t lastIndex = currentQueueSize - 1;

    // clear the last spot in the queue
    imageQueue[lastIndex] = "";
    imageSizes[lastIndex] = 0;

    currentQueueSize--; // decrease queue size

    return true;
}

uint8_t openMVReceiver::queueSize() {
    return currentQueueSize;
}

bool openMVReceiver::receiveData(String& outData, int& outByteCount) {
    // testing entry point for receiving data - this should be removed or replaced with actual receiving logic
    if(testInputData.length() > 0) {
        outData = testInputData;
        outByteCount = testInputData.length();

        testInputData = ""; // clear test input after "receiving" it

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

bool openMVReceiver::checkForTransmissionStart(const String& receivedData) {
    
    bool retVal = false;


    // TODO: get more robust start checking using a contains check instead of starts with 
    if(receivedData.startsWith("IMG_BEGIN")) {
        retVal = true;
    }

    return retVal;
}

bool openMVReceiver::checkForTransmissionEnd(const String& receivedData) {
    
    bool retVal = false;

    if(receivedData.endsWith("IMG_END")) {
        retVal = true;
    }

    return retVal;
}   

void openMVReceiver::handleTransmissionStart(String& receivedData) {
    receiving = true;
    incomingExpectedByteCount = parseExpectedByteCount(receivedData);

    receivedData.replace("IMG_BEGIN", ""); // remove the start marker from the data
    receivedData = "";

    makeRoomForNextImage();
}

int openMVReceiver::parseExpectedByteCount(const String& receivedData) {
    int spaceIndex = receivedData.indexOf(' ');

    if(spaceIndex < 0) {
        return 0;
    }

    return receivedData.substring(spaceIndex + 1).toInt();
}

void openMVReceiver::makeRoomForNextImage() {
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

void openMVReceiver::handleTransmissionEnd(String& receivedData) {
    receiving = false;

    receivedData.replace("IMG_END", ""); // remove the end marker from the data

    // move to next spot in queue
    if(incomingExpectedByteCount > 0) {
        imageSizes[currentQueueSize] = incomingExpectedByteCount;
    }

    currentQueueSize++;
    incomingExpectedByteCount = 0;
    
    // currentQueueSize %= maxQueueSize; // wrap around if we exceed max queue size
   
}

void openMVReceiver::handleTransmission(String& receivedData, String& queueLoc, int& byteCount) {
    // This function should implement the logic to handle the incoming data and store it in the provided queue location.
    
    queueLoc += receivedData;
    byteCount += receivedData.length(); // Assuming each chunk is the length of the received data
}

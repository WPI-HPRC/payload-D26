#include "openMVReceiver.h"
#include <string.h>
#include <math.h>

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

    uint8_t removedIndex = 0; // index of the image being removed from the queue
    bool wrapped = false; // flag to indicate if we've wrapped around the circular queue

    if(currentQueueSize <= maxQueueSize) {
        outBase64Data = imageQueue[0]; // get the oldest image in the queue
        outByteCount = imageSizes[0];

    } else {
        // this case should only happen if we exceed max queue size and start overwriting old images, so we just return one above the current spot in the queue
        removedIndex = (currentQueueSize + 1) % maxQueueSize; // get the oldest image in the queue factoring in wrap around
        outBase64Data = imageQueue[removedIndex];
        outByteCount = imageSizes[removedIndex];
        wrapped = true;
    }

    
    // shift remaining images forward in the queue
    for(int i = removedIndex + 1; i < removedIndex + maxQueueSize && i < currentQueueSize; i++) {

        uint8_t wrappedIndex = i % maxQueueSize; // wrap around index for circular queue

        imageQueue[wrappedIndex - 1] = imageQueue[wrappedIndex];
        imageSizes[wrappedIndex - 1] = imageSizes[wrappedIndex];
    }

    // clear the last spot in the queue after shifting
    // if wrapped, the last spot is the one we just shifted from, otherwise it's just the current queue size - 1
    uint8_t lastIndex = (removedIndex + maxQueueSize - 1) % maxQueueSize; // index of the last valid image in the queue after shifting

    if(!wrapped) {
        lastIndex = currentQueueSize - 1; // if we haven't wrapped, the last valid image is just the current queue size - 1
    }

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
    // This function should implement the actual data receiving logic, such as reading from a serial port or network socket.
    // For demonstration purposes, we'll just return false to indicate no data received.


    // testing entry point for receiving data - this should be removed or replaced with actual receiving logic
    if(testInputData.length() > 0) {
        outData = testInputData;
        outByteCount = testInputData.length();

        testInputData = ""; // clear test input after "receiving" it

        return true;
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

    receivedData.replace("IMG_BEGIN", ""); // remove the start marker from the data
}

void openMVReceiver::handleTransmissionEnd(String& receivedData) {
    receiving = false;

    receivedData.replace("IMG_END", ""); // remove the end marker from the data

    // move to next spot in queue
    currentQueueSize++;
    
    // currentQueueSize %= maxQueueSize; // wrap around if we exceed max queue size
   
}

void openMVReceiver::handleTransmission(String& receivedData, String& queueLoc, int& byteCount) {
    // This function should implement the logic to handle the incoming data and store it in the provided queue location.
    
    queueLoc += receivedData;
    byteCount += receivedData.length(); // Assuming each chunk is the length of the received data
}
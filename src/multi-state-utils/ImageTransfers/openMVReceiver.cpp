#include "openMVReceiver.h"
#include <string.h>

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
            handleTransmission(receivedData, imageQueue[currentQueueSize], imageSizes[currentQueueSize]);
        }


    }

    return false;

}

void openMVReceiver::testInput(const String& testInput, int& inputLength) {
    testInputData = testInput;
    inputLength = testInput.length();
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
    
    currentQueueSize %= maxQueueSize; // wrap around if we exceed max queue size
   
}

void openMVReceiver::handleTransmission(String& receivedData, String& queueLoc, int& byteCount) {
    // This function should implement the logic to handle the incoming data and store it in the provided queue location.
    
    queueLoc += receivedData;
    byteCount += receivedData.length(); // Assuming each chunk is the length of the received data
}
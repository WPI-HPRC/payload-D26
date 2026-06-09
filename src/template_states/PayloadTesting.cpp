#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <stm32h753xx.h>
#include "../Context.h"
#include "../multi-state-utils/ImageTransfers/OpenMVReceiver.h"


/**
 * PC - MARS image transfer 
 */

// String diagnosticMessage = "";
boardToPCConnector connector;
OpenMVReceiver cameraReceiver;

extern HardwareSerial CAMERA_SERIAL;

bool imageReadyToSend = false;
unsigned long receivedAt = 0;
bool noDelay = false; // set to true to skip the 6 second wait before sending back to PC (for testing purposes)


void payloadTestingInit(StateData *data) {

    /// Initialize serial communication with the camera
    CAMERA_SERIAL.begin(115200);
    cameraReceiver.setInputStream(&CAMERA_SERIAL);
}



StateID payloadTestingLoop (StateData* data, Context* ctx) {

    /// For testing purposes, this state will receive image data from the camera and send it back to the PC. The Python script on the PC will print out the size of the received image data to verify that the transfer is working correctly.
    static String incomingBase64 = "";
    static int expectedBytes = 0;


    /**
     * MARS <- Camera image reception protocol:
     */

    /// Receive image data from camera
    if(cameraReceiver.runReceiver()) {

        if(cameraReceiver.getImage(incomingBase64, expectedBytes)) {
            imageReadyToSend = true;
            noDelay = true; // for testing purposes, set to true to skip the 6 second wait before sending back to PC
            receivedAt = millis();
        }
    }

    /**
     * MARS -> PC image transfer protocol:
     */

    /// Update send status if we're in the process of sending
    connector.updateSendImageData();

    /// If we're not currently busy sending an image, check if we have a new image ready to send back to the PC
    if(!connector.isBusy()) {
        if(connector.receiveImageData(incomingBase64, expectedBytes)) {
            imageReadyToSend = true;
            receivedAt = millis();
        }
    }

    /// If we have an image ready to send to PC and we're not currently busy sending, start the sending process (after a delay to allow the PC script to switch to receive mode)
    if(imageReadyToSend && !connector.isBusy()) {
        if(millis() - receivedAt > 6000.0 || noDelay) { // wait 6 seconds before sending back to PC to switch the python script
            connector.startSendingImageData(incomingBase64, expectedBytes);
            imageReadyToSend = false; // reset flag after attempting to send
        }
    }

    return PAYLOAD_TESTING;
}

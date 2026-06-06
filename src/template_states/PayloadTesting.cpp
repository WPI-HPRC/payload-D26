#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <stm32h753xx.h>
#include "../Context.h"
#include "../multi-state-utils/ImageTransfers/openMVReceiver.h"


/**
 * PC - MARS image transfer 
 */

// String diagnosticMessage = "";
boardToPCConnector connector;
openMVReceiver cameraReceiver;

extern HardwareSerial CAMERA_SERIAL;

bool imageReadyToSend = false;
unsigned long receivedAt = 0;
bool noDelay = false; // set to true to skip the 6 second wait before sending back to PC (for testing purposes)


void payloadTestingInit(StateData *data) {
    CAMERA_SERIAL.begin(115200);
    cameraReceiver.setInputStream(&CAMERA_SERIAL);
}



StateID payloadTestingLoop (StateData* data, Context* ctx) {

    static String incomingBase64 = "";
    static int expectedBytes = 0;

    if(cameraReceiver.runReceiver()) {

        // Serial.println("\n=== Image Transfer Completed ===");

        if(cameraReceiver.getImage(incomingBase64, expectedBytes)) {
            imageReadyToSend = true;
            noDelay = true; // for testing purposes, set to true to skip the 6 second wait before sending back to PC
            receivedAt = millis();

            // Serial.println("\n=== Retrived Image From Queue ===");
        }
    }

    // Update send status if we're in the process of sending
    connector.updateSendImageData();

    if(!connector.isBusy()) {
       

        if(connector.receiveImageData(incomingBase64, expectedBytes)) {
            
            // diagnosticMessage += "\nImage receive complete\n\n";
            // diagnosticMessage += "\nReceived base64 chars: " + String(incomingBase64.length());
            // diagnosticMessage += "\nExpected decoded bytes: " + String(expectedBytes);  
            // diagnosticMessage += "\n\n Starting sending image back to PC...";

            imageReadyToSend = true;
            receivedAt = millis();
        }
    }


    if(imageReadyToSend && !connector.isBusy()) {
        if(millis() - receivedAt > 6000.0 || noDelay) { // wait 6 seconds before sending back to PC to switch the python script
            if(connector.startSendingImageData(incomingBase64, expectedBytes)) {
                // diagnosticMessage += "\nImage send initiated successfully.";
            } else {
                // diagnosticMessage += "\nFailed to initiate image send.";
            }
            imageReadyToSend = false; // reset flag after attempting to send
        }
    }

   



    // static unsigned long last_print = 0;

    /// Serial Readout
    // if (millis() - last_print > 2000 && !receivingImage && !connector.isBusy())
    // {
    //     last_print = millis();
    //     Serial.print("\n=== State = Payload Testing | Loop Count = ");
    //     Serial.print(data->loopCount);
    //     Serial.println(" ===");

    //     Serial.print("Image Transfer Status: ");
    //     Serial.println(diagnosticMessage);
    // }

    return PAYLOAD_TESTING;
}

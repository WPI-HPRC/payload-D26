#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"
#include <Arduino.h>
#include "../Context.h"


/**
 * PC - MARS image transfer 
 */

// String diagnosticMessage = "";
boardToPCConnector connector;

bool imageReadyToSend = false;
unsigned long receivedAt = 0;


void payloadTestingInit(StateData *data) {}



StateID payloadTestingLoop (StateData* data, Context* ctx) {

    static String incomingBase64 = "";
    static int expectedBytes = 0;

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
        if(millis() - receivedAt > 6000.0) { // wait 6 seconds before sending back to PC to switch the python script
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

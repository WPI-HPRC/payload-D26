#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"
#include <Arduino.h>
#include "../Context.h"


/**
 * PC - MARS image transfer 
 */
String incomingBase64 = "";
bool receivingImage = false;
int expectedBytes = 0;
String diagnosticMessage = "";
bool imageReceived = false;
boardToPCConnector connector;


void payloadTestingInit(StateData *data) {}



StateID payloadTestingLoop (StateData* data, Context* ctx) {


    if(connector.receiveImageData(incomingBase64, expectedBytes)) {
        
        imageReceived = true;
        
        diagnosticMessage += "\nImage receive complete\n\n";
        diagnosticMessage += "\nReceived base64 chars: " + String(incomingBase64.length());
        diagnosticMessage += "\nExpected decoded bytes: " + String(expectedBytes);   
    }



    static unsigned long last_print = 0;

    /// Serial Readout
    if (millis() - last_print > 1000 && !receivingImage)
    {
        last_print = millis();
        Serial.print("\n=== State = Payload Testing | Loop Count = ");
        Serial.print(data->loopCount);
        Serial.println(" ===");

        Serial.print("Image Transfer Status: ");
        Serial.println(diagnosticMessage);
    }

    return PAYLOAD_TESTING;
}

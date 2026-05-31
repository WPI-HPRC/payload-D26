#define TEMPLATE_STATES_OVERRIDE
#include "../State.h"
#include <Arduino.h>


/**
 * PC - MARS image transfer 
 */
String incomingBase64 = "";
bool receivingImage = false;
int expectedBytes = 0;
String diagnosticMessage = "";
bool imageReceived = false;


void payloadTestingInit(StateData *data) {}



StateID payloadTestingLoop (StateData* data, Context* ctx) {

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();

        if (line.startsWith("IMG_BEGIN")) {
            receivingImage = true;
            incomingBase64 = "";

            int spaceIndex = line.indexOf(' ');
            expectedBytes = line.substring(spaceIndex + 1).toInt();

            diagnosticMessage += "\n\nStarted receiving image. Expecting " + String(expectedBytes) + " bytes.";
        }
        else if (line == "IMG_END") {
            receivingImage = false;

            diagnosticMessage += "\nReceived base64 chars: " + String(incomingBase64.length());
            diagnosticMessage += "\nExpected decoded bytes: " + String(expectedBytes);

            // Decode base64 here, then use/save image bytes

            diagnosticMessage += "\nImage receive complete\n\n";
            imageReceived = true;
        }
        else if (receivingImage) {
            incomingBase64 += line;
        }
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

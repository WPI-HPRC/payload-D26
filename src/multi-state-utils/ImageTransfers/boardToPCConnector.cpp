#include "boardToPCConnector.h"

void boardToPCConnector::sendImageData(const String& base64Image) {
    // Send the image data in base64 format to the PC
    Serial.println("IMG_BEGIN " + String(base64Image.length()));
    Serial.println(base64Image);
    Serial.println("IMG_END");
}

bool boardToPCConnector::receiveImageData(String& base64Image, int& expectedBytes) {
    // This function should be called repeatedly in the main loop to check for incoming data
    static bool receivingImage = false;
    static String incomingBase64 = "";
    static int bytesExpected = 0;
    bool transferComplete = false;

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();

        if (line.startsWith("IMG_BEGIN")) {
            receivingImage = true;
            incomingBase64 = "";

            int spaceIndex = line.indexOf(' ');
            bytesExpected = line.substring(spaceIndex + 1).toInt();
        }
        else if (line == "IMG_END") {
            receivingImage = false;
            transferComplete = true;
            base64Image = incomingBase64;
            expectedBytes = bytesExpected;
        }
        else if (receivingImage) {
            incomingBase64 += line;
        }
    }

    bool onReceivedImage = transferComplete;

    return onReceivedImage;
}
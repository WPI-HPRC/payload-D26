#pragma once

#include <Arduino.h>

class OpenMVReceiver {
    public:
        OpenMVReceiver(Stream* inputStream = nullptr);

        void setInputStream(Stream* inputStream);
        
        /**
         * master method to be run in update loop that runs all receiving and management processes of the receiver
         * returns true on new image received completely
         */
        bool runReceiver();

        /**
         * get number of images stored in the queue
         * returns an integer number of stored photos
         */
        uint8_t queueSize();

        /**
         * points parameters to the oldest string and metadata pair in the queue
         * 
         * returns true if an image can be retrieved, otherwise false (empty queue or error)
         */
        bool getImage(String& outBase64Data, int& outByteCount);

        void testInput(const String& testInput, int& inputLength);

    private:
        bool receiveData(String& outData, int& outByteCount);
        void makeRoomForNextImage();
        int parseExpectedByteCount(const String& receivedData);

        bool checkForTransmissionStart(const String& receivedData);
        bool checkForTransmissionEnd(const String& receivedData);
        bool checkForDiagnosticLine(const String& receivedData);

        void handleTransmissionStart(String& receivedData);
        void handleTransmissionEnd(String& receivedData);
        int expectedBase64Chars(int decodedByteCount);

        void handleTransmission(String& receivedData, String& queueLoc, int& byteCount);

        bool receiving = false;
        int incomingExpectedByteCount = 0;
        int incomingBase64CharCount = 0;
        int incomingChunkCount = 0;

        String testInputData = "";
        Stream* inputStream = nullptr;
        String streamLineBuffer = "";
        
        const static uint8_t maxQueueSize = 1;
        String imageQueue[maxQueueSize];
        int imageSizes[maxQueueSize] = {};
        uint8_t currentQueueSize = 0;
};

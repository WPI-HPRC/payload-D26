#pragma once

#include <Arduino.h>

class openMVReceiver {
    public:
        
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


        bool checkForTransmissionStart(const String& receivedData);
        bool checkForTransmissionEnd(const String& receivedData);

        void handleTransmissionStart(String& receivedData);
        void handleTransmissionEnd(String& receivedData);

        void handleTransmission(String& receivedData, String& queueLoc, int& byteCount);

        



        bool receiving = false;

        String testInputData = "";
        

        /// storage
        const static uint8_t maxQueueSize = 10;
        String imageQueue[maxQueueSize]; // Queue to hold incoming images
        int imageSizes[maxQueueSize] = {};
        uint8_t currentQueueSize = 0;



};

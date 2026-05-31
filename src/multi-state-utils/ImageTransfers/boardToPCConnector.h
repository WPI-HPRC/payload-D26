#include <Arduino.h>

class boardToPCConnector {

    private:
        enum Status {
            IDLE,
            RECEIVING,
            SENDING
        };

        Status currentStatus = IDLE;

        const String* outgoingBase64 = nullptr;
        size_t outgoingDecodedBytes = 0;
        size_t outgoingIndex = 0;

        static constexpr size_t SEND_CHUNK_SIZE = 128;


    public:

        bool startSendingImageData(const String& base64Image, size_t decodedBytes);
        bool updateSendImageData();
        bool isBusy() const;

        /*
        Receives image data from the PC in base64 format, along with the expected byte size.
         - This function should be called repeatedly in the main loop to check for incoming data.
         - Returns true if a complete image has been received and the provided references have been updated, false otherwise.
         - base64Image: reference to a String that will be populated with the incoming base64 data
         - expectedBytes: reference to an int that will be set to the expected byte size of the decoded image
         - This function should handle the protocol of receiving "IMG_BEGIN <size>", followed by base64 lines, and ending with "IMG_END".
         - It should also update internal state to track whether it's currently receiving an image and accumulate the incoming base64 data until completion.
         - Once "IMG_END" is received, it should set the provided references accordingly for further processing (e.g., decoding the image).
        */
       bool receiveImageData(String& base64Image, int& expectedBytes);

};
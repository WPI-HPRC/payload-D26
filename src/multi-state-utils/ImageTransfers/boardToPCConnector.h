#include <Arduino.h>

class boardToPCConnector {

    private:
        enum Status {
            IDLE,
            RECEIVING,
            SENDING
        };

        Status currentStatus = IDLE;

    public:

        /*
        Sends image data to the PC in base64 format.
        */
        void sendImageData(const String& base64Image);

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
       boolean receiveImageData(String& base64Image, int& expectedBytes);

};
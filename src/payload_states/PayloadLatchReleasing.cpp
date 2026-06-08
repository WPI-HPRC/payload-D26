#include "../State.h"
#include "../Context.h"

#define LATCH_RELEASE_DURATION_MS 6000 // Duration to simulate latch release in milliseconds
#define LATCH_DOWN_MS 1000 // Milliseconds to write to the servo for latch down position
#define LATCH_UP_MS 2000 // Milliseconds to write to the servo for latch up position


extern Servo latchServo; // Create an instance of the latch servo



bool checkLatchReleaseComplete(StateData *data) {
    
    /// TODO: need to discuss how completion will be determined. For now timing will be used but more may be required
    if(data->deltaTime > LATCH_RELEASE_DURATION_MS) {
        return true;
    }

    // test hook: check for a serial command to simulate latch release completion
    if(Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if(input == "latch_release_complete") {
            return true;
        }
    }
    return false; // Change to true to simulate latch release completion
}

void handleLatchReleaseComplete() {
   


    // debug print statement to indicate latch release completion
    Serial.println("Latch release complete! Performing post-release actions...");
    Serial.println("Exiting Payload Latch Releasing State...");
}

void payloadLatchReleasingInit(StateData *data) {

    Serial.println("Entered Payload Latch Releasing State...");


    // intialize latch servo
    latchServo.attach(LATCH_SERVO_PWM);

}

StateID payloadLatchReleasingLoop(StateData *data, Context *ctx) {

    /// continously write to the servo to ensure it stays in the correct position during the latch release process. This is to prevent any issues with the servo losing power or signal during this critical time.
    latchServo.writeMicroseconds(LATCH_UP_MS); // Write the servo to the latch up position


    if (checkLatchReleaseComplete(data)) {
        handleLatchReleaseComplete();

        return PAYLOAD_DEPLOYING; // Transition to the next state (deploying)
    }
    return PAYLOAD_LATCH_RELEASING;
}

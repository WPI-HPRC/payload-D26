#include "../State.h"
#include <Servo.h>
#include "Context.h"

#define SERVO_1_CLOSED_POSITION 1000 // microseconds
#define SERVO_1_OPEN_POSITION 2000 // microseconds
#define SERVO_2_CLOSED_POSITION 1000 // microseconds
#define SERVO_2_OPEN_POSITION 2000 // microseconds

#define ORIENTATION_CONFIRMATION_INTERVAL 1000 // milliseconds

// this is defined using the rover's frame of reference (i.e. looking down from the nosecone)
enum Orientation {
    UPRIGHT,
    UPSIDE_DOWN,
    ON_LEFT_SIDE,
    ON_RIGHT_SIDE,
    UNKNOWN
};

extern Servo selfRightingServo1; // connected to top panel
extern Servo selfRightingServo2; // connected to both side panels

// store the payload's orientation
Orientation currentOrientation;

Orientation determineOrientation(StateData *data, Context *ctx, Orientation currentOrientation, unsigned long long &changeTime) {

    static Orientation newOrientation = UNKNOWN;


    /// simple direct access version using accelerometer data --- should check if ekf is working and has a more reliable method.

    // get accelerometer data
    const auto &asm330_desc = ctx->asm330.get_descriptor();
    float accelX = asm330_desc.data.accel0; // forward/backward
    float accelY = asm330_desc.data.accel1; // right/left
    float accelZ = asm330_desc.data.accel2; // up/down


    // determine orientation based on accelerometer data
    if (accelZ > 700 && accelY > -700 && accelY < 700) { // assuming 1g = 1000 in the accelerometer's units, adjust threshold as needed
        newOrientation = UPRIGHT;
    } else if (accelZ < -700 && accelY > -700 && accelY < 700) {
        newOrientation = UPSIDE_DOWN;
    } else if (accelY > 700 && accelZ > -700 && accelZ < 700) {
        newOrientation = ON_LEFT_SIDE;
    } else if (accelY < -700 && accelZ > -700 && accelZ < 700) {
        newOrientation = ON_RIGHT_SIDE;
    } else {
        newOrientation = UNKNOWN;
    }
    
   
    /// test hook input from serial monitor
    if (Serial.available() > 0) {
        char input = Serial.read();
        switch (input) {
            case 'u':
                 newOrientation = UPRIGHT;
                 break;
            case 'd':
                newOrientation = UPSIDE_DOWN;
                break;
            case 'l':
                newOrientation = ON_LEFT_SIDE;
                break;
            case 'r':
                newOrientation = ON_RIGHT_SIDE;
                break;
            default:
                newOrientation = UNKNOWN;
                break;
        }
    }


    if(newOrientation != currentOrientation) {
        
        // if orientation has changed, update the change time and print the new orientation
        changeTime = data->currentTime;
        Serial.print("Orientation changed to: ");
        switch (newOrientation) {
            case UPRIGHT:
                Serial.println("UPRIGHT");
                break;
            case UPSIDE_DOWN:
                Serial.println("UPSIDE_DOWN");
                break;
            case ON_LEFT_SIDE:
                Serial.println("ON_LEFT_SIDE");
                break;
            case ON_RIGHT_SIDE:
                Serial.println("ON_RIGHT_SIDE");
                break;
            default:
                Serial.println("UNKNOWN");
                break;
        }
    }

    return newOrientation;
}

bool handleUpright(StateData *data, unsigned long long lastOrientationChangeTime) {

    // if the payload is upright, we can proceed with the next steps of the mission after delay for confirmation
    if(data->currentTime - lastOrientationChangeTime > ORIENTATION_CONFIRMATION_INTERVAL) {
        Serial.println("Payload is upright.");
        return true;
    }
    return false;
}

void handleUpsideDown(StateData *data, unsigned long long &lastOrientationChangeTime) {
    // if the payload is upside down, we may want to activate some self-righting mechanism (e.g. spinning up a reaction wheel, deploying a small parachute on one side, etc.)
    Serial.println("Payload is upside down.");
    selfRightingServo2.writeMicroseconds(SERVO_2_CLOSED_POSITION); // ensure side panels are closed so they can be used to flip the payload next
   
    if (data->currentTime - lastOrientationChangeTime > 3000) { // after delay to let the side panels close fully, activate the top panel to flip the payload
        Serial.println("Opening top panel to right the payload...");
        selfRightingServo1.writeMicroseconds(SERVO_1_OPEN_POSITION); // open top panel to push the craft onto its right side
        selfRightingServo2.writeMicroseconds(SERVO_2_CLOSED_POSITION); // keep side panels closed so the payload can land on them
    }
}


void handleOnSide(StateData *data, unsigned long long &lastOrientationChangeTime) {
    // if the payload is on its side, we may want to activate some self-righting mechanism
    Serial.println("Payload is on its side.");

    if(data->currentTime - lastOrientationChangeTime > ORIENTATION_CONFIRMATION_INTERVAL) { // after delay to ensure confirmation of orientation
        Serial.println("Closing top panel to prepare for righting the payload...");
        selfRightingServo1.writeMicroseconds(SERVO_1_CLOSED_POSITION); // ensure the top panel is closed so it doesn't interfere with the side panels righting the payload
    } else if(data->currentTime - lastOrientationChangeTime > ORIENTATION_CONFIRMATION_INTERVAL + 3000) { // after delay, activate the side panels to right the payload
        Serial.println("Opening side panels to right the payload...");
        selfRightingServo1.writeMicroseconds(SERVO_1_CLOSED_POSITION); // ensure the top panel remains closed so it doesn't interfere with the side panels righting the payload
        selfRightingServo2.writeMicroseconds(SERVO_2_OPEN_POSITION); // open side panels to push the craft upright
    }
}

bool handleUnknown(StateData *data, unsigned long long &lastOrientationChangeTime) {

    unsigned long long timeInUnknown = data->currentTime - lastOrientationChangeTime;

    // technically the self righting process is not dependent on the accelerometer working.
    // all we have to do is open the top panel and then side panels in order and it will end upright no matter where it starts

    // dead reckoning sequence
    if(timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL && timeInUnknown <= ORIENTATION_CONFIRMATION_INTERVAL + 2000) { // after some time has passed in the unknown state, take some precautionary measures
        Serial.println("Payload orientation has been unknown for 3 seconds. Starting blind self-righting procedure...");
        Serial.println("ensuring top panel is closed...");
        selfRightingServo1.writeMicroseconds(SERVO_1_CLOSED_POSITION);
        selfRightingServo2.writeMicroseconds(SERVO_2_CLOSED_POSITION);
    } else if (timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL + 2000 && timeInUnknown <= ORIENTATION_CONFIRMATION_INTERVAL + 6000) { // then open the top panel
        Serial.println("Payload orientation has been unknown for 5 seconds. Opening top panel as part of blind self-righting procedure...");
        selfRightingServo1.writeMicroseconds(SERVO_1_OPEN_POSITION);
        selfRightingServo2.writeMicroseconds(SERVO_2_CLOSED_POSITION);
    } else if (timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL + 6000 && timeInUnknown <= ORIENTATION_CONFIRMATION_INTERVAL + 10000) { // then open the side panels
        Serial.println("Payload orientation has been unknown for 9 seconds. Opening side panels as part of blind self-righting procedure...");
        selfRightingServo1.writeMicroseconds(SERVO_1_CLOSED_POSITION);
        selfRightingServo2.writeMicroseconds(SERVO_2_OPEN_POSITION);
    } else if (timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL + 10000) { // after 13 seconds in the unknown state, we have done everything we can to try to right the payload, so just proceed with the mission and hope for the best
        Serial.println("Payload orientation has been unknown for 13 seconds. Proceeding with mission but self-righting procedure was performed blindly, so there is a risk the payload is not correctly oriented...");
        return true;
    }

    return false;
}


void payloadSelfRightingInit(StateData *data) {

    Serial.println("Entered Payload Self-Righting State...");

    // initialize servos to closed position
    selfRightingServo1.attach(SELF_RIGHTING_PWM1); // attach servo 1 to pin defined by SELF_RIGHTING_PWM1
    selfRightingServo2.attach(SELF_RIGHTING_PWM2); // attach servo 2 to pin defined by SELF_RIGHTING_PWM2
    selfRightingServo1.writeMicroseconds(SERVO_1_CLOSED_POSITION);
    selfRightingServo2.writeMicroseconds(SERVO_2_CLOSED_POSITION);
}

StateID payloadSelfRightingLoop(StateData *data, Context *ctx) {

    static unsigned long long lastOrientationChangeTime = 0;

    currentOrientation = determineOrientation(data, ctx, currentOrientation, lastOrientationChangeTime);

    switch (currentOrientation) {
        case UPRIGHT:
            if(handleUpright(data, lastOrientationChangeTime)) {
                return PAYLOAD_LATCH_RELEASING;
            }
            break;
        case UPSIDE_DOWN:
            // these handlers should change the accelerometer readings by moving the payload
            // so no explicit transition is needed here, we just wait for the orientation to change to upright
            handleUpsideDown(data, lastOrientationChangeTime);
            break;
        case ON_LEFT_SIDE:
            handleOnSide(data, lastOrientationChangeTime);
            break;
        case ON_RIGHT_SIDE:
            handleOnSide(data, lastOrientationChangeTime);
            break;
        default:
            // dead reckoning sequence if orientation is unknown for some reason (e.g. accelerometer failure)
            if(handleUnknown(data, lastOrientationChangeTime)) {
                return PAYLOAD_LATCH_RELEASING;
            }
            break;
    }


    return PAYLOAD_SELF_RIGHTING;
}

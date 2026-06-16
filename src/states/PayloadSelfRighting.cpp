#include "../State.h"
#include <Servo.h>
#include "Context.h"

static constexpr uint16_t DEFAULT_SERVO_2_CLOSED_POSITION = 1750; // microseconds
static constexpr uint16_t DEFAULT_SERVO_2_OPEN_POSITION = 1350; // microseconds
static constexpr uint16_t DEFAULT_SERVO_1_CLOSED_POSITION = 1250; // microseconds
static constexpr uint16_t DEFAULT_SERVO_1_OPEN_POSITION = 1730; // microseconds

static uint16_t servo1ClosedPosition = DEFAULT_SERVO_1_CLOSED_POSITION;
static uint16_t servo1OpenPosition = DEFAULT_SERVO_1_OPEN_POSITION;
static uint16_t servo2ClosedPosition = DEFAULT_SERVO_2_CLOSED_POSITION;
static uint16_t servo2OpenPosition = DEFAULT_SERVO_2_OPEN_POSITION;

#define ORIENTATION_CONFIRMATION_INTERVAL 1000 // milliseconds

# define USING_IMU_BASED_ORIENTATION_DETECTION 0 // comment out to use test hook for orientation input instead of IMU data

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

enum SelfRightingServoPositionSelection {
    SELF_RIGHTING_CLOSED,
    SELF_RIGHTING_OPEN
};

static String getToken(String const &input, uint8_t targetIndex) {
    uint8_t currentIndex = 0;
    int start = -1;

    for (int i = 0; i <= input.length(); i++) {
        bool atEnd = i == input.length();
        bool atSeparator = atEnd || input.charAt(i) == ' ' || input.charAt(i) == '\t';

        if (!atSeparator && start < 0) {
            start = i;
        }

        if (atSeparator && start >= 0) {
            if (currentIndex == targetIndex) {
                return input.substring(start, i);
            }

            currentIndex++;
            start = -1;
        }
    }

    return "";
}

static bool parseSelfRightingMotor(String const &token, uint8_t &motor) {
    if (token == "1" || token == "servo1" || token == "motor1" || token == "top") {
        motor = 1;
        return true;
    }

    if (token == "2" || token == "servo2" || token == "motor2" || token == "side" || token == "sides") {
        motor = 2;
        return true;
    }

    return false;
}

static bool parseSelfRightingPosition(String const &token, SelfRightingServoPositionSelection &position) {
    if (token == "closed" || token == "close") {
        position = SELF_RIGHTING_CLOSED;
        return true;
    }

    if (token == "open") {
        position = SELF_RIGHTING_OPEN;
        return true;
    }

    return false;
}

static uint16_t& getSelfRightingServoPosition(uint8_t motor, SelfRightingServoPositionSelection position) {
    if (motor == 1) {
        return position == SELF_RIGHTING_OPEN ? servo1OpenPosition : servo1ClosedPosition;
    }

    return position == SELF_RIGHTING_OPEN ? servo2OpenPosition : servo2ClosedPosition;
}

static Servo& getSelfRightingServo(uint8_t motor) {
    return motor == 1 ? selfRightingServo1 : selfRightingServo2;
}

static const char* getSelfRightingPositionName(SelfRightingServoPositionSelection position) {
    return position == SELF_RIGHTING_OPEN ? "open" : "closed";
}

static void printSelfRightingServoTuningStatus(uint8_t motor, SelfRightingServoPositionSelection position, uint16_t stepUs) {
    Serial.print("Self-righting servo tuning: selected motor=");
    Serial.print(motor);
    Serial.print(" position=");
    Serial.print(getSelfRightingPositionName(position));
    Serial.print(" pulse=");
    Serial.print(getSelfRightingServoPosition(motor, position));
    Serial.print(" us step=");
    Serial.print(stepUs);
    Serial.println(" us.");

    Serial.print("Positions: s1 closed=");
    Serial.print(servo1ClosedPosition);
    Serial.print(" open=");
    Serial.print(servo1OpenPosition);
    Serial.print(" | s2 closed=");
    Serial.print(servo2ClosedPosition);
    Serial.print(" open=");
    Serial.print(servo2OpenPosition);
    Serial.println();
}

static void printSelfRightingServoTuningHelp() {
    Serial.println("Self-righting servo tuning active.");
    Serial.println("Commands: motor <1|2>, servo <1|2>, open, closed, value <us>, set <us>, servo <1|2> <open|closed> <us>, +, -, step <us>, status.");
}

static void applySelfRightingServoTuningSelection(uint8_t motor, SelfRightingServoPositionSelection position) {
    getSelfRightingServo(motor).writeMicroseconds(getSelfRightingServoPosition(motor, position));
}

static void runSelfRightingServoTuningLoop() {
    static bool initialized = false;
    static uint8_t selectedMotor = 1;
    static SelfRightingServoPositionSelection selectedPosition = SELF_RIGHTING_CLOSED;
    static uint16_t stepUs = 25;

    if (!initialized) {
        initialized = true;
        printSelfRightingServoTuningHelp();
        printSelfRightingServoTuningStatus(selectedMotor, selectedPosition, stepUs);
    }

    String input = "";
    if (Serial.available() > 0) {
        input = Serial.readStringUntil('\n');
        input.trim();
        input.toLowerCase();
    }

    if (input.length() > 0) {
        String firstToken = getToken(input, 0);
        String secondToken = getToken(input, 1);
        String thirdToken = getToken(input, 2);
        String fourthToken = getToken(input, 3);
        bool handled = false;

        if (input == "help") {
            printSelfRightingServoTuningHelp();
            handled = true;
        } else if (input == "status" || input == "?") {
            handled = true;
        } else if (input == "+" || input == "inc" || input == "increase") {
            uint16_t &pulse = getSelfRightingServoPosition(selectedMotor, selectedPosition);
            pulse = constrain(static_cast<int>(pulse) + stepUs, 500, 2500);
            handled = true;
        } else if (input == "-" || input == "dec" || input == "decrease") {
            uint16_t &pulse = getSelfRightingServoPosition(selectedMotor, selectedPosition);
            pulse = constrain(static_cast<int>(pulse) - stepUs, 500, 2500);
            handled = true;
        } else if ((firstToken == "value" || firstToken == "set" || firstToken == "pulse") && secondToken.length() > 0) {
            int pulse = secondToken.toInt();
            if (pulse > 0) {
                getSelfRightingServoPosition(selectedMotor, selectedPosition) = constrain(pulse, 500, 2500);
            }
            handled = true;
        } else if (firstToken == "step" && secondToken.length() > 0) {
            int step = secondToken.toInt();
            if (step > 0) {
                stepUs = constrain(step, 1, 500);
            }
            handled = true;
        } else if ((firstToken == "motor" || firstToken == "servo") && parseSelfRightingMotor(secondToken, selectedMotor)) {
            if (parseSelfRightingPosition(thirdToken, selectedPosition) && fourthToken.length() > 0) {
                int pulse = fourthToken.toInt();
                if (pulse > 0) {
                    getSelfRightingServoPosition(selectedMotor, selectedPosition) = constrain(pulse, 500, 2500);
                }
            }
            handled = true;
        } else if (parseSelfRightingMotor(firstToken, selectedMotor)) {
            if (parseSelfRightingPosition(secondToken, selectedPosition) && thirdToken.length() > 0) {
                int pulse = thirdToken.toInt();
                if (pulse > 0) {
                    getSelfRightingServoPosition(selectedMotor, selectedPosition) = constrain(pulse, 500, 2500);
                }
            }
            handled = true;
        } else if (parseSelfRightingPosition(firstToken, selectedPosition)) {
            if (secondToken.length() > 0) {
                int pulse = secondToken.toInt();
                if (pulse > 0) {
                    getSelfRightingServoPosition(selectedMotor, selectedPosition) = constrain(pulse, 500, 2500);
                }
            }
            handled = true;
        }

        if (!handled) {
            Serial.print("Unknown self-righting servo tuning command: ");
            Serial.println(input);
            printSelfRightingServoTuningHelp();
        }

        printSelfRightingServoTuningStatus(selectedMotor, selectedPosition, stepUs);
    }

    applySelfRightingServoTuningSelection(selectedMotor, selectedPosition);
}

Orientation determineOrientation(StateData const *data, Context *ctx, Orientation currentOrientation, unsigned long long &changeTime) {

    static Orientation newOrientation = UNKNOWN;


    /// simple direct access version using accelerometer data --- should check if ekf is working and has a more reliable method.


    if (USING_IMU_BASED_ORIENTATION_DETECTION) {
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

bool handleUpright(StateData const *data, unsigned long long lastOrientationChangeTime) {

    // if the payload is upright, we can proceed with the next steps of the mission after delay for confirmation
    if(data->currentTime - lastOrientationChangeTime > ORIENTATION_CONFIRMATION_INTERVAL) {
        Serial.println("Payload is upright.");
        return true;
    }
    return false;
}

void handleUpsideDown(StateData const *data, unsigned long long &lastOrientationChangeTime) {
    // if the payload is upside down, we may want to activate some self-righting mechanism (e.g. spinning up a reaction wheel, deploying a small parachute on one side, etc.)
    Serial.println("Payload is upside down.");
    selfRightingServo2.writeMicroseconds(servo2ClosedPosition); // ensure side panels are closed so they can be used to flip the payload next
   
    if (data->currentTime - lastOrientationChangeTime > 3000) { // after delay to let the side panels close fully, activate the top panel to flip the payload
        Serial.println("Opening top panel to right the payload...");
        selfRightingServo1.writeMicroseconds(servo1OpenPosition); // open top panel to push the craft onto its right side
        selfRightingServo2.writeMicroseconds(servo2ClosedPosition); // keep side panels closed so the payload can land on them
    }
}


void handleOnSide(StateData const *data, unsigned long long &lastOrientationChangeTime) {
    // if the payload is on its side, we may want to activate some self-righting mechanism
    Serial.println("Payload is on its side.");

    if(data->currentTime - lastOrientationChangeTime > ORIENTATION_CONFIRMATION_INTERVAL) { // after delay to ensure confirmation of orientation
        Serial.println("Closing top panel to prepare for righting the payload...");
        selfRightingServo1.writeMicroseconds(servo1ClosedPosition); // ensure the top panel is closed so it doesn't interfere with the side panels righting the payload
    } else if(data->currentTime - lastOrientationChangeTime > ORIENTATION_CONFIRMATION_INTERVAL + 3000) { // after delay, activate the side panels to right the payload
        Serial.println("Opening side panels to right the payload...");
        selfRightingServo1.writeMicroseconds(servo1ClosedPosition); // ensure the top panel remains closed so it doesn't interfere with the side panels righting the payload
        selfRightingServo2.writeMicroseconds(servo2OpenPosition); // open side panels to push the craft upright
    }
}

bool handleUnknown(StateData const *data, unsigned long long &lastOrientationChangeTime) {

    unsigned long long timeInUnknown = data->currentTime - lastOrientationChangeTime;

    // technically the self righting process is not dependent on the accelerometer working.
    // all we have to do is open the top panel and then side panels in order and it will end upright no matter where it starts

    // dead reckoning sequence
    if(timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL && timeInUnknown <= ORIENTATION_CONFIRMATION_INTERVAL + 2000) { // after some time has passed in the unknown state, take some precautionary measures
        Serial.println("Payload orientation has been unknown for 3 seconds. Starting blind self-righting procedure...");
        Serial.println("ensuring top panel is closed...");
        selfRightingServo1.writeMicroseconds(servo1ClosedPosition);
        selfRightingServo2.writeMicroseconds(servo2ClosedPosition);
    } else if (timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL + 2000 && timeInUnknown <= ORIENTATION_CONFIRMATION_INTERVAL + 6000) { // then open the top panel
        Serial.println("Payload orientation has been unknown for 5 seconds. Opening top panel as part of blind self-righting procedure...");
        selfRightingServo1.writeMicroseconds(servo1OpenPosition);
        selfRightingServo2.writeMicroseconds(servo2ClosedPosition);
    } else if (timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL + 6000 && timeInUnknown <= ORIENTATION_CONFIRMATION_INTERVAL + 10000) { // then open the side panels
        Serial.println("Payload orientation has been unknown for 9 seconds. Opening side panels as part of blind self-righting procedure...");
        selfRightingServo1.writeMicroseconds(servo1ClosedPosition);
        selfRightingServo2.writeMicroseconds(servo2OpenPosition);
    } else if (timeInUnknown > ORIENTATION_CONFIRMATION_INTERVAL + 10000) { // after 13 seconds in the unknown state, we have done everything we can to try to right the payload, so just proceed with the mission and hope for the best
        Serial.println("Payload orientation has been unknown for 13 seconds. Proceeding with mission but self-righting procedure was performed blindly, so there is a risk the payload is not correctly oriented...");
        return true;
    }

    return false;
}


void *payloadSelfRightingInit(StateData const *data) {

    Serial.println("Entered Payload Self-Righting State...");

    // initialize servos to closed position
    selfRightingServo1.attach(SELF_RIGHTING_PWM2); // attach servo 1 to pin defined by SELF_RIGHTING_PWM2
    selfRightingServo2.attach(SELF_RIGHTING_PWM1); // attach servo 2 to pin defined by SELF_RIGHTING_PWM1
    selfRightingServo1.writeMicroseconds(servo1ClosedPosition);
    selfRightingServo2.writeMicroseconds(servo2ClosedPosition);

    return nullptr;
}

StateID payloadSelfRightingLoop(StateData const *data, Context *ctx, void *_localData) {

#if ENABLE_SELF_RIGHTING_SERVO_TUNING_DEBUG
    runSelfRightingServoTuningLoop();
    return PAYLOAD_SELF_RIGHTING;
#endif


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

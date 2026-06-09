#include "../State.h"
#include "../Context.h"


#define ANTENNA_RAISE_DURATION_MS 3000
#define ANTENNA_DOWN_MS 1000
#define ANTENNA_UP_MS 2000

extern Servo antennaServo;

// get a reference in order to detach the pin now that the rover is disconnected
extern Servo latchServo;

void raiseAntenna() {
    antennaServo.writeMicroseconds(ANTENNA_UP_MS);
}

bool checkAntennaRaised(StateData *data, String input) {
    if(data->currentTime > ANTENNA_RAISE_DURATION_MS) {
        return true;
    }

    if(input == "antenna_raised") {
        return true;
    }

    return false;
}

void handleAntennaRaised() {
    Serial.println("Antenna raised successfully!");
    Serial.println("Exiting Payload Deployed State...");
}

void payloadDeployedInit(StateData *data) {

    Serial.println("Entered Payload Deployed State...");

    Serial.println("Disconnecting Latch Servo");
    latchServo.detach();

    Serial.println("Attaching Antenna Servo");
    antennaServo.attach(ANTENNA_SERVO_PWM);
}

StateID payloadDeployedLoop(StateData *data, Context *ctx) {

    String input = "";

    if(Serial.available()) {
        input = Serial.readStringUntil('\n');
        input.trim();
    }

    raiseAntenna();

    if (checkAntennaRaised(data, input)) {
        handleAntennaRaised();
        return PAYLOAD_CONNECTING;
    }

    input = ""; // reset input string for next loop iteration
    return PAYLOAD_DEPLOYED;
}

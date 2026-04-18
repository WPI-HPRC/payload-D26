#include "../State.h"
#include <Servo.h>
#include "Context.h"

#define LEFT_MOTOR_PIN 0   // placeholder
#define RIGHT_MOTOR_PIN 1
#define PWM_output 90 

void forwardInit (StateData* data) {
    data->servo1.attach(LEFT_MOTOR_PIN);
    data->servo2.attach(RIGHT_MOTOR_PIN);
}

StateID ForwardLoop (StateData *data, Context *ctx) {

    //Sets a PWM output
    SerialUSB.print("Driving Forward");
    data->servo1.write(PWM_output);
    data->servo2.write(PWM_output);
    return FORWARD; // does not exit state
}
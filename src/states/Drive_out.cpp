#include "../State.h"
#include "States.h"
#include <Servo.h>
#include "Context.h"

#define DRIVEOUT_TIME 10000 // in millisceond

void driveoutInit (StateData* data) {
    SerialUSB.print("Entering Drive out Sequence");
    data->left_motor.attach(LEFT_MOTOR_PIN);
    data->right_motor.attach(RIGHT_MOTOR_PIN);

    data->startTime = millis();
}

StateID DriveOutLoop (StateData *data, Context *ctx) {

    float l_motor_pwm = (PWM_COEF * PWM_CONST) + PWM_OFFSET;
    float r_motor_pwm = (PWM_COEF * PWM_CONST) + PWM_OFFSET;

    //Set PWM outputs
    data->left_motor.write(l_motor_pwm);
    data->right_motor.write(r_motor_pwm);

    //update timer
    data->currentTime = millis();
    data->deltaTime = data->currentTime - data->deltaTime;

    if (data->deltaTime > DRIVEOUT_TIME){
        return CONNECT; // deploy antenna when timer expired
    }

    return DRIVE_OUT; // Drive out unless timer expired
}
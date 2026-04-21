#include "../State.h"
#include "States.h"
#include <Servo.h>
#include "Context.h"

#define LEFT_MOTOR_PIN 0   // placeholder constants
#define RIGHT_MOTOR_PIN 1
#define PWM_CONST 90 
#define PWM_COEF 1
#define PWM_OFFSET 10

void forwardInit (StateData* data) {
    data->servo1.attach(LEFT_MOTOR_PIN);
    data->servo2.attach(RIGHT_MOTOR_PIN);
}

StateID ForwardLoop (StateData *data, Context *ctx) {

    float lin_vel = data->linear_vel;
    float ang_vel = data->angular_vel;

    float l_motor_vel = lin_vel - ang_vel;
    float r_motor_vel = lin_vel + ang_vel;

    float l_motor_pwm = (PWM_COEF * l_motor_vel) + PWM_OFFSET;
    float r_motor_pwm = (PWM_COEF * r_motor_vel) + PWM_OFFSET;

    //Sets a PWM output
    SerialUSB.print("Driving Forward");
    data->servo1.write(l_motor_pwm);
    data->servo2.write(r_motor_pwm);
    return FORWARD; // does not exit state
}
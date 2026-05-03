#include "../State.h"
#include "States.h"
#include <Servo.h>
#include "Context.h"
#include "camera.h"


void teleopInit (StateData* data) {
    SerialUSB.print("Entering Teleoporation");
    data->left_motor.attach(LEFT_MOTOR_PIN);
    data->right_motor.attach(RIGHT_MOTOR_PIN);

    camera_initialization();
}

StateID TeleopLoop (StateData *data, Context *ctx) {

    // Collect Image Data
    get_image();

    // Handle Drive input
    float lin_vel = data->linear_vel;
    float ang_vel = data->angular_vel;

    float l_motor_vel = lin_vel - ang_vel;
    float r_motor_vel = lin_vel + ang_vel;

    float l_motor_pwm = (PWM_COEF * l_motor_vel) + PWM_OFFSET;
    float r_motor_pwm = (PWM_COEF * r_motor_vel) + PWM_OFFSET;

    //Set PWM outputs
    data->left_motor.write(l_motor_pwm);
    data->right_motor.write(r_motor_pwm);

    if (!data->connected){
        return CONNECT; // if connection is lost, attempt to connect
    }

    return TELEOP; // else: continue teleoperation
}
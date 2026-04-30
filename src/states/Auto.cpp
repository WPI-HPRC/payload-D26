#include "../State.h"
#include "States.h"
#include <Servo.h>
#include "Context.h"


void autoInit (StateData* data) {
    SerialUSB.print("Entering Teleoporation");
    data->left_motor.attach(LEFT_MOTOR_PIN);
    data->right_motor.attach(RIGHT_MOTOR_PIN);
}

StateID AutoLoop (StateData *data, Context *ctx) {

    float lin_vel = data->linear_vel;
    float ang_vel = data->angular_vel;

    float l_motor_vel = lin_vel - ang_vel;
    float r_motor_vel = lin_vel + ang_vel;

    float l_motor_pwm = (PWM_COEF * l_motor_vel) + PWM_OFFSET;
    float r_motor_pwm = (PWM_COEF * r_motor_vel) + PWM_OFFSET;

    //Set PWM outputs
    data->left_motor.write(l_motor_pwm);
    data->right_motor.write(r_motor_pwm);

    if (data->connected){
        return TELEOP; // if connection made, enter teleoperation
    }

    return AUTO; // else: continue teleoperation

    /*
    if(robotState == ROBOT_ALIGNING_TAG)
    {
        if(hasTag)
        {
            Serial.println("Tag saw");
            float centerX = 80.0;
            float errorX = lastTag.cx - centerX;

            float targetSize = 50;
            float errorDist = targetSize - lastTag.w;

            float Kp_turn = 0.02;
            float Kp_forward = 0.3;

            float turn = Kp_turn * errorX;
            float forward = Kp_forward * errorDist;

            if(abs(errorX) < 5) turn = 0;

            if(forward > 8) forward = 8;
            if(forward < -4) forward = -4;

            chassis.SetTwist(forward, turn); // proportional control

        
            if(lastTag.w > 45) //tag width threshold
            { 
                chassis.FullStop();
                robotState = ROBOT_IDLE;
                Serial.println("Tag reached");
                
            }
        }
        else
        {
            chassis.SetTwist(0, 0.4);
        }
    }
    */
}
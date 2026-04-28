#pragma once

#include "Context.h"
#include "debouncer.h"

// placeholder constants

// Drive Motor Control
#define LEFT_MOTOR_PIN 0    // pin for the left motor
#define RIGHT_MOTOR_PIN 1   // pin for the right motor
#define PWM_CONST 90        // constant pwm value
#define PWM_COEF 1          // coefficent for mapping pwm to motor speed
#define PWM_OFFSET 10       // offset for mappign pwm to motor speed 

// Antenna Servo Control
#define ANTENNA_SERVO_PIN 2
#define ANTENNA_UPRIGHT_PWM 90

enum StateID {
    FORWARD,
    NUM_STATES,
    DRIVE_OUT,
    CONNECT,
    AUTO,
    TELEOP,
};

struct StateData {
    long long currentTime;
    long long deltaTime;
    long long loopCount;
    long long startTime;
    long long lastLoopTime;
    uint32_t lastAccelReadingTime;
    uint32_t lastBaroReadingTime;
    Debouncer baroDebouncer = Debouncer(20);
    Debouncer accelDebouncer = Debouncer(20);
    Debouncer velDebouncer = Debouncer(20);

    Servo left_motor;
    Servo right_motor;
    Servo antenna_servo;
    Servo release_servo;
    float linear_vel;
    float angular_vel;

    bool connected;

};


// FORWARD
void forwardInit(StateData *data);
void driveoutInit(StateData *data);
void connectInit(StateData *data);
void autoInit(StateData *data);
void teleopInit(StateData *data);
StateID recoveryLoop (StateData* data, Context* ctx);



/**
 * @file wiring_layout_defs.h
 * @brief For convenience since multiple versions of the wiring layout are being tested, this file connects symbolic component pin names to their generic pin definitions. This way, the pin definitions can be easily swapped out by changing this one file instead of having to change multiple files that reference the pins.
 * 
 */

#pragma once

// testing functionality on a V2.0 board with different wiring layout than the real rover
#define ALEX_TEST_WIRING_LAYOUT

// actual wiring layout for the rover 
//#define ACTUAL_ROVER_WIRING_LAYOUT

#ifdef ALEX_TEST_WIRING_LAYOUT
    // camera serial (connected to openMV)
    #define CAMERA_SERIAL_RX PWM_OUT3
    #define CAMERA_SERIAL_TX PWM_OUT4


    // screw drive pins
    #define LEFT_SCREW_PWM PWM_OUT5
    #define RIGHT_SCREW_PWM PWM_OUT6

    #define LATCH_SERVO_PWM PWM_OUT1

    #define ANTENNA_SERVO_PWM ADC_INN5

    #define SELF_RIGHTING_PWM1 PWM_OUT2
    #define SELF_RIGHTING_PWM2 PWM_OUT3
#endif

#ifdef ACTUAL_ROVER_WIRING_LAYOUT
    // camera serial (connected to openMV)
    #define CAMERA_SERIAL_RX CAMERA_SCK
    #define CAMERA_SERIAL_TX CAMERA_CS

    // screw drive pins
    #define LEFT_SCREW_PWM PWM_OUT5
    #define RIGHT_SCREW_PWM PWM_OUT6

    #define LATCH_SERVO_PWM PWM_OUT1

    #define ANTENNA_SERVO_PWM PWM_OUT3

    #define SELF_RIGHTING_PWM1 PWM_OUT2
    #define SELF_RIGHTING_PWM2 PWM_OUT3
#endif
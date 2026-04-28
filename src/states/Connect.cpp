#include "../State.h"
#include "States.h"
#include <Servo.h>
#include "Context.h"

#define CONNECTION_WAIT_LIMIT 10000 // in millisceond

void connectInit (StateData* data) {
    SerialUSB.print("Connecting to Ground Station");
    data->antenna_servo.attach(ANTENNA_SERVO_PIN);
}

StateID ConnectLoop (StateData *data, Context *ctx) {

    //Sets a PWM output
    data->antenna_servo.write(ANTENNA_UPRIGHT_PWM);

    //update timer
    data->currentTime = millis();
    data->deltaTime = data->currentTime - data->deltaTime;

    if (data->connected){
        return TELEOP; //Connection succeeded, entering teleoperation
    }

    if (data->deltaTime > CONNECTION_WAIT_LIMIT){
        return AUTO; // Connection failed, entering autonomous
    }
    
    return CONNECT; // else: continue waiting for connection
}
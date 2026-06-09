#include "../State.h"
#include "../multi-state-utils/ScrewDrive/ScrewDriveInterface.h"

static constexpr float DEPLOY_SPEED = 0.35f;
static constexpr float DEPLOY_TURN_CORRECTION = 0.0f;
static constexpr uint32_t DEPLOYMENT_DURATION_MS = 3000; // Duration to simulate deployment in milliseconds

extern ScrewDriveInterface screwDrive;

bool checkDeploymentComplete(String input, StateData *data) {
    
    // timer check
    if(data->currentTime > DEPLOYMENT_DURATION_MS) {
        return true;
    }

    // test hook: check for a serial command to simulate deployment completion
    if(input == "deployment_complete") {
        return true;
    }
    
    return false; // Change to true to simulate deployment completion
}

void handleDeploymentComplete() {
    // Placeholder for actions to take once deployment is complete
    // This might involve activating certain hardware or sending a message
    screwDrive.stop();
    Serial.println("Deployment complete! Performing post-deployment actions...");
    Serial.println("Exiting Payload Deploying State...");
}

bool checkDeploymentFailure(String input, StateData *data) {
    

    // if get large current draw


    // test hook: check for a serial command to simulate deployment failure
    if(input == "deployment_failed") {
        return true;
    }
    
    return false; // Change to true to simulate deployment failure
}

void handleDeploymentFailure() {
    // Placeholder for actions to take once deployment fails
    // This might involve activating certain hardware or sending a message
    screwDrive.stop();
    Serial.println("Deployment failed! Performing failure actions...");
    Serial.println("Exiting Payload Deploying State...");
}


void handleDriveOut(StateData *data) {
    screwDrive.drive(DEPLOY_SPEED, DEPLOY_TURN_CORRECTION);
}

void payloadDeployingInit(StateData *data) {
    Serial.println("Entered Payload Deploying State...");
    screwDrive.attach(LEFT_SCREW_PWM, RIGHT_SCREW_PWM);
    screwDrive.beginArm();
}

StateID payloadDeployingLoop(StateData *data, Context *ctx) {

    // check if ESCs are armed and if so drive out slowly
    if (screwDrive.updateArm()) {
        handleDriveOut(data);
    }

    String input = "";

    if(Serial.available()) {
        input = Serial.readStringUntil('\n');
        input.trim();
    }

    if (checkDeploymentComplete(input, data)) {
        handleDeploymentComplete();
        return PAYLOAD_DEPLOYED;
    }

    if (checkDeploymentFailure(input, data)) {
        handleDeploymentFailure();
        return PAYLOAD_LATCH_RELEASING; // Transition back to latch releasing state to attempt deployment again
    }

    input = ""; // reset input string for next loop iteration

    return PAYLOAD_DEPLOYING;
}

#include "../State.h"



bool checkDeploymentComplete(String input) {
    // Placeholder for actual deployment completion logic
    // In a real implementation, this might check sensor data or a timer

    if(input == "deployment_complete") {
        return true;
    }
    
    return false; // Change to true to simulate deployment completion
}

void handleDeploymentComplete() {
    // Placeholder for actions to take once deployment is complete
    // This might involve activating certain hardware or sending a message
    Serial.println("Deployment complete! Performing post-deployment actions...");
    Serial.println("Exiting Payload Deploying State...");
}

bool checkDeploymentFailure(String input) {
    // Placeholder for actual deployment failure logic
    // In a real implementation, this might check sensor data or a timer

    if(input == "deployment_failed") {
        return true;
    }
    
    return false; // Change to true to simulate deployment failure
}

void handleDeploymentFailure() {
    // Placeholder for actions to take once deployment fails
    // This might involve activating certain hardware or sending a message
    Serial.println("Deployment failed! Performing failure actions...");
    Serial.println("Exiting Payload Deploying State...");
}


void handleDriveOut(StateData *data) {
    // Placeholder for actions to take to drive out the payload
    // This might involve activating certain hardware or sending a message
}

void payloadDeployingInit(StateData *data) {
    Serial.println("Entered Payload Deploying State...");
}

StateID payloadDeployingLoop(StateData *data, Context *ctx) {

    handleDriveOut(data);

    String input = "";

    if(Serial.available()) {
        input = Serial.readStringUntil('\n');
        input.trim();
    }

    if (checkDeploymentComplete(input)) {
        handleDeploymentComplete();
        return PAYLOAD_DEPLOYED;
    }

    if (checkDeploymentFailure(input)) {
        handleDeploymentFailure();
        return PAYLOAD_LATCH_RELEASING; // Transition back to latch releasing state to attempt deployment again
    }

    input = ""; // reset input string for next loop iteration

    return PAYLOAD_DEPLOYING;
}

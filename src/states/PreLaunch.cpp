#include "../State.h"
#include "Arduino.h"
#include "StateMachineConstants.h"
#include "config.h"
#include "logging.h"

void prelaunchInit(StateData *data) {}

StateID prelaunchLoop(StateData *data, Context *ctx) {
  // Serial.println("looping prelaunch");

  static bool BlueLedState = false;
  static bool GreenLedState = false;
  static uint32_t lastBlueToggleTime = 0;
  static uint32_t lastGreenToggleTime = 0;
  //static bool ComputeInitialOrientationThisLoop = false;

  // get acceleration, magnetometer, and gps data 
  const auto &accel_desc = ctx->asm330.get_descriptor();
  BLA::Matrix<3, 1> accel = {accel_desc.data.accel0, accel_desc.data.accel1,
                             accel_desc.data.accel2};

  const auto &mag_desc = ctx->mag.get_descriptor();
  BLA::Matrix<3, 1> mag = {mag_desc.data.mag0, mag_desc.data.mag1,
                           mag_desc.data.mag2};

  const auto &gps_desc = ctx->gps.get_descriptor();
  // BLA::Matrix<3, 1> gps = {gps_desc.data.ecefX, gps_desc.data.ecefY,
  //                          gps_desc.data.ecefZ};

  // Compute initial sensor data and orientation for 2 seconds after 5 second delay
  if (data->currentTime > 5000 && data->currentTime < 7000) {
    // ctx->estimator.padLoop(accel, mag, gps);
    // ComputeInitialOrientationThisLoop = true;

    ctx->estimator.computeInitialOrientation();
    //ComputeInitialOrientationThisLoop = false;
    ctx->ekfLooping = true;
  }

  /*
  - Poll acceleration data from ctx
  - Check acceleration to detect launch
  - Check if need to abort
  - Update sensor data and ctx for next iteration?
  */

  // get accel data
  const auto acc_vec = ctx->estimator.get_accel_prev();

  // check acceleration in vertical direction is greater than threshold (5) with debouncer
  if (data->accelDebouncer.update(abs(acc_vec(0, 0)) > LAUNCH_TRHESHOLD, millis())) {
    return BOOST;
  }

  if (ctx->sdInitialized && ctx->logFile != NULL) {
    // blink blue led every 250ms
    if ((millis() - lastBlueToggleTime) > 250) {
      lastBlueToggleTime = millis();
      BlueLedState = !BlueLedState;
      digitalWrite(LED_BLUE, BlueLedState);
    }
  }

  // if (gps_desc.data.gpsLockType == 3) {
    // blink green led every 250ms
    if ((millis() - lastGreenToggleTime) > 250) {
      lastGreenToggleTime = millis();
      GreenLedState = !GreenLedState;
      digitalWrite(LED_GREEN, GreenLedState);
    }
  // }

  return PRELAUNCH;
}

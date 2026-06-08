# Screw Drive Interface

`ScrewDriveInterface` owns two `Servo` objects and converts high-level screw
drive commands into bidirectional ESC PWM pulses.

## ESC Signal Model

The default signal range is centered around neutral:

- `1000 us`: full reverse
- `1500 us`: neutral / stop
- `2000 us`: full forward

The interface starts with `maxEffort = 0.25`, so a full-scale command only uses
25 percent of that pulse range until testing proves a higher value is safe.

The JCR Mini 35A AM32 ESC does not include an onboard BEC. Power the ESCs from
their own supply and make sure ESC signal ground and board ground are common.

## State Usage

```cpp
extern ScrewDriveInterface screwDrive;

void payloadDeployingInit(StateData *data) {
    screwDrive.attach(LEFT_SCREW_PWM, RIGHT_SCREW_PWM);
    screwDrive.beginArm();
}

void handleDriveOut(StateData *data) {
    if (screwDrive.updateArm()) {
        screwDrive.drive(0.20f, 0.0f);
    }
}
```

Call `stop()` before leaving a state that should no longer command the motors.
Arming is non-blocking: `beginArm()` starts the neutral hold period, and
`updateArm()` should be called from the state loop until it returns true.

## Arcade Drive

`drive(speed, turn)` takes normalized inputs in `[-1.0, 1.0]`.

- `speed`: forward/reverse screw effort
- `turn`: differential steering effort

In `ARCADE` mode:

```text
left = speed + turnScale * turn
right = speed - turnScale * turn
```

The default `turnScale` is `0.4`, which keeps turns broad and avoids
counter-rotating the screws for mild turn inputs.

## Tuning

- `setMaxEffort(effort)`: caps motor output globally.
- `setMotorCorrections(left, right)`: compensates for one screw having more
  friction or a stronger motor/ESC response.
- `setMotorInversions(left, right)`: flips one or both outputs around neutral.
- `neutralPulseUs`, `minPulseUs`, and `maxPulseUs` are public if the ESC
  calibration uses a different range.

Example ROV command:

```cpp
screwDrive.setMaxEffort(0.35f);
screwDrive.setMotorCorrections(1.0f, 0.92f);
screwDrive.drive(0.4f, 0.25f);
```

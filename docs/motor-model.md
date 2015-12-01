# Motor Model

## Constants

*(TODO: include how these constants are being used in context)*

```c
const float MOTOR_SYSTEM_RESISTANCE = 0.3f
```

This is the resistance (in ohms) of the rest of the system for the model.

```c
const float MOTOR_PWM_FREQUENCY = 1150.0f
```

This is the frequency of the [PWM][] pulse cycle.

[PWM]: https://en.wikipedia.org/wiki/Pulse-width_modulation

```c
const float MOTOR_DIODE_VOLTAGE = 0.75f
```

This is the voltage across the model's diode during the off phase.

```c
const int MOTOR_COMMAND_MAX = 127
```

This is the maximum command that can be given to a motor.

## Motor Model Setup

The following parameters and settings are supplied to `motorModelInit`.

```c
MotorModelSetup setup =
{
    .backEmfPerRpm = 0.0f,
    .resistance = 0.0f,
    .inductance = 0.0f,
    .smoothing = 0.0f,
    .rpmFree = 0.0f,
    .channel = 2
}
MotorModel model = motorModelInit(setup);
```

The `backEmfPerRpm` (float) is the [back EMF][] constant of the motor, measured in Volts per RPM. This is the amount of EMF generated per RPM the motor is spinning at.

The `resistance` (float) is the resistance assumed in the motor model, measured in Ohms.

The `inductance` (float) is the inductance assumed in the motor model.

The `smoothing` (float) is the time constant for the low pass filter, measured in seconds, that is applied to the motor's current to get the value `currentFiltered`.

The `rpmFree` (float) is the maximum RPM of the motor running freely.

The `channel` (unsigned char) is the motor channel on the Vex cortex, on which the motor is connected to.

[back Emf]: https://en.wikipedia.org/wiki/Motor_constants

## The Internal MotorModel Struct

The `MotorModel` struct describes the state and parameters of a motor model at a given time. It is defined in `motor-model.h` so it can be embedded and used directly elsewhere.

The `model.command` (int between -127 to +127) is the motor command given to the motor.

The `model.direction` is the direction of the motor's rotation and current, calculated as the sign of the `model.command`.

The `model.dutyOn` (float) is the proportion of the cycle where the PWM pulse is high.

The `model.dutyOff` (float) is the proportion of the cycle where the PWM pulse is low **and** the current have yet to reach zero.

The `model.backEmfMax` (float) is the maximum possible EMF that can be generated from a free spinning motor.

The `model.backEmfPerRpm` (float) is the back EMF constant of the motor, measured in volts per RPM. This is the amount of EMF that is generated per RPM the motor is spinning at.

The `model.backEmf` (float) is the EMF generated back from a spinning motor at that time.

The `model.resistance` (float) is the resistance assumed by the motor model.

The `model.inductance` (float) is the inductance assumed by the motor model.

The `model.currentSteadyStateOn` (float) is the value of the current when the PWM pulse is held high and the system reaches a steady state.

The `model.currentSteadyStateOff` (float) is the value of the current when the PWM pulse is held low and the system reaches a steady state.

The `model.lambda` (float) is the ratio of PWM period relative to the motor time constant of the inductor circuit (time constant = inductance / resistance).

The `model.contributionPeak` (float) is the contribution ratio of initial current divided by (initial current + on-phase steady state current) of the current at the end of the on-phase (peak current).

The `model.contributionInitial` (float) is the contribution ratio of the peak current divided by (peak current + off-phase steady state current) of the current at the end of the off-phase (final = initial current).

The `model.currentInitial` (float) is the current at the beginning of each PWM cycle.

The `model.currentPeak` (float) is the peak current at the end of the on-phase and the start of the off-phase.

The `model.smoothing` (float) is the low-pass filter's time constant.

The `model.current` (float) is the calculated average current of the motor, using the raw approximation without filtering.

The `model.currentFiiltered` (float) is the filtered version of `model.current`.

The `model.commandNeedsScaling` (bool) indicates whether the motor is connected to a port between 2 through to 9, in which case their commands need rescaling.

## Derivation of the Motor Model



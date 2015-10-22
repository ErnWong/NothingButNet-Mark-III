# Flywheel Setup and Initialisation

This page is all about the config you need to setup a [flywheel](). To use a
`Flywheel`, first create a `FlywheelSetup` object and fill its members in.
Then pass it into the `flywheelInit` function which then spits out your
`Flywheel` object as a pointer:

```c
FlywheelSetup setup =
{
    .id = "flywhe~1",
    .gearing = 5.0f,
    .smoothing = 1.0f,
    .encoderGetter = encoderGetter,
    .encoderResetter = encoderResetter,
    .encoderArgs = encoderGetArgs(portBottom, portTop),
    .motorSetters =
    [
        motorSetter,
        motorSetter,
        smartMotorSetter
    ],
    .motorArgs =
    [
        motorGetArgs(1, false),
        motorGetArgs(2, true),
        smartMotorGetArgs(4, false)
    ],
    .controllerType = FLYWHEEL_TBH,
    .pidKp = 1.0f,
    .pidKd = 1.0f,
    .pidKi = 1.0f,
    .tbhGain = 1.0f,
    .bbValue = 127.0f,
    .bbAbove = 0.0f,
    .bbBelow = 0.0f,
    .priorityReady = 2,
    .priorityActive = 2,
    .frameDelayReady = 200,
    .frameDelayActive = 20,
    .readyErrorInterval = 1.0f,
    .readyDerivativeInterval = 1.0f,
    .readyCheckCycle = 20
}
Flywheel * myFlywheel = flywheelInit(setup);
```

Here's more detail about each setting of the setup:

### Pigeon id

```c
.id = "flywhe~1"
```

This is a null terminated 9-character array, with at most 8 non-null
characters.

This becomes the id the flywheel will try to use when using [Pigeon](). This means
that if you specify `.id = "myfw"`, you can interact with the flywheel
like this:

```
(pc to robot)
myfw.target 1200
myfw.type pid
myfw.isready
(robot to pc)
[00421538|myfw.isready     ] false
```

For more info, check out Pigeon#id.
For more info about the flywheel pigeon portal and what it can do, check out
Flywheel Portal.

### External input/outputs

#### Flywheel RPM input

```c
.gearing = 1.0f,
.smoothing = 1.0f,
.encoderGetter = encoderGetter,
.encoderResetter = encoderResetter,
.encoderArgs = encoderGetArgs(portTop, portBottom)
```

Flywheel uses a generic function pointer as a getter, so that it could read the
rpm of the flywheel from any possible source: from a [shaft encoder](), [IME module](),
or even a dummy function with data tethered in.

An `encoderGetter` is a pointer to a function that returns a raw reading of the rpm.
The function should have the following signature:

```c
float getter(void * args);
```

Having getter args is a useful feature, because the same getter function may
be used more than once. This args can therefore be used to identify what it is
supposed to be getting, e.g. indentifying which encoder it is getting from. The getter args is a generic pointer, so it can be a
pointer to a string, a struct, a function, anything.

For most uses, you can use the provided [`encoderGetter`](), [`imeGetter`](),
[`encoderResetter'](), [`imeResetter'](),
[`encoderGetArgs()`](), and [`imeGetArgs()`]() functions for these settings. For
more info about the predefined getters, checkout their respective docs.

The `gearing` sets the scale factor, in which the `encoderGetter` values are
multiplied to get the raw flywheel rpm readings. For example, if the encoder
is placed on a shaft, but geared to the flywheel with the ratio of 1:25, then
`gearing = 25.0f`.

The `smoothing` sets the amount of smoothing applied to the raw rpm readings.
That's because, by the very [nature]() of most encoders and digital processing,
the values will turn out to be discrete and will contain unwanted noise.
That would affect the controller algorithms, such as giving off a wrong ready
signal or taking-back-half at the wrong times. Smoothing is implemented as a
simple first order(?) [low-pass filter](), aka an
[exponentially weighted average](), of the raw values. The `smoothing`
parameter is the time constant, in seconds.

Note: the more `smoothing` it has, the more lag there will be between real and
measured rpm.
[`z`]() [`z`]()

#### Motor output

```c
.motorSetters = []
.motorArgs = []
```

Motor setters are pointers to functions that sets the motor command to a motor
port. This can be [`motorSetter`](), [`smartMotorSetter`](), or
`yourVeryOwnCustomSetter`.

List your motor setters and motor targets in the arrays, with at most 8 motors.
I mean, you only have 10 ports, and only 8 of which are 'safer' ports, so to
keep the Flywheel struct not-too-monstrously-big and to avoid unnecessary
use of [linked lists](), we've set the limit to 8.

The `motorSetters` is an array of function pointers, each with the signature:

```c
void motorSetter(void * args, int command);
```

Note that the flywheel stops looking at the first
`NULL` entry of the array, so you should not
leave gaps in the array.

The `motorArgs` are the corresponding `arg` arguments to be passed to the
corresponding motor setter function. This `arg` is useful to identify which
motor to set, especially when the same generic function is used for multiple
motors.

As for the predefined motor setters, the [`motorGetArgs`]() function creates
the args for `motorSetter`, and [`smartMotorGetArgs`]() creates for
`smartMotorSetter`. For more info, check out their respective docs.

### Controller tuning parameters

```c
.controllerType = FLYWHEEL_TBH
```

Choose the controller to initially use, with an enum choice of `FLYWHEEL_PID`,
`FLYWHEEL_TBH`, and `FLYWHEEL_BANGBANG`.

#### PID controller

```c
.pidKp = 0.0f
.pidKi = 0.0f
.pidKd = 0.0f
```

Sets the proportional gain, integral gain, and the derivative gain
respectively.

See PID Controllers for more info.

#### Take back half controller

```c
.tbhGain
```

See TBH Controllers for more info.

#### Bang bang controller

```c
.bbValue,
.bbAbove,
.bbBelow
```

See Bang-bang Controllers

### Flywheel states

```c
.priorityReady
.priorityActive

.frameDelayReady
.frameDelayActive

.readyErrorInterval
.readyDerivativeInterval
.readyCheckCycle
```

The flywheel has two states: `ready` and `active`.

Flywheel is `ready` when the flywheel rpm is very near the target rpm, and
the rpm had been stablilised. When the flywheel enters the ready state, it sets
the priority of its running task to `priorityReady`, and sets the delay of each
frame to `frameDelayReady` (in milliseconds). Typically, the ready state has
lower priority and longer frame delays, so it frees up more resources if
needed.

Flywheel is `active` when the flywheel rpm is being readjusted. Its task's
priority is set to `priorityActive` and the frame delays by `frameDelayActive`
(in milliseconds).

The way the flywheel changes between states is configured by the last three
settings. The condition to enter `ready` state is:

 - The difference between the target rpm and the real rpm is within
   `±readyErrorInterval`.
 - The rate of change of the real rpm is within `±readyDerivativeInterval`.

If the conditions are not satisfied, the flywheel returns to `active` state.

The flywheel checks the conditions once every cycle of `readyCheckCycle` number
of frames.

As a side note: you can wait for when the flywheel becomes ready by calling
the function [`waitUntilFlywheelReady(flywheel)`]().

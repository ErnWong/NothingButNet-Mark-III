# Flywheel Setup and Initialisation

This page is about the different settings you need to supply to use
a [flywheel][].

[flywheel]: flywheel

To use a `Flywheel`, first create a `FlywheelSetup` object and fill its
members in. Then pass this object into the `flywheelInit` function, and it
will return a `Flywheel` object as a pointer:

```c
FlywheelSetup setup =
{
    .id = "my-flywheel",
    .pigeon = pigeon,

    .gearing = 5.0f,
    .smoothing = 1.0f,

    .controlSetup = pidSetup,
    .controlUpdater = pidUpdate,
    .controlResetter = pidReset,
    .control = pidInit(1.0f, 2.0f, 3.0f),

    .encoderGetter = encoderGetter,
    .encoderResetter = encoderResetter,
    .encoder = encoderGetHandle(encoder),

    .motorSetters =
    [
        motorSetter,
        motorSetter,
        smartMotorSetter
    ],
    .motorArgs =
    [
        motorGetHandle(1, false),
        motorGetHandle(2, true),
        smartMotorInit(4, false)
    ],

    .priorityReady = 2,
    .priorityActive = 2,
    .frameDelayReady = 200,
    .frameDelayActive = 20,

    .thresholdError = 1.0f,
    .thresholdDerivative = 1.0f,
    .checkCycle = 20
}
Flywheel * myFlywheel = flywheelInit(setup);
```

### Pigeon id and pigeon

```c
.id = "my-flywheel"
```

The `id` is a null terminated string. The flywheel creates a [Pigeon
Portal][] with this id, so you can use this id to communicate with the
flywheel via the serial port.

[Pigeon Portal]: pigeon#portal

```c
.pigeon = pigeon
```

The `pigeon` is a pointer to a [pigeon][]. The flywheel creates the portal
with this pigeon.

To tell the flywheel to not use a pigeon, simply set `pigeon = NULL` and it
will be ignored.

For more info, check out [Pigeon#id][].

For more info about the flywheel pigeon portal and what it can do, check out
[Flywheel Portal][].

[pigeon]:           pigeon
[Pigeon#id]:        pigeon#id
[Flywheel Portal]:  flywheel-portal

### External input/outputs

#### Flywheel RPM input parameters

```c
.gearing = 1.0f,
```

The `gearing` is a float. The flywheel calculates the flywheel's rpm by
multiplying the encoder's rpm by `gearing`. If the gear ratio between the
encoder and flywheel is 1:25, then `gearing = 25.0f`.

```c
.smoothing = 1.0f
```

The `smoothing` is another float. The calculated rpms are usually noisy and
discrete because the encoder readings are discrete integer ticks divided by
a fairly constant time difference. Hence, the flywheel smooths out this raw
rpm value using a [low pass filter][lowpass], also known as an [exponentially
weighted moving average][expaverage].

The `smoothing` sets this filter's time constant parameter measured in
seconds. The larger the `smoothing`, the more smooth the result obtained.
However, larger `smoothing` also result in a more delayed rpm signal as
smoothing averages the past signal and not the future.

Smoothing prevents the control algorithms to detect a false condition and
misbehave.

[lowpass]:      https://en.wikipedia.org/wiki/Low-pass_filter
[expaverage]:   https://en.wikipedia.org/wiki/Exponential_smoothing

#### Flywheel RPM input shims

```c
.encoderGetter = encoderGetter,
.encoderResetter = encoderResetter,
.encoder = encoderGetHandle(encoder)
```

Flywheel uses [shims][] to access encoders, so that the flywheel rpm could be
measured by anything including [shaft encoder][], [IME module][], or even data
tethered in.

An `encoderGetter` is an `EncoderGetter` shim, a pointer to a function that
returns an encoder's angle and rpm.

An `encoderResetter` is an `EncoderResetter` shim, that points to a function
that resets the state of an encoder shim.

Both shim functions require an `EncoderHandle`, which is basically an opaque
pointer (`void*`), so the shim functions know which encoder to use and what
state the encoder is in.

Shims for common encoder types are provided:

| Shaft Encoder                 | IME Module                           |
|-------------------------------|--------------------------------------|
| [encoderGetter][]             | [imeGetter][]                        |
| [encoderResetter][]           | [imeResetter][]                      |
| [encoderGetHandle][](encoder) | [imeGetHandle][](address, motorType) |

[shims]:            shims
[shaft encoder]:    http://www.vexrobotics.com/276-2156.html
[IME module]:       http://www.vexrobotics.com/encoder-modules.html
[encoderGetter]:    shims#encoder-getter
[encoderResetter]:  shims#encoder-resetter
[encoderGetHandle]: shims#encoder-gethandle
[imeGetter]:        shims#ime-getter
[imeResetter]:      shims#ime-resetter
[imeGetHandle]:     shims#ime-gethandle

#### Motor output

```c
.motorSetters = []
.motors = []
```

The `motorSetters` is an array of [motor shim] functions that sets the motor
command to a motor port. These shims uses the corresponding handle from the
`motors` array.

Two shims are provided separately:

| Motor              | Smart Motor         |
|--------------------|---------------------|
| [shims.h][]        | [motor.h][]         |
| [motorSetter][]    | [smartMotorSetter[] |
| [motorGetHandle][] | [smartMotorInit][]  |

List your motor setters and motor handles in the arrays, **with at most 8 motors**.

Note that the flywheel stops looking at the first `NULL` entry of the arrays, so
you should not leave gaps in the array.

[motor shim]:       shims#motor
[shims.h]:          shims
[motorSetter]:      shims#motor-setter
[motorGetHandle]:   shims#motor-gethandle
[motor.h]:          motors
[smartMotorSetter]: motors#setter
[smartMotorInit]:   motors#init

### Flywheel ready/active-state settings

```c
.priorityReady = 2,
.priorityActive = 2,

.frameDelayReady = 200,
.frameDelayActive = 20,

.thresholdError = 1.0f,
.thresholdDerivative = 1.0f,
.checkCycle = 20
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
   `±thresholdError`.
 - The rate of change of the real rpm is within `±thresholdDerivative`.

If the conditions are not satisfied, the flywheel returns to `active` state.

The flywheel checks the conditions once every cycle of `checkCycle` number
of frames.

As a side note, you can wait for when the flywheel becomes ready by calling
the function [waitUntilFlywheelReady(flywheel)][]).

[waitUntilFlywheelReady(flywheel)]: flywheel#wait

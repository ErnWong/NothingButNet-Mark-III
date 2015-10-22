#ifndef FLYWHEEL_H_
#define FLYWHEEL_H_

#include <stdbool.h>
#include "pigeon.h"
#include "control.h"
#include "shims.h"

#ifdef __cplusplus
extern "C" {
#endif



// Typedefs {{{

struct Flywheel;
typedef struct Flywheel Flywheel;

typedef enum
FlywheelController
{
    FLYWHEEL_BANGBANG,
    FLYWHEEL_PID,
    FLYWHEEL_TBH
}
FlywheelController;

typedef struct
FlywheelSetup
{
    char id[PIGEON_KEYSIZE];
    float gearing;
    float smoothing;

    ControlUpdater controlUpdater;
    ControlResetter controlResetter;
    void * control;

    EncoderGetter encoderGetter;
    EncoderResetter encoderResetter;
    void * encoder;

    MotorSetter motorSetters[8];
    void * motors[8];

    unsigned int priorityReady;
    unsigned int priorityActive;
    unsigned long frameDelayReady;
    unsigned long frameDelayActive;

    float readyErrorInterval;
    float readyDerivativeInterval;
    int readyCheckCycle;
}
FlywheelSetup;

// }}}



// Methods {{{

Flywheel *
flywheelInit(FlywheelSetup setup);

void
flywheelRun(Flywheel *flywheel);

void
flywheelSet(Flywheel *flywheel, float rpm);

void
waitUntilFlywheelReady(Flywheel *flywheel, const unsigned long blockTime);

// }}}



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

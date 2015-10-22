#ifndef FLYWHEEL_H_
#define FLYWHEEL_H_

#include <stdbool.h>
#include "pigeon.h"
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

    float pidKp;
    float pidKi;
    float pidKd;
    float tbhGain;
    float bbValue;
    float bbAbove;
    float bbBelow;
    FlywheelController controllerType;

    EncoderGetter encoderGetter;
    EncoderResetter encoderResetter;
    void * encoderArgs;

    MotorSetter motorSetters[8];
    void * motorArgs[8];

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

Flywheel *flywheelInit(FlywheelSetup setup);

void flywheelRun(Flywheel *flywheel);

// Sets target RPM
void flywheelSet(Flywheel *flywheel, float rpm, float estimate);

void flywheelSetController(Flywheel *flywheel, FlywheelController type);
void flywheelSetSmoothing(Flywheel *flywheel, float smoothing);
void flywheelSetPidKp(Flywheel *flywheel, float gain);
void flywheelSetPidKi(Flywheel *flywheel, float gain);
void flywheelSetPidKd(Flywheel *flywheel, float gain);
void flywheelSetTbhGain(Flywheel *flywheel, float gain);
void waitUntilFlywheelReady(Flywheel *flywheel, const unsigned long blockTime);

// }}}



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

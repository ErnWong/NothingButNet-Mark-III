#ifndef FLYWHEEL_H_
#define FLYWHEEL_H_

#include <stdbool.h>

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

typedef void (*FlywheelMotorSetter)(void *target, int command);

typedef struct
FlywheelSetup
{
    char id[8];
    float gearing;                      // Ratio of flywheel RPM per encoder RPM.
    float pidKp;
    float pidKi;
    float pidKd;
    float tbhGain;
    float tbhApprox;
    float bangBangValue;
    float smoothing;                    // Amount of smoothing applied to the flywheel RPM, as the low-pass time constant in seconds.
    FlywheelController controllerType;
    float encoderTicksPerRevolution;    // Number of ticks each time the encoder completes one revolution
    unsigned char encoderPortTop;       // Digital port number where the encoder's top wire is connected.
    unsigned char encoderPortBottom;    // Digital port number where the encoder's bottom wire is connected.
    FlywheelMotorSetter motorSetters[8];
    void * motorTargets[8];
    bool encoderReverse;                // Whether the encoder values should be reversed.
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
void flywheelSet(Flywheel *flywheel, float rpm);

void flywheelSetController(Flywheel *flywheel, FlywheelController type);
void flywheelSetSmoothing(Flywheel *flywheel, float smoothing);
void flywheelSetPidKp(Flywheel *flywheel, float gain);
void flywheelSetPidKi(Flywheel *flywheel, float gain);
void flywheelSetPidKd(Flywheel *flywheel, float gain);
void flywheelSetTbhGain(Flywheel *flywheel, float gain);
void flywheelSetTbhApprox(Flywheel *flywheel, float approx);
void waitUntilFlywheelReady(Flywheel *flywheel, const unsigned long blockTime);

// }}}



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

//
// Motor model.
//
// This is part of a rewrite of James Pearman (aka Jpearman)'s Smart Motor Library,
// which is based on and uses Chris Siegert (aka vamfun)'s model of the motor and PTC.
// https://vamfun.wordpress.com/2012/07/18/estimating-the-ptc-temperature-using-motor-current-first-try-no-luck/
// https://vamfun.wordpress.com/2012/07/21/derivation-of-formulas-to-estimate-h-bridge-controller-current-vex-jaguarvictor-draft/
// https://github.com/jpearman/smartMotorLib/blob/master/SmartMotorLib.c
// http://www.vexforum.com/showthread.php?t=74659
// http://www.vexforum.com/showthread.php?t=73960
//


#ifndef MOTOR_MODEL_H_
#define MOTOR_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


extern const float MOTOR_SYSTEM_RESISTANCE;
extern const float MOTOR_PWM_FREQUENCY;
extern const float MOTOR_DIODE_VOLTAGE;
extern const int MOTOR_COMMAND_MAX;

typedef struct MotorModel
{
    int command;
    int direction;

    float dutyOn;
    float dutyOff;

    float backEmfMax;
    float backEmfPerRpm;
    float backEmf;

    float resistance;
    float inductance;

    float currentSteadyStateOn;
    float currentSteadyStateOff;

    float lambda;

    float contributionPeak;
    float contributionInitial;

    float currentInitial;
    float currentPeak;

    float smoothing;
    float current;
    float currentFiltered;

    bool commandNeedsScaling;
}
MotorModel;



typedef struct MotorModelSetup
{
    float backEmfPerRpm;
    float resistance;
    float inductance;
    float smoothing;
    float rpmFree;
    unsigned char channel;
}
MotorModelSetup;


typedef struct MotorModelMeasurements
{
    int command;
    float rpm;
    float batteryVoltage;
    float timeChange;
}
MotorModelMeasurements;


void
motorModelInit(MotorModel*, MotorModelSetup);

// Also returns the amount of current.
float
motorModelUpdate(MotorModel*, MotorModelMeasurements);


// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

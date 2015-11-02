#ifndef CONTROL_H_
#define CONTROL_H_

#include "pigeon.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef struct
ControlSystem
{
    unsigned long microTime;
    float dt;
    float target;
    float measured;
    float derivative;
    float error;
    float action;
}
ControlSystem;

typedef void * ControlHandle;

typedef float
(*ControlUpdater)(ControlHandle, ControlSystem);

typedef void
(*ControlResetter)(ControlHandle);

typedef void
(*ControlSetup)(ControlHandle, Portal*);

typedef float
(*TbhEstimator)(float target);

ControlHandle
pidInit(float gainP, float gainI, float gainD);

void
pidReset(ControlHandle);

float
pidUpdate(ControlHandle, ControlSystem*);

void
pidSetup(ControlHandle, Portal*);

ControlHandle
tbhInit(float gain, TbhEstimator);

void
tbhReset(ControlHandle);

float
tbhUpdate(ControlHandle, ControlSystem*);

void
tbhSetup(ControlHandle, Portal*);

float
tbhDummyEstimator(float target);

void *
bangBangInit
(
    float actionHigh,
    float actionLow,
    float triggerHigh,
    float triggerLow
);

void
bangBangReset(ControlHandle);

float
bangBangUpdate(ControlHandle, ControlSystem*);

void
bangBangSetup(ControlHandle, Portal*);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

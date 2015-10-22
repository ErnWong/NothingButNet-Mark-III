#include <stdlib.h>
#include <stdbool.h>

#include "control.h"
#include "utils.h"


#define UNUSED(x) (void)(x)


// PID Controller {{{

typedef struct
Pid
{
    float gainP;
    float gainI;
    float gainD;
    float integral;
}
Pid;

ControlHandle
pidInit(float gainP, float gainI, float gainD)
{
    Pid * pid = malloc(sizeof(Pid));
    pid->gainP = gainP;
    pid->gainI = gainI;
    pid->gainD = gainD;
    pidReset(pid);
    return pid;
}

void
pidReset(ControlHandle handle)
{
    Pid * pid = handle;
    pid->integral = 0;
}

float
pidUpdate(ControlHandle handle, ControlSystem * system)
{
    Pid * pid = handle;

    pid->integral += system->error * system->dt;

    float partP = pid->gainP * system->error;
    float partI = pid->gainI * system->error;
    float partD = pid->gainD * system->error;

    system->action = partP + partI + partD;
    return system->action;
}

// }}}


// TBH Controller {{{

typedef struct
Tbh
{
    TbhEstimator estimator;
    float gain;
    float lastAction;
    float lastError;
    float lastTarget;
    bool crossed;
}
Tbh;

ControlHandle
tbhInit(float gain, TbhEstimator estimator)
{
    Tbh * tbh = malloc(sizeof(tbh));
    tbh->estimator = estimator;
    tbh->gain = gain;
    tbhReset(tbh);
    return tbh;
}

void
tbhReset(ControlHandle handle)
{
    Tbh * tbh = handle;
    tbh->lastAction = 0.0f;
    tbh->lastError = 0.0f;
    tbh->lastTarget = 0.0f;
    tbh->crossed = false;
}

float
tbhUpdate(ControlHandle handle, ControlSystem * system)
{
    Tbh * tbh = handle;
    system->action += system->error * system->dt * tbh->gain;
    if (system->target != tbh->lastTarget)
    {
        tbh->crossed = false;
        tbh->lastTarget = system->target;
    }
    if (signOf(system->error) != signOf(tbh->lastError))
    {
        if (!tbh->crossed)
        {
            system->action = tbh->estimator(system->target);
            tbh->crossed = true;
        }
        else
        {
            system->action = 0.5f * (system->action + tbh->lastAction);
        }
        tbh->lastAction = system->action;
    }
    tbh->lastError = system->error;
    return system->action;
}

// }}}


// Bang Bang Controller {{{

typedef struct
BangBang
{
    float actionHigh;
    float actionLow;
    float triggerHigh;
    float triggerLow;
}
BangBang;

ControlHandle
bangBangInit
(
    float actionHigh,
    float actionLow,
    float triggerHigh,
    float triggerLow
){
    BangBang * bb = malloc(sizeof(BangBang));
    bb->actionHigh = actionHigh;
    bb->actionLow = actionLow;
    bb->triggerHigh = triggerHigh;
    bb->triggerLow = triggerLow;
    return bb;
}

void
bangBangReset(ControlHandle handle)
{
    UNUSED(handle);
}

float
bangBangUpdate(ControlHandle handle, ControlSystem * system)
{
    BangBang * bb = handle;
    if (system->error > bb->triggerHigh)
    {
        system->action = bb->actionHigh;
    }
    else if (system->error < bb->triggerLow)
    {
        system->action = bb->actionLow;
    }
    return system->action;
}

// }}}

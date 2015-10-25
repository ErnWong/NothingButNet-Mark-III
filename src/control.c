#include <stdlib.h>
#include <stdbool.h>

#include "control.h"
#include "pigeon.h"
#include "utils.h"


#define UNUSED(x) (void)(x)


// PID Controller {{{

typedef struct
Pid
{
    Portal * portal;
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
    pid->portal = NULL;
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
    portalUpdate(pid->portal, "integral");
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

    portalUpdate(pid->portal, "integral");

    return system->action;
}

void
pidSetup(ControlHandle handle, Portal * portal)
{
    Pid * pid = handle;
    pid->portal = portal;
    PortalEntrySetup setups[] =
    {
        {
            .key = "gain-p",
            .handler = portalFloatHandler,
            .handle = &pid->gainP
        },
        {
            .key = "gain-i",
            .handler = portalFloatHandler,
            .handle = &pid->gainI
        },
        {
            .key = "gain-d",
            .handler = portalFloatHandler,
            .handle = &pid->gainD
        },
        {
            .key = "integral",
            .handler = portalFloatHandler,
            .handle = &pid->integral
        }
    };
    portalAddBatch(portal, setups);
}

// }}}


// TBH Controller {{{

typedef struct
Tbh
{
    Portal * portal;
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
    tbh->portal = NULL;
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
    portalUpdate(tbh->portal, "last-action");
    portalUpdate(tbh->portal, "last-error");
    portalUpdate(tbh->portal, "last-target");
    portalUpdate(tbh->portal, "crossed");
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
        portalUpdate(tbh->portal, "crossed");
        portalUpdate(tbh->portal, "last-target");
    }
    if (signOf(system->error) != signOf(tbh->lastError))
    {
        if (!tbh->crossed)
        {
            system->action = tbh->estimator(system->target);
            tbh->crossed = true;
            portalUpdate(tbh->portal, "crossed");
        }
        else
        {
            system->action = 0.5f * (system->action + tbh->lastAction);
        }
        tbh->lastAction = system->action;
        portalUpdate(tbh->portal, "last-action");
    }
    tbh->lastError = system->error;
    portalUpdate(tbh->portal, "last-error");
    return system->action;
}

void
tbhSetup(ControlHandle handle, Portal * portal)
{
    Tbh * tbh = handle;
    tbh->portal = portal;
    PortalEntrySetup setups[] =
    {
        {
            .key = "gain",
            .handler = portalFloatHandler,
            .handle = &tbh->gain
        },
        {
            .key = "last-action",
            .handler = portalFloatHandler,
            .handle = &tbh->lastAction
        },
        {
            .key = "last-error",
            .handler = portalFloatHandler,
            .handle = &tbh->lastError
        },
        {
            .key = "last-target",
            .handler = portalFloatHandler,
            .handle = &tbh->lastTarget
        },
        {
            .key = "crossed",
            .handler = portalBoolHandler,
            .handle = &tbh->crossed
        }
    };
    portalAddBatch(portal, setups);
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
        system->action = bb->actionLow;
    }
    else if (system->error < bb->triggerLow)
    {
        system->action = bb->actionHigh;
    }
    return system->action;
}

void
bangBangSetup(ControlHandle handle, Portal * portal)
{
    BangBang * bb = handle;
    PortalEntrySetup setups[] =
    {
        {
            .key = "action-high",
            .handler = portalFloatHandler,
            .handle = &bb->actionHigh
        },
        {
            .key = "action-low",
            .handler = portalFloatHandler,
            .handle = &bb->actionLow
        },
        {
            .key = "trigger-high",
            .handler = portalFloatHandler,
            .handle = &bb->triggerHigh
        },
        {
            .key = "trigger-low",
            .handler = portalFloatHandler,
            .handle = &bb->triggerLow
        }
    };
    portalAddBatch(portal, setups);
}

// }}}

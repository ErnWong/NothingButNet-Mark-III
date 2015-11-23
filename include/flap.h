#ifndef FLAP_H_
#define FLAP_H_

#include "pigeon.h"
#include "shims.h"

#ifdef __cplusplus
extern "C" {
#endif



struct Flap;
typedef struct Flap Flap;

typedef enum
FlapState
{
    FLAP_CLOSED,
    FLAP_OPENING,
    FLAP_OPENED,
    FLAP_CLOSING
}
FlapState;

typedef struct
FlapSetup
{
    char * id;
    Pigeon * pigeon;

    MotorSetter motorSetter;
    MotorHandle motor;
    DigitalGetter digitalOpenedGetter;
    DigitalHandle digitalOpened;
    DigitalGetter digitalClosedGetter;
    DigitalHandle digitalClosed;

    FlapState initialState;

    unsigned int priorityReady;
    unsigned int priorityActive;
    unsigned int priorityDrop;
    unsigned long frameDelayReady;
    unsigned long frameDelayActive;
    unsigned long dropDelay;
}
FlapSetup;

Flap *
flapInit(FlapSetup);

void
flapOpen(Flap*);

void
flapClose(Flap*);

void
flapDrop(Flap*);

void
waitUntilFlapOpened(Flap*);

void
waitUntilFlapClosed(Flap*);



#ifdef __cplusplus
}
#endif

// End include guard
#endif

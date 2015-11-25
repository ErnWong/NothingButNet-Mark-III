#ifndef RECKONER_H_
#define RECKONER_H_

#include "pigeon.h"
#include "shims.h"

#ifdef __cplusplus
extern "C" {
#endif



struct Reckoner;
typedef struct Reckoner Reckoner;

typedef struct
ReckonerState
{
    float velocity;
    float heading;
    float x;
    float y;
}
ReckonerState;

typedef struct
ReckonerSetup
{
    char * id;
    Pigeon * pigeon;

    float initialX;
    float initialY;
    float initialHeading;
    float initialVelocity;

    float gearingLeft;
    float gearingRight;
    float radiusLeft;
    float radiusRight;
    float wheelSeparation;
    float smoothing;

    EncoderGetter encoderLeftGetter;
    EncoderHandle encoderLeft;
    EncoderGetter encoderRightGetter;
    EncoderHandle encoderRight;
}
ReckonerSetup;

Reckoner *
reckonerInit(ReckonerSetup);

void
reckonerUpdate(Reckoner*);

ReckonerState *
reckonerGetState(Reckoner*);



// End C++ export structure
#ifdef __cplusplus
}
#endif

//End include guard
#endif

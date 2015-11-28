#ifndef DIFFSTEER_H_
#define DIFFSTEER_H_

#include "pigeon.h"
#include "shims.h"

#ifdef __cplusplus
extern "C" {
#endif



struct Diffsteer;
typedef struct Diffsteer;

typedef enum
DiffsteerMode
{
    DIFFSTEER_IDLE,
    DIFFSTEER_ROTATING,
    DIFFSTEER_MOVING
}
DiffsteerMode;

typedef struct
DiffsteerSetup
{
    char * id;
    Pigeon * pigeon;

    float gainDistance;
    float gainHeading;

    MotorSetter motorLeftSetter;
    MotorHandle motorLeft;
    MotorSetter motorRightSetter;
    MotorHandle motorRight;
}
DiffsteerSetup;

void
diffsteerRotate(Diffsteer*, float heading);

void
diffsteerMove(Diffsteer*, float x, float y);

void
diffsteerStop(Diffsteer*);

void
diffsteerUpdate(Diffsteer*);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

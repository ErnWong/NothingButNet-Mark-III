#ifndef CONVEYOR_H_
#define CONVEYOR_H_

#include "pigeon.h"
#include "shims.h"

#ifdef __cplusplus
extern "C" {
#endif



struct Conveyor;
typedef struct Conveyor Conveyor;

typedef enum
ConveyorState
{
    CONVEYOR_IDLE,
    CONVEYOR_PICKUP,
    CONVEYOR_PICKUP_RETURN,
    CONVEYOR_PUSHIN,
    CONVEYOR_PREPARE_FEED,
    CONVEYOR_FEEDING,
    CONVEYOR_EJECTING
}
ConveyorState;

typedef struct
ConveyorSetup
{
    char * id;
    Pigeon * pigeon;

    MotorSetter motorSetter;
    MotorHandle motor;
    DigitalGetter sIntakeOuterGetter;
    DigitalHandle sIntakeOuter;
    DigitalGetter sIntakeInnerGetter;
    DigitalHandle sIntakeInner;
    DigitalGetter sFeedGetter;
    DigitalHandle sFeed;

    int initialBallCount;
    ConveyorState initialState;
}
ConveyorSetup;

void
conveyorStop(Conveyor*);

void
conveyorPickup(Conveyor*);

void
conveyorPrepareFeed(Conveyor*);

void
conveyorFeed(Conveyor*);

void
conveyorEject(Conveyor*);

void
conveyorUpdate(Conveyor*);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

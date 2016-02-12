#include "conveyor.h"

#include <API.h>
#include <string.h>
#include <stdbool.h>
#include "pigeon.h"
#include "shims.h"

struct Conveyor
{
    Portal * portal;

    MotorSetter motorSet;
    MotorHandle motor;
    DigitalGetter sIntakeOuterGet;
    DigitalHandle sIntakeOuter;
    bool sIntakeOuterPrev;
    DigitalGetter sIntakeInnerGet;
    DigitalHandle sIntakeInner;
    bool sIntakeInnerPrev;
    DigitalGetter sFeedGet;
    DigitalHandle sFeed;
    bool sFeedPrev;

    int ballCount;
    ConveyorState state;
};

static void update(Conveyor*);
static void initPortal(Conveyor*, ConveyorSetup);
static void stateHandler(void * handle, char * message, char * response);

Conveyor *
conveyorInit(ConveyorSetup setup)
{
    Conveyor * conveyor = malloc(sizeof(Conveyor));

    initPortal(conveyor, setup);

    conveyor->motorSet = setup.motorSetter;
    conveyor->motor = setup.motor;
    conveyor->sIntakeOuterGet = setup.sIntakeOuterGetter;
    conveyor->sIntakeOuter = setup.sIntakeOuter;
    conveyor->sIntakeInnerGet = setup.sIntakeInnerGetter;
    conveyor->sIntakeInner = setup.sIntakeInner;
    conveyor->sFeedGet = setup.sFeedGetter;
    conveyor->sFeed = setup.sFeed;

    conveyor->ballCount = setup.initialBallCount;
    conveyor->state = setup.initialState;

    return conveyor;
}

void
conveyorStop(Conveyor * conveyor)
{
    conveyor->state = CONVEYOR_IDLE;
}

void
conveyorPickup(Conveyor * conveyor)
{
    bool isBallNearIntake = conveyor->sIntakeOuterGet(conveyor->sIntakeOuter);
    if (isBallNearIntake)
    {
        // To prevent treating ball as new ball
        conveyor->state = CONVEYOR_PICKUP_RETURN;
    }
    else
    {
        conveyor->state = CONVEYOR_PICKUP;
    }
}

void
conveyorPrepareFeed(Conveyor * conveyor)
{
    conveyor->state = CONVEYOR_PREPARE_FEED;
}

void
conveyorFeed(Conveyor * conveyor)
{
    conveyor->state = CONVEYOR_FEEDING;
}

void
conveyorEject(Conveyor * conveyor)
{
    conveyor->state = CONVEYOR_EJECTING;
}

void
conveyorUpdate(Conveyor * conveyor)
{
    update(conveyor);
}

static void
update(Conveyor * conveyor)
{
    int command = 0;

    // 1. Get the needed sensor information

    bool sIntakeOuter = conveyor->sIntakeOuterGet(conveyor->sIntakeOuter);
    bool sIntakeInner = conveyor->sIntakeInnerGet(conveyor->sIntakeInner);
    bool sFeed = conveyor->sFeedGet(conveyor->sFeed);

    bool sIntakeOuterOndown = sIntakeOuter && !conveyor->sIntakeOuterPrev;
    bool sIntakeOuterOnup = !sIntakeOuter && conveyor->sIntakeOuterPrev;
    bool sIntakeInnerOnup = !sIntakeInner && conveyor->sIntakeInnerPrev;
    bool sFeedOnup = !sFeed && conveyor->sFeedPrev;

    // 2. Behaviour depending on state

    switch (conveyor->state)
    {

    case CONVEYOR_IDLE:
        command = 0;
        break;

    case CONVEYOR_PICKUP:
        command = 100;
        if (sIntakeOuterOndown)
        {
            conveyor->ballCount++;
            conveyor->state = CONVEYOR_PUSHIN;
        }
        else if (sIntakeInner)
        {
            conveyor->state = CONVEYOR_PICKUP_RETURN;
        }
        break;

    case CONVEYOR_PICKUP_RETURN:
        command = -127;
        if (sIntakeOuter)
        {
            conveyor->state = CONVEYOR_PICKUP;
        }
        break;

    case CONVEYOR_PUSHIN:
        command = 127;
        if (sIntakeInnerOnup)
        {
            conveyor->state = CONVEYOR_PICKUP;
        }
        break;

    case CONVEYOR_PREPARE_FEED:
        command = 127;
        if (sFeed)
        {
            conveyor->state = CONVEYOR_IDLE;
        }

    case CONVEYOR_FEEDING:
        command = 127;
        if (sFeedOnup)
        {
            conveyor->ballCount--;
            conveyor->state = CONVEYOR_IDLE;
        }
        break;

    case CONVEYOR_EJECTING:
        command = -127;
        if (sIntakeOuterOnup)
        {
            conveyor->ballCount--;
            conveyor->state = CONVEYOR_IDLE;
        }
        break;

    }

    // 4. Update

    conveyor->motorSet(conveyor->motor, command);

    conveyor->sIntakeOuterPrev = sIntakeOuter;
    conveyor->sIntakeInnerPrev = sIntakeInner;
    conveyor->sFeedPrev = sFeed;
}

static void initPortal(Conveyor * conveyor, ConveyorSetup setup)
{
    conveyor->portal = pigeonCreatePortal(setup.pigeon, setup.id);

    PortalEntrySetup setups[] =
    {
        {
            .key = "ball-count",
            .handler = portalIntHandler,
            .handle = &conveyor->ballCount
        },
        {
            .key = "state",
            .handler = stateHandler,
            .handle = conveyor
        },

        // End terminating struct
        {
            .key = "~",
            .handler = NULL,
            .handle = NULL
        }
    };
    portalAddBatch(conveyor->portal, setups);
    portalReady(conveyor->portal);
}

static void
stateHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (response == NULL) return;
    Conveyor * conveyor = handle;
    if (message == NULL)
    {
        switch (conveyor->state)
        {
        case CONVEYOR_IDLE:
            strcpy(response, "idle");
            break;
        case CONVEYOR_PICKUP:
            strcpy(response, "pickup");
            break;
        case CONVEYOR_PICKUP_RETURN:
            strcpy(response, "pickup-return");
            break;
        case CONVEYOR_PUSHIN:
            strcpy(response, "pushin");
            break;
        case CONVEYOR_PREPARE_FEED:
            strcpy(response, "prepare-feed");
            break;
        case CONVEYOR_FEEDING:
            strcpy(response, "feeding");
            break;
        case CONVEYOR_EJECTING:
            strcpy(response, "ejecting");
            break;
        }
    }
    else if (strcmp(message, "idle") == 0) conveyorStop(conveyor);
    else if (strcmp(message, "pickup") == 0) conveyorPickup(conveyor);
    else if (strcmp(message, "prepare-feed") == 0) conveyorPrepareFeed(conveyor);
    else if (strcmp(message, "feed") == 0) conveyorFeed(conveyor);
    else if (strcmp(message, "eject") == 0) conveyorEject(conveyor);
}

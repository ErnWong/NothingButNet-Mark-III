#include "diffsteer-control.h"

#include <API.h>
#include <math.h>
#include "reckoner.h"
#include "shims.h"
#include "pigeon.h"

struct Diffsteer
{
    Portal * portal;

    ReckonerState * state;

    DiffsteerMode mode;
    float targetX;
    float targetY;
    float targetHeading;

    float gainDistance;
    float gainHeading;

    MotorSetter motorLeftSet;
    MotorHandle motorLeft;
    MotorSetter motorRightSet;
    MotorHandle motorRight;

    Mutex mutex;
};

static void updateRotate(Diffsteer*);
static void updateMove(Diffsteer*);
static void setupPortal(Diffsteer*, DiffsteerSetup);
static void modeHandler(void * handle, char * message, char * response);

Diffsteer *
diffsteerInit(DiffsteerSetup setup)
{
    Diffsteer * d = malloc(sizeof(Diffsteer));

    setupPortal(d, setup);

    d->mode = DIFFSTEER_IDLE;
    d->targetX = 0.0f;
    d->targetY = 0.0f;
    d->targetHeading = 0.0f;

    d->gainDistance = setup.gainDistance;
    d->gainHeading = setup.gainHeading;

    d->motorLeftSet = setup.motorLeftSetter;
    d->motorLeft = setup.motorLeft;
    d->motorRightSet = setup.motorRightSetter;
    d->motorRight = setup.mtorRight;

    d->mutex = mutexCreate();
}

void
diffsteerRotate(Diffsteer * d, float heading)
{
    mutexTake(d->mutex);
    d->mode = DIFFSTEER_ROTATING;
    heading = fmodf(heading + PI, TAU) - PI;
    d->targetHeading = heading;
    mutexGive(d->mutex);
}

void
diffsteerMove(Diffsteer * d, float x, float y)
{
    mutexTake(d->mutex);
    d->mode = DIFFSTEER_MOVING;
    d->targetX = x;
    d->targetY = y;
    mutexGive(d->mutex);
}

void
diffsteerStop(Diffsteer * d)
{
    mutexTake(d->mutex);
    d->mode = DIFFSTEER_IDLE;
    mutexGive(d->mutex);
}

void
diffsteerUpdate(Diffsteer * d)
{
    mutexTake(d->mutex);
    switch (d->mode)
    {
    case DIFFSTEER_ROTATING:
        updateRotate(d);
        break;
    case DIFFSTEER_MOVING:
        updateMove(d);
        break;
    }
    mutexGive(d->mutex);
}

static void
updateRotate(Diffsteer * d)
{
    float errorHeading = d->targetHeading - d->state->heading;
    float command = errorHeading * 0.5f * d->gainHeading;

    d->motorLeftSet(d-motorLeft, -(int)command);
    d->motorRightSet(d-motorRight, (int)command);
}

static void
updateMove(Diffsteer * d)
{
    float errorX = d->targetX - d->state->x;
    float errorY = d->targetY - d->state->y;
    float distance = sqrtf(errorX * errorX + errorY * errorY);

    float targetHeading = atan2f(errorY, errorX);
    float errorHeading = targetHeading - d->state->heading;

    float command = distance * d->gainDistance;
    command *= cosf(errorHeading);

    float commandDiff = errorHeading * d->gainHeading;
    float commandLeft = command - 0.5f * commandDiff;
    float commandRight = command + 0.5f * commandDiff;

    float * commandHigher;
    float * commandLower;

    if (commandDiff > 0)
    {
        commandHigher = &commandRight;
        commandLower = &commandLeft;
    }
    if (commandHigher > 127.0f)
    {
        commandHigher = 127.0f;
        commandLower = 127.0f - fabsf(commandDiff);
    }
    if (commandLower < -127.0f)
    {
        commandLower = -127.0f;
        commandHigher = -127.0f + fabsf(commandDiff);
    }

    d->motorLeftSet(d->motorLeft, (int)commandLeft);
    d->motorRightSet(d->motorRight, (int)commandRight);
}

static void
setupPortal(Diffsteer * d, DiffsteerSetup setup)
{
    d->portal = pigeonCreatePortal(setup.pigeon, setup.id);

    PortalEntrySetup setups[] =
    {
        {
            .key = "mode",
            .handler = modeHandler,
            .handle = d,
            .onchange = true
        },
        {
            .key = "target-x",
            .handler = portalFloatHandler,
            .handle = &d->targetX,
            .onchange = true
        },
        {
            .key = "target-y",
            .handler = portalFloatHandler,
            .handle = &d->targetY,
            .onchange = true
        },
        {
            .key = "target-heading",
            .handler = portalFloatHandler,
            .handle = &d->targetHeading,
            .onchange = true
        },
        {
            .key = "gain-distance",
            .handler = portalFloatHandler,
            .handle = &d->gainDistance
        },
        {
            .key = "gain-heading",
            .handler = portalFloatHandler,
            .handle = &d->gainHeading
        },

        {
            .key = "~",
            .handler = NULL,
            .handle = NULL
        }
    };
    portalAddBatch(d->portal, setups);
}


static void
modeHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (response == NULL) return;
    Diffsteer * d = handle;
    if (message == NULL)
    {
        switch(d->mode)
        {
        case DIFFSTEER_IDLE:
            strcpy(response, "idle");
            break;
        case DIFFSTEER_ROTATING:
            strcpy(response, "rotating");
            break;
        case DIFFSTEER_MOVING:
            strcpy(response, "moving");
            break;
        }
    }
    else if (strcmp(message, "idle") == 0) diffsteerStop(d);
}

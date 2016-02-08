#include "diffsteer-control.h"

#include <API.h>
#include <math.h>
#include <string.h>
#include "reckoner.h"
#include "shims.h"
#include "pigeon.h"
#include "utils.h"

typedef enum
DiffsteerMode
{
    DIFFSTEER_IDLE,
    DIFFSTEER_ROTATING,
    DIFFSTEER_MOVING
}
DiffsteerMode;

struct Diffsteer
{
    Portal * portal;

    ReckonerState * state;

    DiffsteerMode mode;
    float targetX;
    float targetY;
    float targetHeading;

    bool ready;
    float thresholdDistance;
    float thresholdHeading;

    float gainDistance;
    float gainHeading;

    MotorSetter motorLeftSet;
    MotorHandle motorLeft;
    MotorSetter motorRightSet;
    MotorHandle motorRight;

    Mutex mutex;
    Semaphore readySemaphore;
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

    d->state = setup.state;

    d->mode = DIFFSTEER_IDLE;
    d->targetX = 0.0f;
    d->targetY = 0.0f;
    d->targetHeading = 0.0f;

    d->ready = true;
    d->thresholdDistance = setup.thresholdDistance;
    d->thresholdHeading = setup.thresholdHeading;

    d->gainDistance = setup.gainDistance;
    d->gainHeading = setup.gainHeading;

    d->motorLeftSet = setup.motorLeftSetter;
    d->motorLeft = setup.motorLeft;
    d->motorRightSet = setup.motorRightSetter;
    d->motorRight = setup.motorRight;

    d->mutex = mutexCreate();
    d->readySemaphore = semaphoreCreate();

    return d;
}

void
diffsteerRotate(Diffsteer * d, float heading)
{
    mutexTake(d->mutex, -1);
    d->mode = DIFFSTEER_ROTATING;
    d->ready = false;
    heading = fmodf(heading + PI, TAU) - PI;
    d->targetHeading = heading;
    mutexGive(d->mutex);
}

void
diffsteerMove(Diffsteer * d, float x, float y)
{
    mutexTake(d->mutex, -1);
    d->mode = DIFFSTEER_MOVING;
    d->ready = false;
    d->targetX = x;
    d->targetY = y;
    mutexGive(d->mutex);
}

void
diffsteerStop(Diffsteer * d)
{
    mutexTake(d->mutex, -1);
    d->mode = DIFFSTEER_IDLE;
    d->ready = true;
    mutexGive(d->mutex);
}

void
diffsteerUpdate(Diffsteer * d)
{
    mutexTake(d->mutex, -1);
    switch (d->mode)
    {
    case DIFFSTEER_IDLE:
        // do nothing
        break;
    case DIFFSTEER_ROTATING:
        updateRotate(d);
        break;
    case DIFFSTEER_MOVING:
        updateMove(d);
        break;
    }
    mutexGive(d->mutex);
}

void
waitUntilDiffsteerReady(Diffsteer * d, const unsigned long blockTime)
{
    if (d->ready) return;
    semaphoreTake(d->readySemaphore, blockTime);
}

static void
updateRotate(Diffsteer * d)
{
    float errorHeading = d->targetHeading - d->state->heading;
    float command = errorHeading * 0.5f * d->gainHeading;

    d->motorLeftSet(d->motorLeft, -(int)command);
    d->motorRightSet(d->motorRight, (int)command);

    bool isReady = -d->thresholdHeading <= errorHeading;
    isReady &= errorHeading <= d->thresholdHeading;
    if (isReady && !d->ready)
    {
        d->ready = true;
        semaphoreGive(d->readySemaphore);
    }
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
    if (*commandHigher > 127.0f)
    {
        *commandHigher = 127.0f;
        *commandLower = 127.0f - fabsf(commandDiff);
    }
    if (*commandLower < -127.0f)
    {
        *commandLower = -127.0f;
        *commandHigher = -127.0f + fabsf(commandDiff);
    }

    d->motorLeftSet(d->motorLeft, (int)commandLeft);
    d->motorRightSet(d->motorRight, (int)commandRight);

    bool isReady = -d->thresholdDistance <= distance;
    isReady &= distance <= d->thresholdDistance;
    if (isReady && !d->ready)
    {
        d->ready = true;
        semaphoreGive(d->readySemaphore);
    }
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
    portalReady(d->portal);
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

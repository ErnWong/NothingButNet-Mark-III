#include "flap.h"

#include <API.h>
#include <string.h>
#include <stdbool.h>
#include "pigeon.h"
#include "shims.h"

struct Flap
{
    Portal * portal;

    float slew;
    float highCommand;
    float command;
    MotorSetter motorSet;
    MotorHandle motor;
    DigitalGetter digitalOpenedGet;
    DigitalHandle digitalOpened;
    DigitalGetter digitalClosedGet;
    DigitalHandle digitalClosed;

    FlapState state;

    unsigned int priority;
    unsigned int priorityReady;
    unsigned int priorityActive;
    unsigned int priorityDrop;
    unsigned long frameDelay;
    unsigned long frameDelayReady;
    unsigned long frameDelayActive;
    unsigned long dropDelay;

    Semaphore semaphoreOpened;
    Semaphore semaphoreClosed;

    Mutex mutex;
    TaskHandle task;
    TaskHandle dropTask;
};

static void task(void*);
static void dropTask(void*);
static void update(Flap*);
static void readify(Flap*);
static void activate(Flap*);
static void initPortal(Flap*, FlapSetup);
static void stateHandler(void * handle, char * message, char * response);
static void openHandler(void * handle, char * message, char * response);
static void closeHandler(void * handle, char * message, char * response);

Flap *
flapInit(FlapSetup setup)
{
    Flap * flap = malloc(sizeof(Flap));

    initPortal(flap, setup);

    flap->slew = setup.slew;
    flap->highCommand = setup.highCommand;
    flap->command = 0.0f;

    flap->motorSet = setup.motorSetter;
    flap->motor = setup.motor;
    flap->digitalOpenedGet = setup.digitalOpenedGetter;
    flap->digitalOpened = setup.digitalOpened;
    flap->digitalClosedGet = setup.digitalClosedGetter;
    flap->digitalClosed = setup.digitalClosed;

    flap->state = setup.initialState;

    flap->priority = setup.priorityActive;
    flap->priorityReady = setup.priorityReady;
    flap->priorityActive = setup.priorityActive;
    flap->priorityDrop = setup.priorityDrop;
    flap->frameDelay = setup.frameDelayActive;
    flap->frameDelayReady = setup.frameDelayReady;
    flap->frameDelayActive = setup.frameDelayActive;
    flap->dropDelay = setup.dropDelay;

    flap->semaphoreOpened = semaphoreCreate();
    flap->semaphoreClosed = semaphoreCreate();

    flap->mutex = mutexCreate();
    flap->task = NULL;
    flap->dropTask = NULL;

    return flap;
}

void
flapRun(Flap * flap)
{
    if (flap->task != NULL) return;
    flap->task = taskCreate(
        task,
        TASK_DEFAULT_STACK_SIZE,
        flap,
        flap->priority
    );
}

void
flapOpen(Flap * flap)
{
    mutexTake(flap->mutex, -1);
    flap->state = FLAP_OPENING;
    portalUpdate(flap->portal, "state");
    mutexGive(flap->mutex);
}

void
flapClose(Flap * flap)
{
    mutexTake(flap->mutex, -1);
    flap->state = FLAP_CLOSING;
    portalUpdate(flap->portal, "state");
    mutexGive(flap->mutex);
}

void
flapDrop(Flap * flap)
{
    if (flap->state != FLAP_CLOSED) return;
    if (flap->dropTask != NULL) return;
    flap->dropTask = taskCreate(
        dropTask,
        TASK_DEFAULT_STACK_SIZE,
        flap,
        flap->priorityDrop
    );
}

void
waitUntilFlapOpened(Flap * flap)
{
    if (flap->state == FLAP_OPENED) return;
    semaphoreTake(flap->semaphoreOpened, -1);
}

void
waitUntilFlapClosed(Flap * flap)
{
    if (flap->state == FLAP_CLOSED) return;
    semaphoreTake(flap->semaphoreClosed, -1);
}

static void
task(void * flapPointer)
{
    Flap * flap = flapPointer;
    while (true)
    {
        mutexTake(flap->mutex, -1);
        update(flap);
        mutexGive(flap->mutex);
        delay(flap->frameDelay);
    }
}

static void
dropTask(void * flapPointer)
{
    Flap * flap = flapPointer;
    flapOpen(flap);
    waitUntilFlapOpened(flap);
    delay(flap->dropDelay);
    flapClose(flap);
    flap->dropTask = NULL;
}

static void
update(Flap * flap)
{
    bool isOpened = flap->digitalOpenedGet(flap->digitalOpened);
    bool isClosed = flap->digitalClosedGet(flap->digitalClosed);

    switch (flap->state)
    {
    case FLAP_CLOSED:
        if (flap->command < 0)
        {
            flap->command += flap->slew;
        }
        else
        {
            flap->command = 0;
        }
        if (!isClosed)
        {
            flap->state = FLAP_CLOSING;
            portalUpdate(flap->portal, "state");
            flap->command = -flap->highCommand;
            activate(flap);
        }
        break;
    case FLAP_OPENING:
        flap->command = flap->highCommand;
        if (isOpened)
        {
            flap->state = FLAP_OPENED;
            portalUpdate(flap->portal, "state");
            readify(flap);
            semaphoreGive(flap->semaphoreOpened);
        }
        break;
    case FLAP_OPENED:
        if (flap->command > 0)
        {
            flap->command -= flap->slew;
        }
        else
        {
            flap->command = 0;
        }
        if (!isOpened)
        {
            flap->state = FLAP_OPENING;
            portalUpdate(flap->portal, "state");
            flap->command = flap->highCommand;
            activate(flap);
        }
        break;
    case FLAP_CLOSING:
        flap->command = -flap->highCommand;
        if (isClosed)
        {
            flap->state = FLAP_CLOSED;
            portalUpdate(flap->portal, "state");
            readify(flap);
            semaphoreGive(flap->semaphoreClosed);
        }
        break;
    }
    flap->motorSet(flap->motor, (int)flap->command);
    portalFlush(flap->portal);
}

static void
readify(Flap * flap)
{
    flap->frameDelay = flap->frameDelayReady;
    flap->priority = flap->priorityReady;
    if (flap->task != NULL)
    {
        taskPrioritySet(flap->task, flap->priority);
    }
}

static void
activate(Flap * flap)
{
    flap->frameDelay = flap->frameDelayActive;
    flap->priority = flap->priorityActive;
    if (flap->task != NULL)
    {
        taskPrioritySet(flap->task, flap->priority);
    }
}

static void
initPortal(Flap * flap, FlapSetup setup)
{
    flap->portal = pigeonCreatePortal(setup.pigeon, setup.id);

    PortalEntrySetup setups[] =
    {
        {
            .key = "state",
            .handler = stateHandler,
            .handle = flap,
            .onchange = true
        },
        {
            .key = "open",
            .handler = openHandler,
            .handle = flap
        },
        {
            .key = "close",
            .handler = closeHandler,
            .handle = flap
        },
        {
            .key = "frame-delay",
            .handler = portalUlongHandler,
            .handle = &flap->frameDelay
        },
        {
            .key = "frame-delay-ready",
            .handler = portalUlongHandler,
            .handle = &flap->frameDelayReady
        },
        {
            .key = "frame-delay-active",
            .handler = portalUlongHandler,
            .handle = &flap->frameDelayActive
        },
        {
            .key = "drop-delay",
            .handler = portalUlongHandler,
            .handle = &flap->dropDelay
        },

        // End terminating struct
        {
            .key = "~",
            .handler = NULL,
            .handle = NULL
        }
    };
    portalAddBatch(flap->portal, setups);
    portalReady(flap->portal);
}

static void
stateHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (response == NULL) return;
    Flap * flap = handle;
    if (message == NULL)
    {
        switch (flap->state)
        {
        case FLAP_CLOSED:
            strcpy(response, "closed");
            break;
        case FLAP_OPENING:
            strcpy(response, "opening");
            break;
        case FLAP_OPENED:
            strcpy(response, "opened");
            break;
        case FLAP_CLOSING:
            strcpy(response, "closing");
            break;
        }
    }
    else if (strcmp(message, "closing") == 0) flapClose(flap);
    else if (strcmp(message, "opening") == 0) flapOpen(flap);
}

static void
openHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    Flap * flap = handle;
    flapOpen(flap);
}

static void
closeHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    Flap * flap = handle;
    flapClose(flap);
}

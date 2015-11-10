#include "flywheel.h"

#include <API.h>
#include <string.h>
#include "pigeon.h"
#include "control.h"
#include "utils.h"
#include "shims.h"


#define UNUSED(x) (void)(x)


// Typedefs {{{

struct Flywheel
{
    Portal * portal;

    ControlSystem system;
    ControlUpdater controlUpdate;
    ControlResetter controlReset;
    ControlHandle control;

    float measuredRaw;

    float gearing;
    float smoothing;
    EncoderGetter encoderGet;
    EncoderResetter encoderReset;
    EncoderHandle encoder;

    MotorSetter motorSet[8];
    MotorHandle motors[8];

    bool ready;
    unsigned int priorityReady;
    unsigned int priorityActive;
    unsigned long frameDelay;
    unsigned long frameDelayReady;
    unsigned long frameDelayActive;
    float thresholdError;
    float thresholdDerivative;
    int checkCycle;

    Semaphore readySemaphore;

    Mutex mutex;
    TaskHandle task;
};

/// }}}




// Private functions, forward declarations. {{{

static void task(void * flywheelPointer);
static void update(Flywheel*);
static void updateSystem(Flywheel*);
static void updateControl(Flywheel*);
static void updateMotor(Flywheel*);
static void checkReady(Flywheel*);
static void activate(Flywheel*);
static void readify(Flywheel*);
static void initPortal(Flywheel*, FlywheelSetup);
static void readyHandler(void * handle, char * message, char * response);

static void printDebugInfo(Flywheel*);

// }}}




// Public methods {{{

Flywheel *
flywheelInit(FlywheelSetup setup)
{
    Flywheel * flywheel = malloc(sizeof(Flywheel));

    initPortal(flywheel, setup);

    flywheel->system.microTime = micros();
    flywheel->system.dt = 0.0f;
    flywheel->system.target = 0.0f;
    flywheel->system.measured = 0.0f;
    flywheel->system.derivative = 0.0f;
    flywheel->system.error = 0.0f;
    flywheel->system.action = 0.0f;

    flywheel->controlUpdate = setup.controlUpdater;
    flywheel->controlReset = setup.controlResetter;
    flywheel->control = setup.control;
    //setup.controlSetup(setup.control, flywheel->portal);

    flywheel->gearing = setup.gearing;
    flywheel->smoothing = setup.smoothing;
    flywheel->encoderGet = setup.encoderGetter;
    flywheel->encoderReset = setup.encoderResetter;
    flywheel->encoder = setup.encoder;

    for (int i = 0; i < 8; i++)
    {
        flywheel->motorSet[i] = setup.motorSetters[i];
        flywheel->motors[i] = setup.motors[i];
    }

    flywheel->ready = true;
    flywheel->priorityReady = setup.priorityReady;
    flywheel->priorityActive = setup.priorityActive;

    flywheel->frameDelay = setup.frameDelayReady;
    flywheel->frameDelayReady = setup.frameDelayReady;
    flywheel->frameDelayActive = setup.frameDelayActive;

    flywheel->thresholdError = setup.thresholdError;
    flywheel->thresholdDerivative = setup.thresholdDerivative;

    flywheel->checkCycle = setup.checkCycle;

    flywheel->readySemaphore = semaphoreCreate();

    flywheel->mutex = mutexCreate();
    flywheel->task = NULL;

    portalReady(flywheel->portal);


    return flywheel;
}


void
flywheelReset(Flywheel * flywheel)
{
    flywheel->system.measured = 0.0f;
    flywheel->system.derivative = 0.0f;
    flywheel->system.error = 0.0f;
    flywheel->system.action = 0.0f;

    portalUpdate(flywheel->portal, "measured");
    portalUpdate(flywheel->portal, "derivative");
    portalUpdate(flywheel->portal, "error");
    portalUpdate(flywheel->portal, "action");

    flywheel->controlReset(flywheel->control);
    flywheel->encoderReset(flywheel->encoder);
}


void
flywheelSet(Flywheel * flywheel, float rpm)
{
    mutexTake(flywheel->mutex, -1);
    flywheel->system.target = rpm;
    mutexGive(flywheel->mutex);

    portalUpdate(flywheel->portal, "target");

    if (flywheel->ready)
    {
        activate(flywheel);
    }
}

void
flywheelRun(Flywheel * flywheel)
{
    //puts("Starting flywheel task");
    //printf("Flywheel->task = %d\n", (int)flywheel->task);
    //printf("NULL = %d\n", (int)NULL);
    if (flywheel->task == NULL)
    {
        //puts("Creating task...");
        flywheelReset(flywheel);
        flywheel->task = taskCreate(
            task,
            TASK_DEFAULT_STACK_SIZE,
            flywheel,
            flywheel->priorityActive
        );
    }
}

void
waitUntilFlywheelReady(Flywheel * flywheel, const unsigned long blockTime)
{
    semaphoreTake(flywheel->readySemaphore, blockTime);
}

// }}}




// Private functions {{{

static void
task(void * flywheelPointer)
{
    puts("Inside task beginning");
    Flywheel * flywheel = flywheelPointer;
    int i = 0;
    while (true)
    {
        i = flywheel->checkCycle;
        //printf("Flywheel task: counter i = %d\n", i);
        while (i)
        {
            //puts("\nBefore flywheel update");
            //encoderDebug(flywheel->encoder);
            update(flywheel);
            //puts("\nAfter flywheel update");
            //encoderDebug(flywheel->encoder);
            //printDebugInfo(flywheel);
            //puts("\nAfter update all");
            //printf("Address of encoder handle: %d\n", (int)flywheel->encoder);
            //printf("Address of control handle: %d\n", (int)flywheel->control);
            //delay(1000);
            delay(flywheel->frameDelay);
            --i;
        }
        checkReady(flywheel);
    }
}


// Temporary debugging measures:
// (Should be replaced with pigeon once pigeon is stabalized)
static void
printDebugInfo(Flywheel * flywheel)
{
    printf("flywheel rpm: %f\n", flywheel->system.measured);
    printf("flywheel action: %f\n", flywheel->system.action);
    printf("flywheel raw: %f\n", flywheel->measuredRaw);
}


static void
update(Flywheel * flywheel)
{
    mutexTake(flywheel->mutex, -1);
    updateSystem(flywheel);
    //puts("\nBefore control update");
    //encoderDebug(flywheel->encoder);
    updateControl(flywheel);
    //puts("\nAfter control update");
    //encoderDebug(flywheel->encoder);
    updateMotor(flywheel);
    //puts("\nBefore portal flush");
    //encoderDebug(flywheel->encoder);
    portalFlush(flywheel->portal);
    //puts("\nAfter portal flush");
    //encoderDebug(flywheel->encoder);
    mutexGive(flywheel->mutex);
}


static void
updateSystem(Flywheel * flywheel)
{
    float dt = timeUpdate(&flywheel->system.microTime);
    flywheel->system.dt = dt;

    // Raw rpm
    //puts("\nBefore Encoder Get");
    //encoderDebug(flywheel->encoder);
    //puts("\nIn updateSystem");
    //printf("Address of encoder handle: %d\n", (int)flywheel->encoder);
    //printf("Address of control handle: %d\n", (int)flywheel->control);
    float rpm = flywheel->encoderGet(flywheel->encoder).rpm;
    //puts("\nAfter Encoder Get");
    //encoderDebug(flywheel->encoder);
    rpm *= flywheel->gearing;
    //puts("\nAfter flywheel gearing");
    //encoderDebug(flywheel->encoder);

    // Low-pass filter
    float measureChange = (rpm - flywheel->system.measured);
    measureChange *= dt / flywheel->smoothing;

    // Update
    flywheel->measuredRaw = rpm;
    flywheel->system.measured += measureChange;
    flywheel->system.derivative = measureChange / dt;

    // Calculate error
    float error = flywheel->system.measured - flywheel->system.target;
    flywheel->system.error = error;

    //puts("\nBefore portal update system");
    //encoderDebug(flywheel->encoder);

    portalUpdate(flywheel->portal, "dt");
    portalUpdate(flywheel->portal, "raw");
    portalUpdate(flywheel->portal, "measured");
    portalUpdate(flywheel->portal, "derivative");
    portalUpdate(flywheel->portal, "error");

    //puts("\nAfter portal update system");
    //encoderDebug(flywheel->encoder);
}


static void
updateControl(Flywheel * flywheel)
{
    //puts("\n In updateControl");
    //printf("Address of encoder handle: %d\n", (int)flywheel->encoder);
    //printf("Address of control handle: %d\n", (int)flywheel->control);
    flywheel->controlUpdate(flywheel->control, &flywheel->system, flywheel->encoder);
    //puts("\nAfter control update handled, before clipping");
    //encoderDebug(flywheel->encoder);

    if (flywheel->system.action > 127)
    {
        flywheel->system.action = 127;
    }
    if (flywheel->system.action < -127)
    {
        flywheel->system.action = -127;
    }
    //puts("\nAfter clipping, before portal action update");
    //encoderDebug(flywheel->encoder);
    portalUpdate(flywheel->portal, "action");
    //puts("\nAfter portal action update");
    //encoderDebug(flywheel->encoder);
}


static void
updateMotor(Flywheel * flywheel)
{
    for (int i = 0; flywheel->motorSet[i] && i < 8; i++)
    {
        void * handle = flywheel->motors[i];
        int command = flywheel->system.action;
        flywheel->motorSet[i](handle, command);
    }
}


static void
checkReady(Flywheel * flywheel)
{
    bool errorReady =
        isWithin(flywheel->system.error, flywheel->thresholdError);
    bool derivativeReady =
        isWithin(flywheel->system.derivative, flywheel->thresholdDerivative);
    bool ready = errorReady && derivativeReady;

    if (ready && !flywheel->ready)
    {
        readify(flywheel);
    }
    else if (!ready && flywheel->ready)
    {
        activate(flywheel);
    }
}


static void
activate(Flywheel * flywheel)
{
    flywheel->ready = false;
    flywheel->frameDelay = flywheel->frameDelayActive;
    if (flywheel->task)
    {
        taskPrioritySet(flywheel->task, flywheel->priorityActive);
    }
    portalUpdate(flywheel->portal, "ready");
    portalUpdate(flywheel->portal, "delay");

    // TODO: Signal not ready?
}


static void
readify(Flywheel * flywheel)
{
    flywheel->ready = true;
    flywheel->frameDelay = flywheel->frameDelayReady;
    if (flywheel->task)
    {
        taskPrioritySet(flywheel->task, flywheel->priorityReady);
    }
    portalUpdate(flywheel->portal, "ready");
    portalUpdate(flywheel->portal, "delay");

    semaphoreGive(flywheel->readySemaphore);
}


// }}}



// Pigeon setup {{{

static void
initPortal(Flywheel * flywheel, FlywheelSetup setup)
{
    flywheel->portal = pigeonCreatePortal(setup.pigeon, setup.id);

    PortalEntrySetup setups[] =
    {
        {
            .key = "time",
            .handler = portalUlongHandler,
            .handle = &flywheel->system.microTime
        },
        {
            .key = "dt",
            .handler = portalFloatHandler,
            .handle = &flywheel->system.dt
        },
        {
            .key = "target",
            .handler = portalFloatHandler,
            .handle = &flywheel->system.target,
            .stream = true,
            .onchange = true
        },
        {
            .key = "measured",
            .handler = portalFloatHandler,
            .handle = &flywheel->system.measured,
            .stream = true
        },
        {
            .key = "derivatve",
            .handler = portalFloatHandler,
            .handle = &flywheel->system.derivative
        },
        {
            .key = "error",
            .handler = portalFloatHandler,
            .handle = &flywheel->system.error
        },
        {
            .key = "action",
            .handler = portalFloatHandler,
            .handle = &flywheel->system.action,
            .stream = true
        },
        {
            .key = "raw",
            .handler = portalFloatHandler,
            .handle = &flywheel->measuredRaw
        },
        {
            .key = "gearing",
            .handler = portalFloatHandler,
            .handle = &flywheel->gearing
        },
        {
            .key = "smoothing",
            .handler = portalFloatHandler,
            .handle = &flywheel->smoothing
        },
        {
            .key = "ready",
            .handler = readyHandler,
            .handle = flywheel,
            .onchange = true
        },
        {
            .key = "priority-ready",
            .handler = portalUintHandler,
            .handle = &flywheel->priorityReady
        },
        {
            .key = "priority-active",
            .handler = portalUintHandler,
            .handle = &flywheel->priorityActive
        },
        {
            .key = "delay",
            .handler = portalUlongHandler,
            .handle = &flywheel->frameDelay,
            .onchange = true
        },
        {
            .key = "delay-ready",
            .handler = portalUlongHandler,
            .handle = &flywheel->frameDelayReady
        },
        {
            .key = "delay-active",
            .handler = portalUlongHandler,
            .handle = &flywheel->frameDelayActive
        },
        {
            .key = "threshold-error",
            .handler = portalUlongHandler,
            .handle = &flywheel->thresholdError
        },
        {
            .key = "threshold-derivative",
            .handler = portalUlongHandler,
            .handle = &flywheel->thresholdDerivative
        },
        {
            .key = "check-cycle",
            .handler = portalUlongHandler,
            .handle = &flywheel->checkCycle
        },
        {
            .key = "keys",
            .handler = portalStreamKeyHandler,
            .handle = &flywheel->portal
        },

        // End terminating struct
        {
            .key = "~",
            .handler = NULL,
            .handle = NULL
        }
    };
    portalAddBatch(flywheel->portal, setups);
}

void readyHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (message == NULL) return;
    if (response == NULL) return;
    Flywheel * flywheel = handle;
    if (message[0] == '\0')
    {
        strcpy(response, flywheel->ready? "true" : "false");
    }
    else if (strcmp(message, "true") == 0) readify(flywheel);
    else if (strcmp(message, "false") == 0) activate(flywheel);
}

// }}}

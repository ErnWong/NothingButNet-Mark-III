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
    char id[PIGEON_KEYSIZE];

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
    float readyErrorInterval;
    float readyDerivativeInterval;
    int readyCheckCycle;

    Semaphore readySemaphore;

    Mutex targetMutex;
    TaskHandle task;
};

/// }}}




// Private functions, forward declarations. {{{

static void task(void *flywheelPointer);
static void update(Flywheel *flywheel);
static void updateSystem(Flywheel *flywheel);
static void updateControl(Flywheel *flywheel);
static void updateMotor(Flywheel *flywheel);
static void checkReady(Flywheel *flywheel);
static void activate(Flywheel *flywheel);
static void readify(Flywheel *flywheel);

// }}}




// Public methods {{{

Flywheel *
flywheelInit(FlywheelSetup setup)
{
    Flywheel *flywheel = malloc(sizeof(Flywheel));

    strncpy(flywheel->id, setup.id, 8);

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

    flywheel->readyErrorInterval = setup.readyErrorInterval;
    flywheel->readyDerivativeInterval = setup.readyDerivativeInterval;

    flywheel->readyCheckCycle = setup.readyCheckCycle;

    flywheel->readySemaphore = semaphoreCreate();

    flywheel->targetMutex = mutexCreate();
    flywheel->task = NULL;

    return flywheel;
}


void
flywheelReset(Flywheel *flywheel)
{
    flywheel->system.measured = 0.0f;
    flywheel->system.derivative = 0.0f;
    flywheel->system.error = 0.0f;
    flywheel->system.action = 0.0f;
    flywheel->controlReset(flywheel->control);
    flywheel->encoderReset(flywheel->encoder);
}


void
flywheelSet(Flywheel *flywheel, float rpm)
{
    mutexTake(flywheel->targetMutex, 100); // TODO: figure out how long the block time should be.
    flywheel->system.target = rpm;
    mutexGive(flywheel->targetMutex);

    if (flywheel->ready)
    {
        activate(flywheel);
    }
}

void
flywheelRun(Flywheel *flywheel)
{
    if (!flywheel->task)
    {
        flywheelReset(flywheel);
        flywheel->task = taskCreate(task, TASK_DEFAULT_STACK_SIZE, flywheel, flywheel->priorityActive);
    }
}

void
waitUntilFlywheelReady(Flywheel *flywheel, const unsigned long blockTime)
{
    semaphoreTake(flywheel->readySemaphore, blockTime);
}

// }}}




// Private functions {{{

static void
task(void *flywheelPointer)
{
    Flywheel *flywheel = flywheelPointer;
    int i = 0;
    while (1)
    {
        i = flywheel->readyCheckCycle;
        while (i)
        {
            update(flywheel);
            delay(flywheel->frameDelay);
            --i;
        }
        checkReady(flywheel);
    }
}


static void
update(Flywheel *flywheel)
{
    updateSystem(flywheel);
    updateControl(flywheel);
    updateMotor(flywheel);
}


static void
updateSystem(Flywheel *flywheel)
{
    float dt = timeUpdate(&flywheel->system.microTime);
    flywheel->system.dt = dt;

    // Raw rpm
    float rpm = flywheel->encoderGet(flywheel->encoder);
    rpm *= flywheel->gearing;

    // Low-pass filter
    float measureChange = (rpm - flywheel->system.measured);
    measureChange *= dt / flywheel->smoothing;

    // Update
    flywheel->measuredRaw = rpm;
    flywheel->system.measured += measureChange;
    flywheel->system.derivative = measureChange / dt;

    // Calculate error
    float error = flywheel->system.measured - flywheel->system.target;
    mutexTake(flywheel->targetMutex, -1);
    // TODO: Find out what block time is suitable, or needeed at all.
    flywheel->system.error = error;
    mutexGive(flywheel->targetMutex);
}


static void
updateControl(Flywheel *flywheel)
{
    flywheel->controlUpdate(flywheel->control, flywheel->system);

    if (flywheel->system.action > 127)
    {
        flywheel->system.action = 127;
    }
    if (flywheel->system.action < -127)
    {
        flywheel->system.action = -127;
    }
}


static void
updateMotor(Flywheel *flywheel)
{
    for (int i = 0; flywheel->motorSet[i] && i < 8; i++)
    {
        void * handle = flywheel->motors[i];
        int command = flywheel->system.action;
        flywheel->motorSet[i](handle, command);
    }
}


static void
checkReady(Flywheel *flywheel)
{
    bool errorReady =
        isWithin(flywheel->system.error, flywheel->readyErrorInterval);
    bool derivativeReady =
        isWithin(flywheel->system.derivative, flywheel->readyDerivativeInterval);
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
activate(Flywheel *flywheel)
{
    flywheel->ready = false;
    flywheel->frameDelay = flywheel->frameDelayActive;
    if (flywheel->task)
    {
        taskPrioritySet(flywheel->task, flywheel->priorityActive);
    }
    // TODO: Signal not ready?
}


static void
readify(Flywheel *flywheel)
{
    flywheel->ready = true;
    flywheel->frameDelay = flywheel->frameDelayReady;
    if (flywheel->task)
    {
        taskPrioritySet(flywheel->task, flywheel->priorityReady);
    }
    semaphoreGive(flywheel->readySemaphore);
}


// }}}

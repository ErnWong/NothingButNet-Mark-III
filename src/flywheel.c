#include "flywheel.h"

#include <API.h>
#include <string.h>
#include "utils.h"
#include "shims.h"


// Definitions {{{

#define UNUSED(x) (void)(x)

// }}}




// Typedefs {{{

struct Flywheel
{
    char id[8];

    float target;                       // Target speed in rpm.
    float estimate;
    float measured;                     // Measured speed in rpm.
    float measuredRaw;
    float derivative;                   // Rate at which the measured speed had changed.
    float integral;
    float error;                        // Difference in the target and the measured speed in rpm.
    float action;                       // Controller output sent to the (smart) motors.

    float lastAction;
    float lastError;
    bool firstCross;

    unsigned long microTime;            // The time in microseconds of the last update.
    float timeChange;                   // The time difference between updates, in seconds.

    // Controller parameters
    float pidKp;
    float pidKi;
    float pidKd;
    float tbhGain;
    float bbValue;
    float bbAbove;
    float bbBelow;
    float gearing;
    float smoothing;

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

    FlywheelController controllerType;

    Mutex targetMutex;                  // Mutex for updating the target speed.
    TaskHandle task;                    // Handle to the controlling task.
    EncoderGetter encoderGet;
    EncoderResetter encoderReset;
    void * encoderArgs;
    MotorSetter motorSet[8];
    void * motorArgs[8];
};

/// }}}




// Private functions, forward declarations. {{{

static void task(void *flywheelPointer);
static void update(Flywheel *flywheel);
static void measureRpm(Flywheel *flywheel, float timeChange);
static void controllerUpdate(Flywheel *flywheel, float timeChange);
static void pidUpdate(Flywheel *flywheel, float timeChange);
static void tbhUpdate(Flywheel *flywheel, float timeChange);
static void bangBangUpdate(Flywheel *flywheel, float timeChange);
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

    flywheel->target = 0.0f;
    flywheel->estimate = 0.0f;
    flywheel->measured = 0.0f;
    flywheel->derivative = 0.0f;
    flywheel->integral = 0.0f;
    flywheel->error = 0.0f;
    flywheel->action = 0.0f;

    flywheel->lastAction = 0.0f;
    flywheel->lastError = 0.0f;
    flywheel->firstCross = true;

    flywheel->microTime = micros();
    flywheel->timeChange = 0.0f;

    flywheel->pidKp = setup.pidKp;
    flywheel->pidKi = setup.pidKi;
    flywheel->pidKd = setup.pidKd;
    flywheel->tbhGain = setup.tbhGain;
    flywheel->bbValue = setup.bbValue;
    flywheel->bbAbove = setup.bbAbove;
    flywheel->bbBelow = setup.bbBelow;
    flywheel->gearing = setup.gearing;
    flywheel->smoothing = setup.smoothing;

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

    flywheel->controllerType = setup.controllerType;

    flywheel->targetMutex = mutexCreate();
    flywheel->task = NULL;
    flywheel->encoderGet = setup.encoderGetter;
    flywheel->encoderReset = setup.encoderResetter;
    flywheel->encoderArgs = setup.encoderArgs;
    for (int i = 0; i < 8; i++)
    {
        flywheel->motorSet[i] = setup.motorSetters[i];
        flywheel->motorArgs[i] = setup.motorArgs[i];
    }

    return flywheel;
}


void
flywheelReset(Flywheel *flywheel)
{
    flywheel->derivative = 0.0f;
    flywheel->integral = 0.0f;
    flywheel->error = 0.0f;
    flywheel->action = 0.0f;
    flywheel->lastAction = 0.0f;
    flywheel->lastError = 0.0f;
    flywheel->firstCross = true;
    flywheel->encoderReset(flywheel->encoderArgs);
}

// Sets target RPM.
void
flywheelSet(Flywheel *flywheel, float rpm, float estimate)
{
    mutexTake(flywheel->targetMutex, 100); // TODO: figure out how long the block time should be.
    flywheel->target = rpm;
    mutexGive(flywheel->targetMutex);

    flywheel->estimate = estimate;
    flywheel->firstCross = true;

    if (flywheel->ready)
    {
        activate(flywheel);
    }
}

void
flywheelSetController(Flywheel *flywheel, FlywheelController type)
{
    flywheelReset(flywheel);
    flywheel->controllerType = type;
}

void
flywheelSetSmoothing(Flywheel *flywheel, float smoothing)
{
    flywheel->smoothing = smoothing;
}
void
flywheelSetPidKp(Flywheel *flywheel, float gain)
{
    flywheel->pidKp = gain;
}
void
flywheelSetPidKi(Flywheel *flywheel, float gain)
{
    flywheel->pidKi = gain;
}
void
flywheelSetPidKd(Flywheel *flywheel, float gain)
{
    flywheel->pidKd = gain;
}
void
flywheelSetTbhGain(Flywheel *flywheel, float gain)
{
    flywheel->tbhGain = gain;
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
    float timeChange = timeUpdate(&flywheel->microTime);
    measureRpm(flywheel, timeChange);
    controllerUpdate(flywheel, timeChange);
    updateMotor(flywheel);
}


static void
measureRpm(Flywheel *flywheel, float timeChange)
{
    // Raw rpm
    float rpm = flywheel->encoderGet(flywheel->encoderArgs);
    rpm *= flywheel->gearing;

    // Low-pass filter
    float measureChange = (rpm - flywheel->measured);
    measureChange *= timeChange / flywheel->smoothing;

    // Update
    flywheel->measuredRaw = rpm;
    flywheel->measured += measureChange;
    flywheel->derivative = measureChange / timeChange;

    // Calculate error
    mutexTake(flywheel->targetMutex, -1);   // TODO: Find out what block time is suitable, or needeed at all.
    flywheel->error = flywheel->measured - flywheel->target;
    mutexGive(flywheel->targetMutex);
}


static void
controllerUpdate(Flywheel *flywheel, float timeChange)
{
    switch (flywheel->controllerType)
    {
    case FLYWHEEL_PID:
        pidUpdate(flywheel, timeChange);
        break;
    case FLYWHEEL_TBH:
        tbhUpdate(flywheel, timeChange);
        break;
    case FLYWHEEL_BANGBANG:
        bangBangUpdate(flywheel, timeChange);
        break;
    }
    if (flywheel->action > 127)
    {
        flywheel->action = 127;
    }
    if (flywheel->action < -127)
    {
        flywheel->action = -127;
    }
}


static void
pidUpdate(Flywheel *flywheel, float timeChange)
{
    flywheel->integral += flywheel->error * timeChange;

    float proportionalPart = flywheel->pidKp * flywheel->error;
    float integralPart = flywheel->pidKi * flywheel->integral;
    float derivativePart = flywheel->pidKd * flywheel->derivative;

    flywheel->action = proportionalPart + integralPart + derivativePart;
}


static void
tbhUpdate(Flywheel *flywheel, float timeChange)
{
    flywheel->action += flywheel->error * timeChange * flywheel->tbhGain;
    if (signOf(flywheel->error) != signOf(flywheel->lastError))
    {
        if (flywheel->firstCross)
        {
            flywheel->action = flywheel->estimate;
            flywheel->firstCross = false;
        }
        else
        {
            flywheel->action = 0.5f * (flywheel->action + flywheel->lastAction);
        }
        flywheel->lastAction = flywheel->action;
    }
    flywheel->lastError = flywheel->error;
}


static void
bangBangUpdate(Flywheel *flywheel, float timeChange)
{
    UNUSED(timeChange);

    if (flywheel->measured > flywheel->target + flywheel->bangBangAbove)
    {
        flywheel->action = 0;
    }
    else if (flywheel->measured < flywheel->target + flywheel->bangBangBelow)
    {
        flywheel->action = flywheel->bangBangValue;
    }
}


static void
updateMotor(Flywheel *flywheel)
{
    for (int i = 0; flywheel->motorSet[i] && i < 8; i++)
    {
        void * args = flywheel->motorArgs[i];
        int command = flywheel->action;
        flywheel->motorSet[i](args, command);
    }
}


static void
checkReady(Flywheel *flywheel)
{
    bool errorReady = -flywheel->readyErrorInterval < flywheel->error && flywheel->error < flywheel->readyErrorInterval;
    bool derivativeReady = -flywheel->readyDerivativeInterval < flywheel->derivative && flywheel->derivative < flywheel->readyDerivativeInterval;
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


// Faster updates, higher priority, signals active.
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


// Slower updates, lower priority, signals ready.
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

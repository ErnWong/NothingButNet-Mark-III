#include "flywheel.h"

#include <API.h>
#include "utils.h"




// TODO: tune success intervals, priorities, and checking period.

#define FLYWHEEL_READY_ERROR_INTERVAL 1.0f      // The +/- interval for which the error needs to lie to be considered 'ready'.
#define FLYWHEEL_READY_DERIVATIVE_INTERVAL 1.0f // The +/- interval for which the measured derivative needs to lie to be considered 'ready'.

#define FLYWHEEL_ACTIVE_PRIORITY 3              // Priority of the update task during active mode
#define FLYWHEEL_READY_PRIORITY 2               // Priority of the update task during ready mode

#define FLYWHEEL_ACTIVE_DELAY 20                // Delay for each update during active mode
#define FLYWHEEL_READY_DELAY 200                // Delay for each update during ready mode

#define FLYWHEEL_CHECK_READY_PERIOD 20          // Number of updates before rechecking its ready state








// Private functions, forward declarations.

void task(void *flywheelPointer);
void update(Flywheel *flywheel);
void measureRpm(Flywheel *flywheel, float timeChange);
void controllerUpdate(Flywheel *flywheel, float timeChange);
void pidUpdate(Flywheel *flywheel, float timeChange);
void tbhUpdate(Flywheel *flywheel, float timeChange);
void bangBangUpdate(Flywheel *flywheel, float timeChange);
void updateMotor(Flywheel *flywheel);
void checkReady(Flywheel *flywheel);
void activate(Flywheel *flywheel);
void readify(Flywheel *flywheel);



Flywheel *flywheelInit(FlywheelSetup setup)
{
	Flywheel *flywheel = malloc(sizeof(Flywheel));

	flywheel->target = 0.0f;
	flywheel->measured = 0.0f;
	flywheel->measured = 0.0f;
	flywheel->derivative = 0.0f;
	flywheel->integral = 0.0f;
	flywheel->error = 0.0f;
	flywheel->action = 0.0f;

	flywheel->lastAction = 0.0f;
	flywheel->lastError = 0.0f;
	flywheel->firstCross = true;

	flywheel->reading = 0;
	flywheel->microTime = micros();
	flywheel->timeChange = 0.0f;

	flywheel->pidKp = setup.pidKp;
	flywheel->pidKi = setup.pidKi;
	flywheel->pidKd = setup.pidKd;
	flywheel->tbhGain = setup.tbhGain;
	flywheel->tbhApprox = setup.tbhApprox;
	flywheel->bangBangValue = setup.bangBangValue;
	flywheel->gearing = setup.gearing;
	flywheel->encoderTicksPerRevolution = setup.encoderTicksPerRevolution;
	flywheel->smoothing = setup.smoothing;

	flywheel->ready = true;
	flywheel->delay = FLYWHEEL_READY_DELAY;
	flywheel->allowReadify = true;

	flywheel->readySemaphore = semaphoreCreate();
	flywheel->stepSemaphore = semaphoreCreate();

	flywheel->controllerType = CONTROLLER_TYPE_PID;

	flywheel->targetMutex = mutexCreate();
	flywheel->task = NULL;
	//flywheel->task = taskCreate(task, 1000000, flywheel, FLYWHEEL_READY_PRIORITY);	// TODO: What stack size should be set?
	flywheel->encoder = encoderInit(setup.encoderPortTop, setup.encoderPortBottom, setup.encoderReverse);
	flywheel->motorChannels[0] = setup.motorChannels[0];
	flywheel->motorChannels[1] = setup.motorChannels[1];
	flywheel->motorChannels[2] = setup.motorChannels[2];
	flywheel->motorChannels[3] = setup.motorChannels[3];
	flywheel->motorReversed[0] = setup.motorReversed[0];
	flywheel->motorReversed[1] = setup.motorReversed[1];
	flywheel->motorReversed[2] = setup.motorReversed[2];
	flywheel->motorReversed[3] = setup.motorReversed[3];

	return flywheel;
}


void flywheelReset(Flywheel *flywheel)
{
	flywheel->derivative = 0.0f;
	flywheel->integral = 0.0f;
	flywheel->error = 0.0f;
	flywheel->action = 0.0f;
	flywheel->lastAction = 0.0f;
	flywheel->lastError = 0.0f;
	flywheel->firstCross = true;
	flywheel->reading = 0;
	encoderReset(flywheel->encoder);
}

// Sets target RPM.
void flywheelSet(Flywheel *flywheel, float rpm)
{
	mutexTake(flywheel->targetMutex, -1); // TODO: figure out how long the block time should be.
	flywheel->target = rpm;
	mutexGive(flywheel->targetMutex);

	flywheel->firstCross = true;

	if (flywheel->ready)
	{
		activate(flywheel);
	}

	//printf("Debug Target set to %f rpm.\n", rpm);
}

void flywheelSetController(Flywheel *flywheel, ControllerType type)
{
	flywheelReset(flywheel);
	flywheel->controllerType = type;
}

void flywheelSetSmoothing(Flywheel *flywheel, float smoothing)
{
	flywheel->smoothing = smoothing;
}
void flywheelSetPidKp(Flywheel *flywheel, float gain)
{
	flywheel->pidKp = gain;
	//printf("Debug pidKp set to %f.\n", gain);
}
void flywheelSetPidKi(Flywheel *flywheel, float gain)
{
	flywheel->pidKi = gain;
}
void flywheelSetPidKd(Flywheel *flywheel, float gain)
{
	flywheel->pidKd = gain;
}
void flywheelSetTbhGain(Flywheel *flywheel, float gain)
{
	flywheel->tbhGain = gain;
}
void flywheelSetTbhApprox(Flywheel *flywheel, float approx)
{
	flywheel->tbhApprox = approx;
}
void flywheelSetAllowReadify(Flywheel *flywheel, bool isAllowed)
{
	flywheel->allowReadify = isAllowed;
}

void flywheelRun(Flywheel *flywheel)
{
	if (!flywheel->task)
	{
		flywheelReset(flywheel);
		flywheel->task = taskCreate(task, TASK_DEFAULT_STACK_SIZE, flywheel, FLYWHEEL_ACTIVE_PRIORITY);
	}
}

void waitUntilFlywheelReady(Flywheel *flywheel, const unsigned long blockTime)
{
	semaphoreTake(flywheel->readySemaphore, blockTime);
}

void waitUntilFlywheelStep(Flywheel *flywheel, const unsigned long blockTime)
{
	semaphoreTake(flywheel->stepSemaphore, blockTime);
}



void task(void *flywheelPointer)
{
	Flywheel *flywheel = flywheelPointer;
	int i = 0;
	while (1)
	{
		i = FLYWHEEL_CHECK_READY_PERIOD;
		while (i)
		{
			update(flywheel);
			delay(flywheel->delay);
			--i;
		}
		checkReady(flywheel);
	}
}


void update(Flywheel *flywheel)
{
	float timeChange = timeUpdate(&flywheel->microTime);
	measureRpm(flywheel, timeChange);
	controllerUpdate(flywheel, timeChange);
	updateMotor(flywheel);
	// TODO: update smart motor group.
}


void measureRpm(Flywheel *flywheel, float timeChange)
{
	int reading = encoderGet(flywheel->encoder);
	int ticks = reading - flywheel->reading;

	// Raw rpm
	float rpm = ticks / flywheel->encoderTicksPerRevolution * flywheel->gearing / timeChange * 60;

	// Low-pass filter
	float measureChange = (rpm - flywheel->measured) * timeChange / flywheel->smoothing;

	// Update
	flywheel->reading = reading;
	flywheel->measuredRaw = rpm;
	flywheel->measured += measureChange;
	flywheel->derivative = measureChange / timeChange;

	// Calculate error
	mutexTake(flywheel->targetMutex, -1);	// TODO: Find out what block time is suitable, or needeed at all.
	flywheel->error = flywheel->measured - flywheel->target;
	mutexGive(flywheel->targetMutex);
}


void controllerUpdate(Flywheel *flywheel, float timeChange)
{
	switch (flywheel->controllerType)
	{
	case CONTROLLER_TYPE_PID:
		pidUpdate(flywheel, timeChange);
		break;
	case CONTROLLER_TYPE_TBH:
		tbhUpdate(flywheel, timeChange);
		break;
	case CONTROLLER_TYPE_BANG_BANG:
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

void pidUpdate(Flywheel *flywheel, float timeChange)
{
	flywheel->integral += flywheel->error * timeChange;

	float proportionalPart = flywheel->pidKp * flywheel->error;
	float integralPart = flywheel->pidKi * flywheel->integral;
	float derivativePart = flywheel->pidKd * flywheel->derivative;

	flywheel->action = proportionalPart + integralPart + derivativePart;
}

void tbhUpdate(Flywheel *flywheel, float timeChange)
{
	flywheel->action += flywheel->error * timeChange * flywheel->tbhGain;
	if (signOf(flywheel->error) != signOf(flywheel->lastError))
	{
		if (flywheel->firstCross)
		{
			flywheel->action = flywheel->tbhApprox;
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

void bangBangUpdate(Flywheel *flywheel, float timeChange)
{
	if (flywheel->measured > flywheel->target)
	{
		flywheel->action = 0;
	}
	else if (flywheel->measured < flywheel->target)
	{
		flywheel->action = flywheel->bangBangValue;
	}
}


void updateMotor(Flywheel *flywheel)
{
	for (int i = 0; flywheel->motorChannels[i]; i++)
	{
		int action = flywheel->motorReversed[i]? -flywheel->action : flywheel->action;
		motorSet(flywheel->motorChannels[i], action);
	}
}


void checkReady(Flywheel *flywheel)
{
	bool errorReady = -FLYWHEEL_READY_ERROR_INTERVAL < flywheel->error && flywheel->error < FLYWHEEL_READY_ERROR_INTERVAL;
	bool derivativeReady = -FLYWHEEL_READY_DERIVATIVE_INTERVAL < flywheel->derivative && flywheel->derivative < FLYWHEEL_READY_DERIVATIVE_INTERVAL;
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
void activate(Flywheel *flywheel)
{
	flywheel->ready = false;
	flywheel->delay = FLYWHEEL_ACTIVE_DELAY;
	if (flywheel->task)
	{
		taskPrioritySet(flywheel->task, FLYWHEEL_ACTIVE_PRIORITY);
	}
	// TODO: Signal not ready?
}


// Slower updates, lower priority, signals ready.
void readify(Flywheel *flywheel)
{
	flywheel->ready = true;
	flywheel->delay = FLYWHEEL_READY_DELAY;
	if (flywheel->task)
	{
		taskPrioritySet(flywheel->task, FLYWHEEL_READY_PRIORITY);
	}
	semaphoreGive(flywheel->readySemaphore);
}
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



typedef struct FlywheelData		// TODO: look at packing and alignment
{

	float target;                       // Target speed in rpm.
	float measured;                     // Measured speed in rpm.
	float derivative;                   // Rate at which the measured speed had changed.
	float error;                        // Difference in the target and the measured speed in rpm.
	float action;                       // Controller output sent to the (smart) motors.

	int reading;                        // Previous encoder value.
	unsigned long microTime;            // The time in microseconds of the last update.
	float timeChange;                   // The time difference between updates, in seconds.

	float gain;                         // Gain proportional constant for integrating controller.
	float gearing;                      // Ratio of flywheel RPM per encoder RPM.
	float encoderTicksPerRevolution;    // Number of ticks each time the encoder completes one revolution
	float smoothing;                    // Amount of smoothing applied to the flywheel RPM, which is the low-pass filter time constant in seconds.

	bool ready;                         // Whether the controller is in ready mode (true, flywheel at the right speed) or active mode (false), which affects task priority and update rate.
	unsigned long delay;

	Mutex targetMutex;                  // Mutex for updating the target speed.
	TaskHandle task;                    // Handle to the controlling task.
	Encoder encoder;                    // Encoder used to measure the rpm.
}
FlywheelData;




// Private functions, forward declarations.

void update(FlywheelData *data);
void measureRpm(FlywheelData *data, float timeChange);
void controllerUpdate(FlywheelData *data, float timeChange);
void checkReady(FlywheelData *data);
void activate(FlywheelData *data);
void readify(FlywheelData *data);



Flywheel flywheelInit(FlywheelSetup setup)
{
	FlywheelData *data = malloc(sizeof(FlywheelData));

	data->target = 0.0f;
	data->measured = 0.0f;
	data->derivative = 0.0f;
	data->error = 0.0f;
	data->action = 0.0f;

	data->reading = 0;
	data->microTime = micros();
	data->timeChange = 0.0f;

	data->gain = setup.gain;
	data->gearing = setup.gearing;
	data->encoderTicksPerRevolution = setup.encoderTicksPerRevolution;
	data->smoothing = setup.smoothing;

	data->ready = true;
	data->delay = FLYWHEEL_READY_DELAY;

	data->targetMutex = mutexCreate();
	data->task = taskCreate(task, 1000000, data, FLYWHEEL_READY_PRIORITY);	// TODO: What stack size should be set?
	data->encoder = encoderInit(setup.encoderPortTop, setup.encoderPortBottom, setup.encoderReverse);

	return (Flywheel)data;
}


// Sets target RPM.
void flywheelSet(Flywheel flywheel, float rpm)
{
	FlywheelData *data = flywheel;

	mutexTake(data->targetMutex, -1); // TODO: figure out how long the block time should be.
	data->target = rpm;
	mutexGive(data->targetMutex);
	
	if (data->ready)
	{
		activate(data);
	}
}


void task(void *flywheel)
{
	FlywheelData *data = flywheel;
	int i = 0;
	while (1)
	{
		i = FLYWHEEL_CHECK_READY_PERIOD;
		while (i)
		{
			update(data);
			delay(data->delay);
			--i;
		}
		checkReady(data);
	}
}


void update(FlywheelData *data)
{
	float timeChange = timeUpdate(&data->microTime);
	measureRpm(data, timeChange);
	controllerUpdate(data, timeChange);
	// TODO: update smart motor group.
}


void measureRpm(FlywheelData *data, float timeChange)
{
	int reading = encoderGet(data->encoder);
	int ticks = reading - data->reading;

	// Raw rpm
	float rpm = ticks / data->encoderTicksPerRevolution * data->gearing / timeChange;
	
	// Low-pass filter
	float measureChange = (rpm - data->measured) * timeChange / data->smoothing;

	// Update
	data->reading = reading;
	data->measured += measureChange;
	data->derivative = measureChange / timeChange;
}


// Proportionally integral controller.
void controllerUpdate(FlywheelData *data, float timeChange)
{
	// Calculate error
	mutexTake(data->targetMutex, -1);	// TODO: Find out what block time is suitable, or needeed at all.
	data->error = data->measured - data->target;
	mutexGive(data->targetMutex);

	// Integrate
	data->action += timeChange * data->gain * data->error;	// TODO: try take-back-half controller.
}


void checkReady(FlywheelData *data)
{
	bool errorReady = -FLYWHEEL_READY_ERROR_INTERVAL < data->error < FLYWHEEL_READY_ERROR_INTERVAL;
	bool derivativeReady = -FLYWHEEL_READY_DERIVATIVE_INTERVAL < data->derivative < FLYWHEEL_READY_DERIVATIVE_INTERVAL;
	bool ready = errorReady && derivativeReady;

	if (ready && !data->ready)
	{
		readify(data);
	}
	else if (!ready && data->ready)
	{
		activate(data);
	}
}


// Faster updates, higher priority, signals active.
void activate(FlywheelData *data)
{
	data->ready = false;
	data->delay = FLYWHEEL_ACTIVE_DELAY;
	taskPrioritySet(data->task, FLYWHEEL_ACTIVE_PRIORITY);
	// TODO: Signal not ready?
}


// Slower updates, lower priority, signals ready.
void readify(FlywheelData *data)
{
	data->ready = true;
	data->delay = FLYWHEEL_READY_DELAY;
	taskPrioritySet(data->task, FLYWHEEL_READY_PRIORITY);
	// TODO: Signal ready?
}
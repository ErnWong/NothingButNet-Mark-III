#ifndef FLYWHEEL_H_
#define FLYWHEEL_H_

#include <API.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef enum ControllerType
{
	CONTROLLER_TYPE_PID,
	CONTROLLER_TYPE_TBH,
	CONTROLLER_TYPE_BANG_BANG
}
ControllerType;

typedef struct Flywheel		// TODO: look at packing and alignment
{

	float target;                       // Target speed in rpm.
	float measured;                     // Measured speed in rpm.
	float measuredRaw;
	float derivative;                   // Rate at which the measured speed had changed.
	float integral;
	float error;                        // Difference in the target and the measured speed in rpm.
	float action;                       // Controller output sent to the (smart) motors.

	float lastAction;
	float lastError;
	bool firstCross;

	int reading;                        // Previous encoder value.
	unsigned long microTime;            // The time in microseconds of the last update.
	float timeChange;                   // The time difference between updates, in seconds.

	float pidKp;                         // Gain proportional constant for integrating controller.
	float pidKi;
	float pidKd;
	float tbhGain;
	float tbhApprox;
	float bangBangValue;
	float gearing;                      // Ratio of flywheel RPM per encoder RPM.
	float encoderTicksPerRevolution;    // Number of ticks each time the encoder completes one revolution
	float smoothing;                    // Amount of smoothing applied to the flywheel RPM, which is the low-pass filter time constant in seconds.

	bool ready;                         // Whether the controller is in ready mode (true, flywheel at the right speed) or active mode (false), which affects task priority and update rate.
	unsigned long delay;
	bool allowReadify;

	Semaphore readySemaphore;
	Semaphore stepSemaphore;

	ControllerType controllerType;

	Mutex targetMutex;                  // Mutex for updating the target speed.
	TaskHandle task;                    // Handle to the controlling task.
	Encoder encoder;                    // Encoder used to measure the rpm.
	unsigned char motorChannels[4];
	bool motorReversed[4];
}
Flywheel;

typedef struct FlywheelSetup
{
	float gearing;                      // Ratio of flywheel RPM per encoder RPM.
	float pidKp;
	float pidKi;
	float pidKd;
	float tbhGain;
	float tbhApprox;
	float bangBangValue;
	float smoothing;                    // Amount of smoothing applied to the flywheel RPM, as the low-pass time constant in seconds.
	float encoderTicksPerRevolution;    // Number of ticks each time the encoder completes one revolution
	unsigned char encoderPortTop;       // Digital port number where the encoder's top wire is connected.
	unsigned char encoderPortBottom;    // Digital port number where the encoder's bottom wire is connected. 
	unsigned char motorChannels[4];
	bool encoderReverse;                // Whether the encoder values should be reversed.
	bool motorReversed[4];
}
FlywheelSetup;

Flywheel *flywheelInit(FlywheelSetup setup);

void flywheelRun(Flywheel *flywheel);

// Sets target RPM
void flywheelSet(Flywheel *flywheel, float rpm);

void flywheelSetController(Flywheel *flywheel, ControllerType type);
void flywheelSetSmoothing(Flywheel *flywheel, float smoothing);
void flywheelSetPidKp(Flywheel *flywheel, float gain);
void flywheelSetPidKi(Flywheel *flywheel, float gain);
void flywheelSetPidKd(Flywheel *flywheel, float gain);
void flywheelSetTbhGain(Flywheel *flywheel, float gain);
void flywheelSetTbhApprox(Flywheel *flywheel, float approx);
void flywheelSetAllowReadify(Flywheel *flywheel, bool isAllowed);
void waitUntilFlywheelReady(Flywheel *flywheel, const unsigned long blockTime);
void waitUntilFlywheelStep(Flywheel *flywheel, const unsigned long blockTime);

// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

#ifndef FLYWHEEL_H_
#define FLYWHEEL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef void* Flywheel;

typedef struct FlywheelSetup
{
	float gearing;                      // Ratio of flywheel RPM per encoder RPM.
	float gain;                         // Gain proportional constant for integrating controller.
	float smoothing;                    // Amount of smoothing applied to the flywheel RPM, as the low-pass time constant in seconds.
	float encoderTicksPerRevolution;    // Number of ticks each time the encoder completes one revolution
	unsigned char encoderPortTop;       // Digital port number where the encoder's top wire is connected.
	unsigned char encoderPortBottom;    // Digital port number where the encoder's bottom wire is connected.  
	bool encoderReverse;                // Whether the encoder values should be reversed.
}
FlywheelSetup;

Flywheel flywheelInit(FlywheelSetup setup);

// Sets target RPM
void flywheelSet(Flywheel flywheel, float rpm);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

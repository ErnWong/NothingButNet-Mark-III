// 
// Motor model.
//
// This is part of a rewrite of James Pearman (aka Jpearman)'s Smart Motor Library,
// which is based on and uses Chris Siegert (aka vamfun)'s model of the motor and PTC.
// https://vamfun.wordpress.com/2012/07/18/estimating-the-ptc-temperature-using-motor-current-first-try-no-luck/
// https://vamfun.wordpress.com/2012/07/21/derivation-of-formulas-to-estimate-h-bridge-controller-current-vex-jaguarvictor-draft/
// https://github.com/jpearman/smartMotorLib/blob/master/SmartMotorLib.c
// http://www.vexforum.com/showthread.php?t=74659
// http://www.vexforum.com/showthread.php?t=73960
//


#ifndef MOTOR_MODEL_H_
#define MOTOR_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



#define MOTOR_SYSTEM_RESISTANCE ( (float)    0.30f )    // Resistance of the rest of the system for the model.
#define MOTOR_PWM_FREQUENCY     ( (float) 1150.00f )    // Frequency of the PWM pulse cycle.
#define MOTOR_DIODE_VOLTAGE     ( (float)    0.75f )    // Voltage across the model's diode during the off phase.

#define MOTOR_COMMAND_MAX       ( (int) 127 )   // Maximum command that can be given to a motor.



typedef struct MotorModel
{
	int command;                        // Motor command (from -127 to +127) given to the motor.
	int direction;                      // Direction of the motor rotation and current.

	float dutyOn;                       // Proportion of the cycle where the PWM pulse is high.
	float dutyOff;                      // Proportion of the cycle where the PWM pulse is low and the current have yet to reach zero.

	const float backEmfMax;             // Maximum possible EMF that can be generated from a free spinning motor.
	const float backEmfPerRpm;          // Back EMF constant (Volts per RPM), a property of the motor, that is the amount of EMF generated per RPM the motor is spinning.
	float backEmf;                      // EMF generated back from a spinning motor.

	const float resistance;             // Resistance of the motor model.
	const float inductance;             // Inductance of the motor model.

	float currentSteadyStateOn;         // Current when the PWM pulse is held high and the system reaches a steady state.
	float currentSteadyStateOff;        // Current when the PWM pulse is held low and the system reaches a steady state.

	const float lambda;                 // Ratio of PWM period relative to the motor time constant (time constant = inductance / resistance).

	float contributionPeak;             // Contribution ratio of (initial current) / (initial current + on-phase steady state current) of the current at the end of the on-phase (peak current).
	float contributionInitial;          // Contribution ratio of (peak current) / (peak current + off-phase steady state current) of the current at the end of the off-phase (final = initial current).

	float currentInitial;               // Current at the begining of each PWM cycle.
	float currentPeak;                  // Peak current at the end of the on phase and the start of the off phase.

	const float smoothing;              // Amount of smoothing (low-pass filter time constant) applied to current to get currentFiltered.
	float current;                      // Average current, the raw approximation without filtering.
	float currentFiltered;              // Average current, filtered with a low-pass filter.

	bool commandNeedsScaling;			// Whether the motor is connected to a port between 2 through to 9, in which case their commands need rescaling.
}
MotorModel;



typedef struct MotorModelSetup
{
	float backEmfPerRpm;			// Back EMF constant (Volts per RPM), a property of the motor, that is the amount of EMF generated per RPM the motor is spinning.
	float resistance;				// Resistance of the motor model.
	float inductance;				// Inductance of the motor model.
	float smoothing;				// Amount of smoothing (low-pass filter time constant) applied to current to get currentFiltered.
	float rpmFree;					// Maximum rpm of the motor running freely.
	unsigned char channel;			// Channel to which the motor is connected to.
}
MotorModelSetup;


typedef struct MotorModelMeasurements
{
	int command;
	float rpm;
	float batteryVoltage;
	float timeChange;
}
MotorModelMeasurements;


// Also returns the amount of current.
//float motorModelUpdate(MotorModel *m, MotorModelMeasurements measurements);
float motorModelUpdate(MotorModel *m, int command, float rpm, float batteryVoltage, float timeChange);


// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

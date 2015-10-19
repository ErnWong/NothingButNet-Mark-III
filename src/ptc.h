// 
// PTC temperature and tripping model.
//
// This is part of a rewrite of James Pearman (aka Jpearman)'s Smart Motor Library,
// which is based on and uses Chris Siegert (aka vamfun)'s model of the motor and PTC.
// https://vamfun.wordpress.com/2012/07/18/estimating-the-ptc-temperature-using-motor-current-first-try-no-luck/
// https://vamfun.wordpress.com/2012/07/21/derivation-of-formulas-to-estimate-h-bridge-controller-current-vex-jaguarvictor-draft/
// https://github.com/jpearman/smartMotorLib/blob/master/SmartMotorLib.c
// http://www.vexforum.com/showthread.php?t=74659
// http://www.vexforum.com/showthread.php?t=73960
//


#ifndef PTC_H_
#define PTC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



#define PTC_TEMPERATURE_AMBIENT ( (float)  20.0f )      // Temperature of the surroundings (in deg C).
#define PTC_TEMPERATURE_TRIP    ( (float) 100.0f )      // Temperature over which a PTC will trip (in deg C).
#define PTC_TEMPERATURE_UNTRIP  ( (float)  90.0f )      // Temperature under which a tripped PTC will untrip (in deg C).


// Calculates the tau constant, parameter of the PTC.
#define PTC_CONSTANT_TAU(kTau, tTrip)       ((kTau) * (tTrip) * 5.0 * 5.0)

// Calculates the constant1 parameter of the PTC.
#define PTC_CONSTANT_1(tTrip, tRef, iHold)  (((tTrip) - (tRef)) / ((iHold) * (iHold)))

// Calculates the constant2 parameter of the PTC.
#define PTC_CONSTANT_2(tau)                 (1.0f / (tau))




//
// PTC temperature and tripping model.
//
typedef struct Ptc
{
	const float constant1;      // Reciprocal of the dissipation constant, 1 / k = (T_c - T_0) / (I_hold)^2 .
	const float constant2;      // Dissipation constant per specific heat, 1 / tau, where tau = 0.5 * (I_trip / I_0)^2 * t_trip .
	float ambient;              // Temperature of the surroundings (in deg C).
	float temperature;          // PTC temperature (in deg C)
	bool tripped;               // Whether the PTC had tripped.
}
Ptc;


typedef struct PtcSetup
{
	float ambient;              // Temperature of the surroundings. (in deg C);
	float constant1;            // Reciprocal of the dissipation constant, 1 / k = (T_c - T_0) / (I_hold)^2 .
	float constant2;            // Dissipation constant per specific heat, 1 / tau, where tau = 0.5 * (I_tri / I_0)^2 * t_trip .
}
PtcSetup;


void ptcInit(Ptc *ptc, PtcSetup setup);


// Also returns the trip status.
bool ptcUpdate(Ptc *ptc, float current, float timeChange);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

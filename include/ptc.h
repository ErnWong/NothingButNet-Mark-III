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


extern const float PTC_TEMPERATURE_AMBIENT;
extern const float PTC_TEMPERATURE_TRIP;
extern const float PTC_TEMPERATURE_UNTRIP;

extern const float PTC_TEMPERATURE_REFERENCE;

extern const float PTC_CURRENT_HOLD_CORTEX;
extern const float PTC_TIME_TRIP_CORTEX;
extern const float PTC_K_TAU_CORTEX;
extern const float PTC_CONSTANT_TAU_CORTEX;
extern const float PTC_CONSTANT_1_CORTEX;
extern const float PTC_CONSTANT_2_CORTEX;

extern const float PTC_CURRENT_HOLD_393;
extern const float PTC_TIME_TRIP_393;
extern const float PTC_K_TAU_393;
extern const float PTC_CONSTANT_TAU_393;
extern const float PTC_CONSTANT_1_393;
extern const float PTC_CONSTANT_2_393;

extern const float PTC_CURRENT_HOLD_269;
extern const float PTC_TIME_TRIP_269;
extern const float PTC_K_TAU_269;
extern const float PTC_CONSTANT_TAU_269;
extern const float PTC_CONSTANT_1_269;
extern const float PTC_CONSTANT_2_269;

extern const float PTC_CONSTANT_1_3WIRE;
extern const float PTC_CONSTANT_2_3WIRE;

// Calculates the tau constant, parameter of the PTC.
#define PTC_CONSTANT_TAU(kTau, tTrip) ((kTau) * (tTrip) * 5.0 * 5.0)

// Calculates the constant-1 parameter of the PTC.
#define PTC_CONSTANT_1(tTrip, iHold) (((tTrip) - PTC_TEMPERATURE_REFERENCE) / ((iHold) * (iHold)))

// Calculates the constant-2 parameter of the PTC.
#define PTC_CONSTANT_2(tau) (1.0f / (tau))


//
// PTC temperature and tripping model.
//
typedef struct
Ptc
{
    float constant1;
    float constant2;
    float ambient;
    float temperature;
    bool tripped;
}
Ptc;


typedef struct
PtcSetup
{
    float ambient;
    float constant1;
    float constant2;
}
PtcSetup;


void
ptcInit(Ptc * ptc, PtcSetup setup);


// Also returns the trip status.
bool
ptcUpdate(Ptc * ptc, float current, float timeChange);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

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


#include "ptc.h"

#include <stdbool.h>

const float PTC_TEMPERATURE_AMBIENT = 20.0f;
const float PTC_TEMPERATURE_TRIP = 100.0f;
const float PTC_TEMPERATURE_UNTRIP = 90.0f;

const float PTC_TEMPERATURE_REFERENCE = 25.0f;

// PTC HR16-400 - cortex and power expander
const float PTC_CURRENT_HOLD_CORTEX = 3.0f;
const float PTC_TIME_TRIP_CORTEX = 1.7f;
const float PTC_K_TAU_CORTEX = 0.5f;
const float PTC_CONSTANT_TAU_CORTEX = PTC_CONSTANT_TAU(PTC_K_TAU_CORTEX, PTC_TIME_TRIP_CORTEX);
const float PTC_CONSTANT_1_CORTEX = PTC_CONSTANT_1(PTC_TIME_TRIP_CORTEX, PTC_CURRENT_HOLD_CORTEX);
const float PTC_CONSTANT_2_CORTEX = PTC_CONSTANT_2(PTC_TAU_CORTEX);

// PTC HR30-090
const float PTC_CURRENT_HOLD_393 = 1.0f;
const float PTC_TIME_TRIP_393 = 7.1f;
const float PTC_K_TAU_393 = 0.5f;
const float PTC_CONSTANT_TAU_393 = PTC_CONSTANT_TAU(PTC_K_TAU_393, PTC_TIME_TRIP_393);
const float PTC_CONSTANT_1_393 = PTC_CONSTANT_1(PTC_TIME_TRIP_393, PTC_CURRENT_HOLD_393);
const float PTC_CONSTANT_2_393 = PTC_CONSTANT_2(PTC_TAU_393);

// PTC HR16-075
const float PTC_CURRENT_HOLD_269 = 0.75f;
const float PTC_TIME_TRIP_269 = 2.0f;
const float PTC_K_TAU_269 = 0.5f;
const float PTC_CONSTANT_TAU_269 = PTC_CONSTANT_TAU(PTC_K_TAU_269, PTC_TIME_TRIP_269);
const float PTC_CONSTANT_1_269 = PTC_CONSTANT_1(PTC_TIME_TRIP_269, PTC_CURRENT_HOLD_269);
const float PTC_CONSTANT_2_269 = PTC_CONSTANT_2(PTC_TAU_269);

// Approx PTC MINISMDC-075F
const float PTC_CONSTANT_1_3WIRE = PTC_CONSTANT_1_269;
const float PTC_CONSTANT_2_3WIRE = PTC_CONSTANT_2_269;

static void updateTemperature(Ptc*, float current, float timeChange);
static void updateTripStatus(Ptc*);

void
ptcInit(Ptc * ptc, PtcSetup setup)
{
    ptc->constant1 = setup.constant1;
    ptc->constant2 = setup.constant2;

    ptc->ambient = setup.ambient;
    ptc->temperature = ptc->ambient;

    ptc->tripped = false;
}


// Also returns the trip status.
bool
ptcUpdate(Ptc * ptc, float current, float timeChange)
{
    updateTemperature(ptc, current, timeChange);
    updateTripStatus(ptc);
    return ptc->tripped;
}


static void
updateTemperature(Ptc * ptc, float current, float timeChange)
{
    float heatLoss = ptc->temperature - ptc->ambient;

    float heatGain = current * current * ptc->constant1;

    float rate = ptc->constant2 * (heatGain - heatLoss);

    ptc->temperature += timeChange * rate;
}


static void
updateTripStatus(Ptc * ptc)
{
    // Tripping
    if (!ptc->tripped && ptc->temperature > PTC_TEMPERATURE_TRIP)
    {
        ptc->tripped = true;
    }

    // Un-tripping, below hysterisis
    else if (ptc->tripped && ptc->temperature < PTC_TEMPERATURE_UNTRIP)
    {
        ptc->tripped = false;
    }
}

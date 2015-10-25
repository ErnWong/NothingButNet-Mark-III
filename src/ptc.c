#if 0

// {{{ Disabled for now


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



void ptcInit(Ptc *ptc, PtcSetup setup)
{
	*(float *)&ptc->constant1 = setup.constant1;
	*(float *)&ptc->constant2 = setup.constant2;

	ptc->ambient = setup.ambient;
	ptc->temperature = ptc->ambient;

	ptc->tripped = false;
}


// Also returns the trip status.
bool ptcUpdate(Ptc *ptc, float current, float timeChange)
{

	updateTemperature(ptc, current, timeChange);
	updateTripStatus(ptc);

	return ptc->tripped;
}


void updateTemperature(Ptc *ptc, float current, float timeChange)
{
	float heatLoss = ptc->temperature - ptc->ambient;

	float heatGain = current * current * ptc->constant1;

	float rate = ptc->constant2 * (heatGain - heatLoss);

	ptc->temperature += timeChange * rate;
}


void updateTripStatus(Ptc *ptc)
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


// }}}

#endif

//
// Motor model.
//
// This is part of a port of James Pearman (aka Jpearman)'s Smart Motor Library,
// which is based on and uses Chris Siegert (aka vamfun)'s model of the motor and PTC.
// https://vamfun.wordpress.com/2012/07/18/estimating-the-ptc-temperature-using-motor-current-first-try-no-luck/
// https://vamfun.wordpress.com/2012/07/21/derivation-of-formulas-to-estimate-h-bridge-controller-current-vex-jaguarvictor-draft/
// https://github.com/jpearman/smartMotorLib/blob/master/SmartMotorLib.c
// http://www.vexforum.com/showthread.php?t=74659
// http://www.vexforum.com/showthread.php?t=73960
//


#include "motor-model.h"

#include <stdbool.h>
#include <math.h>

const float MOTOR_SYSTEM_RESISTANCE = 0.3f;
const float MOTOR_PWM_FREQUENCY = 1150.0f;
const float MOTOR_DIODE_VOLTAGE = 0.75f;
const int MOTOR_COMMAND_MAX = 127;

static void updateCommand(MotorModel*, int command);
static void updateDirection(MotorModel*, float rpm);
static void updateDutyOn(MotorModel*);
static void updateBackEmf(MotorModel*, float rpm);
static void updateSteadyStateCurrents(Motor Model*, float batteryVoltage);
static void updateConstants(MotorModel*);
static void updateDutyOff(MotorModel*);
static void updateCurrent(MotorModel*, float timeChange);


void
motorModelInit(MotorModel * m, MotorModelSetup setup)
{
    m->commandNeedsScaling = 2 <= setup.channel && setup.channel <= 9;
    m->command = 0;
    m->direction = 0;

    m->dutyOn = 0;
    m->dutyOff = 0;

    m->backEmfPerRpm = setup.backEmfPerRpm;
    m->backEmfMax = m->backEmfPerRpm * setup.rpmFree;
    m->backEmf = 0;

    m->resistance = setup.resistance;
    m->inductance = setup.inductance;

    m->currentSteadyStateOn = 0;
    m->currentSteadyStateOff = 0;

    // PWM period relative to time constant, time constant = inductance / resistance
    m->lambda = m->resistance / (MOTOR_PWM_FREQUENCY * m->inductance);

    m->contributionPeak = 0;
    m->contributionInitial = 0;

    m->currentInitial = 0;
    m->currentPeak = 0;

    m->smoothing = setup.smoothing;
    m->current = 0;
    m->currentFiltered = 0;
}




// Also returns the amount of current.
float
motorModelUpdate(MotorModel * m, MotorModelMeasurements measurements)
{
    updateCommand(m, measurements.command);
    updateDirection(m, measurements.rpm);
    updateDutyOn(m);
    updateConstants(m);
    updateBackEmf(m, measurements.rpm);
    updateSteadyStateCurrents(m, measurements.batteryVoltage);
    updateDutyOff(m);
    updateCurrent(m, measurements.timeChange);
    return m->current;
}



static void
updateCommand(MotorModel * m, int command)
{
    if (m->commandNeedsScaling)
    {
        // Rescale
        m->command = (command * 128) / 90;

        // Clip
        if (abs(m->command) > MOTOR_COMMAND_MAX)
        {
            m->command = signOf(m->command) * MOTOR_COMMAND_MAX;
        }
    }
    else
    {
        m->command = command;
    }
}


static void
updateDirection(MotorModel * m, float rpm)
{
    if (abs(m->command) > 10)
    {
        m->direction = signOf(m->command);
    }
    else
    {
        // Use rpm to reduce transients
        m->direction = signOf(rpm);
    }
}


static void
updateDutyOn(MotorModel * m)
{
    m->dutyOn = abs(m->command) / (float)MOTOR_COMMAND_MAX;
}


static void
updateBackEmf(MotorModel * m, float rpm)
{
    m->backEmf = m->backEmfPerRpm * rpm;

    // Clip
    if (abs(m->backEmf) > m->backEmfMax)
    {
        m->backEmf = copysignf(m->backEmfMax, m->backEmf);
    }
}


static void
updateSteadyStateCurrents(MotorModel * m, float batteryVoltage)
{
    // On phase
    float emf = batteryVoltage * m->direction - m->backEmf;
    float resistance = m->resistance + MOTOR_SYSTEM_RESISTANCE;
    m->currentSteadyStateOn = emf / resistance;

    // Off phase
    emf = -MOTOR_DIODE_VOLTAGE * m->direction - m->backEmf;
    resistance = m->resistance;
    m->currentSteadyStateOff = emf / resistance;
}


static void
updateConstants(MotorModel * m)
{
    //TODO: isn't this constant constant??
    //m->lambda = m->resistance / (MOTOR_PWM_FREQUENCY * m->inductance);
    m->contributionPeak = exp(-m->lambda * m->dutyOn);
    m->contributionInitial = exp(-m->lambda * (1 - m->dutyOn));
}


// Calculates initial current, peak current, duty off period.
static void
updateDutyOff(MotorModel * m)
{
    // Calculate initial current as if equal to final current
    float onCurrentChange = m->currentSteadyStateOn * (1 - m->contributionPeak) * m->contributionInitial;
    float offCurrentChange = m->currentSteadyStateOff * (1 - m->contributionInitial) / (1 - m->contributionPeak * m->contributionInitial);
    m->currentInitial = onCurrentChange + offCurrentChange;

    // Test to see if the circuit is continuous, or clipped by the diode
    bool crossedZero = m->currentInitial * m->direction < 0;
    if (crossedZero)
    {
        // Diode comes into play, and clips current
        m->currentInitial = 0;
        m->currentPeak = m->currentSteadyStateOn * (1 - m->contributionPeak);
        m->dutyOff = -log(-m->currentSteadyStateOff / (m->currentPeak - m->currentSteadyStateOff)) / m->lambda;
    }
    else
    {
        // Otherwise the rest of the time is dutyOff
        m->currentPeak = m->currentInitial * m->contributionPeak + m->currentSteadyStateOn * (1 - m->contributionPeak);
        m->dutyOff = 1 - m->dutyOn;
    }
}


static void
updateCurrent(MotorModel * m, float timeChange)
{
    // Average current (the other terms cancel out)
    float on = m->currentSteadyStateOn * m->dutyOn;
    float off = m->currentSteadyStateOff * m->dutyOff;
    m->current = on + off;

    // Low-pass filter to remove transients
    m->currentFiltered += (m->current - m->currentFiltered) * timeChange / m->smoothing;
}

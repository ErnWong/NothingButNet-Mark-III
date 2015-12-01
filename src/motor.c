#include <API.h>

#include "motor-model.h"
#include "ptc.h"
#include "utils.h"

// TODO: create something like an infographic that explains the model.


const int SMARTMOTOR_COMMAND_NO_LIMIT = 255;
const int SMARTMOTOR_COMMAND_MAX = 127;
const int SMARTMOTOR_COMMAND_MIN = -127;
const float SMARTMOTOR_CURRENT_UNTRIP_FACTOR = 0.9f;

typedef enum
SmartMotorLimitType
{
    SMARTMOTOR_LIMIT_NONE,
    SMARTMOTOR_LIMIT_CURRENT,
    SMARTMOTOR_LIMIT_PTC
}
SmartMotorLimitType;

typedef enum
SmartMotorSensorType
{
    SMARTMOTOR_SENSOR_NONE,
    SMARTMOTOR_SENSOR_IME,
    SMARTMOTOR_SENSOR_QUADRATURE,
    SMARTMOTOR_SENSOR_ANALOG
}
SmartMotorSensorType;

typedef struct
SmartMotor
{
    int command;
    unsigned long microTime;
    float timeChange;
    MotorModel model;
    SmartController *bank;
    Ptc ptc;
    float temperature;
    float targetCurrent;
    float safeCurrent;
    float limitCurrent;
    int commandLimit;
    unsigned char channel;
    float rpm;

    bool limitTripped;

    SmartMotorLimitType limitType;

    SmartMotorSensorType sensorType;
    void *sensor;
}
SmartMotor;

typedef struct
SmartController
{
    Ptc ptc;
}
SmartController;

void
update(SmartMotor * m)
{
    float batteryVoltage;
    measure(m, &batteryVoltage);

    motorModelUpdate(&m->model, m->command, m->rpm, batteryVoltage, m->timeChange);
    ptcUpdate(&m->ptc, m->model.current, m->timeChange);

    switch (m->limitType)
    {
    case SMARTMOTOR_LIMIT_PTC:
        monitorPtc(m, batteryVoltage);
        break;
    case SMARTMOTOR_LIMIT_CURRENT:
        monitorCurrent(m, batteryVoltage);
        break;
    }
}

void
measure(SmartMotor * m, float * batteryVoltage)
{
    m->timeChange = timeUpdate(&m->microTime);

    *batteryVoltage = powerLevelMain() / 1000;

    m->command = motorGet(m->channel);

    measureRpm(m);
}

void
measureRpm(SmartMotor * m)
{
    switch (m->sensorType)
    {

    }
}

void
monitorCurrent(SmartMotor * m, float batteryVoltage)
{
    m->targetCurrent = m->limitCurrent;

    checkCurrent(m);

    if (m->limitTripped)
    {
        m->commandLimit = getCommandSafeLimit(m, batteryVoltage);
    }
    else
    {
        m->commandLimit = SMARTMOTOR_COMMAND_NO_LIMIT;
    }
}

void
checkCurrent(SmartMotor * m)
{
    if (!m->limitTripped)
    {
        if (abs(m->model.currentFiltered) > m->targetCurrent)
        {
            m->limitTripped = true;
        }
    }
    else
    {
        if (abs(m->model.currentFiltered) < m->targetCurrent * SMARTMOTOR_CURRENT_UNTRIP_FACTOR)
        {
            m->limitTripped = false;
        }
    }
}

void
monitorPtc(SmartMotor * m, float batteryVoltage)
{

    if (m->bank && m->bank->ptc.tripped)
    {
        return;
    }

    if (m->ptc.tripped)
    {
        m->targetCurrent = m->safeCurrent; // TODO: is this constant?
        m->commandLimit = getCommandSafeLimit(m, batteryVoltage);
    }
    else
    {
        // Remove the limit
        m->commandLimit = SMARTMOTOR_COMMAND_NO_LIMIT;
    }
}


int
getCommandSafeLimit(SmartMotor * m, float batteryVoltage)
{
    int command = motorGet(m->channel);

    if (m->targetCurrent == 0)
    {
        return 0;
    }

    // Command polarity must match RPM polarity.
    if (command >= 0)
    {
        if (m->rpm >= 0)
        {
            command = getCommandSafeLimitForward(m, batteryVoltage);
        }
        else
        {
            command = SMARTMOTOR_COMMAND_MAX;
        }
    }
    else
    {
        if (m->rpm <= 0)
        {
            command = getCommandSafeLimitBackward(m, batteryVoltage);
        }
        else
        {
            command = SMARTMOTOR_COMMAND_MIN;
        }
    }

    // Ports 2 through to 9 behave a little differently.
    if (2 <= m->channel <= 9)
    {
        return (command * 90) / 128;
    }
    return command;
}


int
getCommandSafeLimitForward(SmartMotor * m, float batteryVoltage)
{
    float resistance = m->model.resistance + MOTOR_SYSTEM_RESISTANCE;
    float voltageResistive = m->targetCurrent * resistance;

    float voltageDutyOn = m->model.backEmf + voltageResistive + MOTOR_DIODE_VOLTAGE;
    float voltageAvailable = batteryVoltage + MOTOR_DIODE_VOLTAGE;

    float dutyOn = voltageDutyOn / voltageAvailable;
    int command = SMARTMOTOR_COMMAND_MAX * dutyOn;

    // Clip
    if (command > SMARTMOTOR_COMMAND_MAX)
    {
        return SMARTMOTOR_COMMAND_MAX;
    }
    return command;
}


int
getCommandSafeLimitBackward(SmartMotor * m, float batteryVoltage)
{
    float resistance = m->model.resistance + MOTOR_SYSTEM_RESISTANCE;
    float voltageResistive = m->targetCurrent * resistance;

    float voltageDutyOn = m->model.backEmf - voltageResistive - MOTOR_DIODE_VOLTAGE;
    float voltageAvailable = batteryVoltage + MOTOR_DIODE_VOLTAGE;

    float dutyOn = voltageDutyOn / voltageAvailable;
    int command = SMARTMOTOR_COMMAND_MAX * dutyOn;

    // Clip
    if (command < SMARTMOTOR_COMMAND_MIN)
    {
        return SMARTMOTOR_COMMAND_MIN;
    }
    return command;
}

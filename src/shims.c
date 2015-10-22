#include "shims.h"

#include <API.h>
#include <stdbool.h>
#include "utils.h"


typedef struct
EncoderArgs
{
    Encoder encoder;
    int reading;
    unsigned long microTime;
}
EncoderArgs;

float
encoderGetter(void * args)
{
    EncoderArgs * encoderArgs = args;

    float timeChange = timeUpdate(&encoderArgs->microTime);
    int reading = encoderGet(encoderArgs->encoder);
    int ticks = reading - encoderArgs->reading;

    encoderArgs->reading = reading;
    return ticks / ENCODER_TICKS_PER_REV / timeChange;
}

void
encoderResetter(void * args)
{
    EncoderArgs * encoderArgs = args;
    encoderReset(encoderArgs->encoder);
}

void *
encoderGetArgs(Encoder encoder)
{
    EncoderArgs * args = malloc(sizeof(EncoderArgs));
    args->encoder = encoder;
    args->reading = encoderGet(encoder);
    args->microTime = micros();
    return args;
}

typedef struct
ImeArgs
{
    unsigned char address;
    float gearing;
}
ImeArgs;

float
imeGetter(void * args)
{
    ImeArgs * imeArgs = args;
    int rpm = 0;
    int i = 2;
    while (i > 0)
    {
        bool success = imeGetVelocity(imeArgs->address, &rpm);
        if (success) break;
        i--;
    }
    return ((float)rpm) / imeArgs->gearing;
}

void
imeResetter(void * args)
{
    ImeArgs * imeArgs = args;
    int i = 2;
    while (i > 0)
    {
        bool success = imeReset(imeArgs->address);
        if (success) break;
        i--;
    }
}

void *
imeGetArgs(unsigned char address, MotorType type)
{
    ImeArgs * args = malloc(sizeof(ImeArgs));
    args->address = address;
    switch (type)
    {
    case MOTOR_TYPE_269:
        args->gearing = IME_269_GEARING;
        break;
    case MOTOR_TYPE_393_TORQUE:
        args->gearing = IME_393_TORQUE_GEARING;
        break;
    case MOTOR_TYPE_393_SPEED:
        args->gearing = IME_393_SPEED_GEARING;
        break;
    }
    return args;
}

typedef struct
MotorArgs
{
    unsigned char channel;
    bool reversed;
}
MotorArgs;

void
motorSetter(void * args, int command)
{
    MotorArgs * motorArgs = args;
    if (motorArgs->reversed) command *= -1;
    motorSet(motorArgs->channel, command);
}

void *
motorGetArgs(unsigned char channel, bool reversed)
{
    MotorArgs * args = malloc(sizeof(MotorArgs));
    args->channel = channel;
    args->reversed = reversed;
    return args;
}

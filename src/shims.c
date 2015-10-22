#include "shims.h"

#include <API.h>
#include <stdbool.h>
#include "utils.h"


typedef struct
EncoderShim
{
    Encoder encoder;
    int reading;
    unsigned long microTime;
}
EncoderShim;

float
encoderGetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;

    float timeChange = timeUpdate(&shim->microTime);
    int reading = encoderGet(shim->encoder);
    int ticks = reading - shim->reading;

    shim->reading = reading;
    return ticks / ENCODER_TICKS_PER_REV / timeChange;
}

void
encoderResetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;
    encoderReset(shim->encoder);
}

void *
encoderGetHandle(Encoder encoder)
{
    EncoderShim * shim = malloc(sizeof(EncoderHandle));
    shim->encoder = encoder;
    shim->reading = encoderGet(encoder);
    shim->microTime = micros();
    return shim;
}

typedef struct
ImeShim
{
    unsigned char address;
    float gearing;
}
ImeShim;

float
imeGetter(EncoderHandle handle)
{
    ImeShim * shim = handle;
    int rpm = 0;
    int i = 2;
    while (i > 0)
    {
        bool success = imeGetVelocity(shim->address, &rpm);
        if (success) break;
        i--;
    }
    return ((float)rpm) / shim->gearing;
}

void
imeResetter(EncoderHandle handle)
{
    ImeShim * shim = handle;
    int i = 2;
    while (i > 0)
    {
        bool success = imeReset(shim->address);
        if (success) break;
        i--;
    }
}

void *
imeGetHandle(unsigned char address, MotorType type)
{
    ImeShim * shim = malloc(sizeof(ImeShim));
    shim->address = address;
    switch (type)
    {
    case MOTOR_TYPE_269:
        shim->gearing = IME_269_GEARING;
        break;
    case MOTOR_TYPE_393_TORQUE:
        shim->gearing = IME_393_TORQUE_GEARING;
        break;
    case MOTOR_TYPE_393_SPEED:
        shim->gearing = IME_393_SPEED_GEARING;
        break;
    }
    return shim;
}

typedef struct
MotorShim
{
    unsigned char channel;
    bool reversed;
}
MotorShim;

void
motorSetter(MotorHandle handle, int command)
{
    MotorShim * shim = handle;
    if (shim->reversed) command *= -1;
    motorSet(shim->channel, command);
}

void *
motorGetHandle(unsigned char channel, bool reversed)
{
    MotorShim * shim = malloc(sizeof(MotorShim));
    shim->channel = channel;
    shim->reversed = reversed;
    return shim;
}

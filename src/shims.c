#include "shims.h"

#include <API.h>
#include <stdbool.h>
#include "utils.h"


const float SHIM_DEGREES_PER_REV = 360.0f;
const float SHIM_RADIANS_PER_REV = 6.283185307179586f;

const float TICKS_PER_REV_ENCODER = 360.0f;
const float TICKS_PER_REV_IME_269 = 240.448f;
const float TICKS_PER_REV_IME_393_TORQUE = 627.2f;
const float TICKS_PER_REV_IME_393_SPEED = 392.0f;

const float ENCODER_GEARING_IME_269 = 30.056f;
const float ENCODER_GEARING_IME_393_TORQUE = 39.2f;
const float ENCODER_GEARING_IME_393_SPEED = 24.5f;

typedef struct
EncoderShim
{
    Encoder encoder;
    int ticks;
    unsigned long microTime;
}
EncoderShim;

EncoderReading
encoderGetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;

    float minutes = timeUpdate(&shim->microTime) / 60.0f;
    int ticks = encoderGet(shim->encoder);
    int ticksChange = ticks - shim->ticks;

    float revolutions = ((float)ticks) / TICKS_PER_REV_ENCODER;
    shim->ticks = ticks;

    float rpm = ticksChange / TICKS_PER_REV_ENCODER / minutes;
    EncoderReading reading =
    {
        .revolutions = revolutions,
        .rpm = rpm
    };
    return reading;
}

void
encoderResetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;
    shim->ticks = 0;
    encoderReset(shim->encoder);
}

EncoderHandle
encoderGetHandle(Encoder encoder)
{
    EncoderShim * shim = malloc(sizeof(EncoderShim));
    shim->encoder = encoder;
    shim->ticks = encoderGet(encoder);
    shim->microTime = micros();
    return shim;
}

typedef struct
ImeShim
{
    unsigned char address;
    float gearing;
    float ticksPerRevolution;
}
ImeShim;

EncoderReading
imeGetter(EncoderHandle handle)
{
    ImeShim * shim = handle;
    int angle = 0;
    int rpm = 0;
    int i;
    i = 2;
    while (i > 0)
    {
        bool success = imeGetVelocity(shim->address, &rpm);
        if (success) break;
        i--;
    }
    i = 2;
    while (i > 0)
    {
        bool success = imeGet(shim->address, &angle);
        if (success) break;
        i--;
    }
    EncoderReading reading =
    {
        .revolutions = ((float)angle) / shim->ticksPerRevolution,
        .rpm = ((float)rpm) / shim->gearing
    };
    return reading;
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

EncoderHandle
imeGetHandle(unsigned char address, MotorType type)
{
    ImeShim * shim = malloc(sizeof(ImeShim));
    shim->address = address;
    switch (type)
    {
    case MOTOR_TYPE_269:
        shim->gearing = ENCODER_GEARING_IME_269;
        shim->ticksPerRevolution = TICKS_PER_REV_IME_269;
        break;
    case MOTOR_TYPE_393_TORQUE:
        shim->gearing = ENCODER_GEARING_IME_393_TORQUE;
        shim->ticksPerRevolution = TICKS_PER_REV_IME_393_TORQUE;
        break;
    case MOTOR_TYPE_393_SPEED:
        shim->gearing = ENCODER_GEARING_IME_393_SPEED;
        shim->ticksPerRevolution = TICKS_PER_REV_IME_393_SPEED;
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

MotorHandle
motorGetHandle(unsigned char channel, bool reversed)
{
    MotorShim * shim = malloc(sizeof(MotorShim));
    shim->channel = channel;
    shim->reversed = reversed;
    return shim;
}

typedef struct
DigitalShim
{
    unsigned char port;
}
DigitalShim;

bool
digitalGetter(DigitalHandle handle)
{
    DigitalShim * shim = handle;
    return digitalRead(shim->port);
}

DigitalHandle
digitalGetHandle(unsigned char port)
{
    DigitalShim * shim = malloc(sizeof(DigitalShim));
    shim->port = port;

    return shim;
}

typedef struct
EncoderRangeShim
{
    EncoderGetter encoderGet;
    EncoderHandle encoder;
    float lower;
    float upper;
}
EncoderRangeShim;

bool
encoderRangeGetter(DigitalHandle handle)
{
    EncoderRangeShim * shim = handle;
    float revolutions = shim->encoderGet(shim->encoder).revolutions;
    float degrees = revolutions * SHIM_DEGREES_PER_REV;
    return shim->lower <= degrees && degrees <= shim->upper;
}

DigitalHandle
encoderRangeGetHandle(EncoderGetter encoderGetter, EncoderHandle encoder, float upper, float lower)
{
    EncoderRangeShim * shim = malloc(sizeof(EncoderRangeShim));
    shim->encoderGet = encoderGetter;
    shim->encoder = encoder;
    shim->upper = upper;
    shim->lower = lower;

    return shim;
}

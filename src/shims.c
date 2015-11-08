#include "shims.h"

#include <API.h>
#include <stdbool.h>
#include "utils.h"


const float SHIM_REVOLUTION = 360;

const float TICKS_PER_REV_ENCODER = 360;
const float TICKS_PER_REV_IME_269 = 240.448;
const float TICKS_PER_REV_IME_393_TORQUE = 627.2;
const float TICKS_PER_REV_IME_393_SPEED = 392;

const float ENCODER_GEARING_IME_269 = 30.056;
const float ENCODER_GEARING_IME_393_TORQUE = 39.2;
const float ENCODER_GEARING_IME_393_SPEED = 24.5;

typedef struct
EncoderShim
{
    Encoder encoder;
    int angle;
    unsigned long microTime;
}
EncoderShim;

EncoderReading
encoderGetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;

    float timeChange = timeUpdate(&shim->microTime);
    int angle = encoderGet(shim->encoder);
    printf("encodershim prevangle: %d\n", shim->angle);
    int ticks = angle - shim->angle;
    printf("encodershim ticks: %d\n", ticks);
    printf("encodershim angle: %d\n", angle);
    printf("encodershim timechange: %f\n", timeChange);

    shim->angle = angle;
    float rpm = ticks / TICKS_PER_REV_ENCODER / timeChange;
    EncoderReading reading =
    {
        .angle = ((float)angle) / TICKS_PER_REV_ENCODER * SHIM_REVOLUTION,
        .rpm = rpm
    };
    return reading;
}

void
encoderResetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;
    shim->angle = 0;
    encoderReset(shim->encoder);
}

void *
encoderGetHandle(Encoder encoder)
{
    EncoderShim * shim = malloc(sizeof(EncoderHandle));
    shim->encoder = encoder;
    shim->angle = encoderGet(encoder);
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
        .angle = ((float)angle) / shim->ticksPerRevolution * 360,
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

void *
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

void *
motorGetHandle(unsigned char channel, bool reversed)
{
    MotorShim * shim = malloc(sizeof(MotorShim));
    shim->channel = channel;
    shim->reversed = reversed;
    return shim;
}

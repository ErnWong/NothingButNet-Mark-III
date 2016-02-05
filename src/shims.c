#include "shims.h"

#include <API.h>
#include <stdbool.h>
#include "utils.h"


const float SHIM_DEGREES_PER_REV = 360.0f;
const float SHIM_RADIANS_PER_REV = TAU;

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
    Mutex mutex;
}
EncoderShim;

EncoderReading
encoderGetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;

    mutexTake(shim->mutex, -1);

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

    mutexGive(shim->mutex);

    return reading;
}

void
encoderResetter(EncoderHandle handle)
{
    EncoderShim * shim = handle;
    mutexTake(shim->mutex, -1);
    shim->ticks = 0;
    encoderReset(shim->encoder);
    mutexGive(shim->mutex);
}

EncoderHandle
encoderGetHandle(Encoder encoder)
{
    EncoderShim * shim = malloc(sizeof(EncoderShim));
    shim->encoder = encoder;
    shim->ticks = encoderGet(encoder);
    shim->microTime = micros();
    shim->mutex = mutexCreate();
    return shim;
}

typedef struct
ImeShim
{
    unsigned char address;
    float gearing;
    float ticksPerRevolution;
    Mutex mutex;
}
ImeShim;

EncoderReading
imeGetter(EncoderHandle handle)
{
    ImeShim * shim = handle;
    mutexTake(shim->mutex, -1);
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
    mutexGive(shim->mutex);
    return reading;
}

void
imeResetter(EncoderHandle handle)
{
    ImeShim * shim = handle;
    mutexTake(shim->mutex, -1);
    int i = 2;
    while (i > 0)
    {
        bool success = imeReset(shim->address);
        if (success) break;
        i--;
    }
    mutexGive(shim->mutex);
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
    shim->mutex = mutexCreate();
    return shim;
}

typedef struct
MotorShim
{
    unsigned char channel;
    bool reversed;
    Mutex mutex;
}
MotorShim;

void
motorSetter(MotorHandle handle, int command)
{
    MotorShim * shim = handle;
    mutexTake(shim->mutex, -1);
    printf(" - reversed: %s\n", shim->reversed? "yes" : "no");
    if (shim->reversed) command *= -1;
    motorSet(shim->channel, command);
    mutexGive(shim->mutex);
}

MotorHandle
motorGetHandle(unsigned char channel, bool reversed)
{
    MotorShim * shim = malloc(sizeof(MotorShim));
    shim->channel = channel;
    shim->reversed = reversed;
    shim->mutex = mutexCreate();
    return shim;
}

typedef struct
DigitalShim
{
    unsigned char port;
    bool negate;
    Mutex mutex;
}
DigitalShim;

bool
digitalGetter(DigitalHandle handle)
{
    DigitalShim * shim = handle;

    mutexTake(shim->mutex, -1);
    bool value = digitalRead(shim->port);
    mutexGive(shim->mutex);

    // negate if needed
    return shim->negate != value;
}

DigitalHandle
digitalGetHandle(unsigned char port, bool negate)
{
    DigitalShim * shim = malloc(sizeof(DigitalShim));
    shim->port = port;
    shim->negate = negate;
    shim->mutex = mutexCreate();

    return shim;
}

typedef struct
EncoderRangeShim
{
    EncoderGetter encoderGet;
    EncoderHandle encoder;
    float lower;
    float upper;
    Mutex mutex;
}
EncoderRangeShim;

bool
encoderRangeGetter(DigitalHandle handle)
{
    EncoderRangeShim * shim = handle;

    mutexTake(shim->mutex, -1);
    float revolutions = shim->encoderGet(shim->encoder).revolutions;
    mutexGive(shim->mutex);

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
    shim->mutex = mutexCreate();

    return shim;
}

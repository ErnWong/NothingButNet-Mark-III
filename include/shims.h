#ifndef ENCODER_H_
#define ENCODER_H_

#include <API.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



extern const float SHIM_DEGREES_PER_REV;
extern const float SHIM_RADIANS_PER_REV;

extern const float TICKS_PER_REV_ENCODER;
extern const float TICKS_PER_REV_IME_269;
extern const float TICKS_PER_REV_IME_393_TORQUE;
extern const float TICKS_PER_REV_IME_393_SPEED;

extern const float ENCODER_GEARING_IME_269;
extern const float ENCODER_GEARING_IME_393_TORQUE;
extern const float ENCODER_GEARING_IME_393_SPEED;


typedef void * EncoderHandle;
typedef void * MotorHandle;
typedef void * DigitalHandle;

typedef struct
EncoderReading
{
    float revolutions;
    float rpm;
}
EncoderReading;

typedef EncoderReading
(*EncoderGetter)(EncoderHandle handle);

typedef void
(*EncoderResetter)(EncoderHandle handle);

typedef void
(*MotorSetter)(MotorHandle handle, int command);

typedef bool
(*DigitalGetter)(DigitalHandle handle);

typedef enum
MotorType
{
    MOTOR_TYPE_269,
    MOTOR_TYPE_393_TORQUE,
    MOTOR_TYPE_393_SPEED
}
MotorType;

EncoderReading
encoderGetter(EncoderHandle);

void
encoderResetter(EncoderHandle);

EncoderHandle
encoderGetHandle(Encoder);

EncoderReading
imeGetter(EncoderHandle);

void
imeResetter(EncoderHandle);

EncoderHandle
imeGetHandle(unsigned char address, MotorType);

void
motorSetter(MotorHandle, int command);

MotorHandle
motorGetHandle(unsigned char channel, bool reversed);

bool
digitalGetter(DigitalHandle);

DigitalHandle
digitalGetHandle(unsigned char port, bool negate);

bool
encoderRangeGetter(DigitalHandle);

DigitalHandle
encoderRangeGetHandle(EncoderGetter, EncoderHandle, float upper, float lower);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

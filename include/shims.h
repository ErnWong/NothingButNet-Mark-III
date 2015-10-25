#ifndef ENCODER_H_
#define ENCODER_H_

#include <API.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif



#define ENCODER_TICKS_PER_REV 360
#define IME_269_GEARING 30.056
#define IME_393_TORQUE_GEARING 39.2
#define IME_393_SPEED_GEARING 24.5

typedef void * EncoderHandle;
typedef void * MotorHandle;

typedef float
(*EncoderGetter)(EncoderHandle handle);

typedef void
(*EncoderResetter)(EncoderHandle handle);

typedef void
(*MotorSetter)(MotorHandle handle, int command);

typedef enum
MotorType
{
    MOTOR_TYPE_269,
    MOTOR_TYPE_393_TORQUE,
    MOTOR_TYPE_393_SPEED
}
MotorType;

float
encoderGetter(EncoderHandle);

void
encoderResetter(EncoderHandle);

void *
encoderGetHandle(Encoder);

float
imeGetter(EncoderHandle);

void
imeResetter(EncoderHandle);

void *
imeGetHandle(unsigned char address, MotorType);

void
motorSetter(MotorHandle, int command);

void *
motorGetHandle(unsigned char channel, bool reversed);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

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

typedef float
(*EncoderGetter)(void * args);

typedef void
(*EncoderResetter)(void * args);

typedef void
(*MotorSetter)(void * args, int command);

typedef enum
MotorType
{
    MOTOR_TYPE_269,
    MOTOR_TYPE_393_TORQUE,
    MOTOR_TYPE_393_SPEED
}
MotorType;

float
encoderGetter(void * args);

void
encoderResetter(void * args);

void *
encoderGetArgs(Encoder);

float
imeGetter(void * args);

void
imeResetter(void * args);

void *
imeGetArgs(unsigned char address, MotorType);

void
motorSetter(void * args, int command);

void *
motorGetArgs(unsigned char channel, bool reversed);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

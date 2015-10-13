#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


//
// Updates the given variable with the current time in microseconds,
// and returns the time difference in seconds.
//
float timeUpdate(unsigned long *microTime);

int signOf(int x);

float stringToFloat(const char* string);

bool stringStartsWith(char const *pre, char const *string);
/*
bool stringCaseInsensitiveStartsWith(char const *pre, char const *string);

int stringCaseInsensitiveCompare(char const *a, char const *b, size_t maxCount);

*/

// TODO: Move these ticks per rev to somewhere meaningful.

#define TICKS_PER_REVOLUTION_MOTOR_269 (float)(240.448f)
#define TICKS_PER_REVOLUTION_MOTOR_393_TURBO (float)(261.333f)
#define TICKS_PER_REVOLUTION_MOTOR_393_SPEED (float)(392f)
#define TICKS_PER_REVOLUTION_MOTOR_393_STANDARD (float)(627.2f)
#define TICKS_PER_REVOLUTION_QUADRATURE (float)(360.0f)


// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

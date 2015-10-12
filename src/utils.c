#include "utils.h"

#include <API.h>




//
// Updates the given variable with the current time in microseconds,
// and returns the time difference in seconds.
//
float timeUpdate(unsigned long *microTime)
{
	unsigned long newMicroTime = micros();
	float change = (newMicroTime - *microTime) / 1000000.0f;
	*microTime = newMicroTime;

	return change;
}


int signOf(int x)
{
	return (x > 0) - (x < 0);
}
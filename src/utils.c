#include "utils.h"

#include <API.h>
#include <ctype.h>
#include <string.h>




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


// http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way

char *
trimSpaces(char * str)
{
    // Trim leading spaces
    while (isspace((unsigned char)*str)) str++;

    if (*str == '\0') return str;

    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}

#include "utils.h"

#include <API.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>




//
// Updates the given variable with the current time in microseconds,
// and returns the time difference in seconds.
//
float
timeUpdate(unsigned long * microTime)
{
	unsigned long newMicroTime = micros();
	float dt = (newMicroTime - *microTime) / 1000000.0f;

	*microTime = newMicroTime;

	return dt;
}


int
signOf(int x)
{
	return (x > 0) - (x < 0);
}

bool
isWithin(float x, float size)
{
    return -size < x && x < size;
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

//http://stackoverflow.com/questions/4392665/converting-string-to-float-without-atof-in-c
// And, using atof makes the linker complain about _sbrk not defined, so the lazy way is to write custom atof:
bool
stringToFloat(const char * string, float * dest)
{
    float rez = 0, factor = 1;
    if (*string == '-')
    {
        string++;
        factor = -1;
    }
    for (int pointSeen = 0; *string; string++)
    {
        if (*string == '.')
        {
            pointSeen = 1;
            continue;
        }
        int digit = *string - '0';
        if (digit >= 0 && digit <= 9)
        {
            if (pointSeen) {
                factor /= 10.0f;
            }
            rez = rez * 10.0f + (float)digit;
        }
        else return false;
    }
    *dest = rez * factor;
    return true;
};

bool
stringToUlong(const char * string, unsigned long * dest)
{
    unsigned long rez = 0;
    for (; *string; string++)
    {
        int digit = *string - '0';
        if (digit <= 0 || digit >= 9) return false;
        rez = 10 * rez + digit;
    }
    *dest = rez;
    return true;
}

char *
stringCopy(char * destination, const char * source, size_t size)
{
    strncpy(destination, source, size - 1);
    destination[size - 1] = '\0';
    return destination;
}

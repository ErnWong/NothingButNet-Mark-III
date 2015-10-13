#include "utils.h"

#include <API.h>
//#include <ctype.h>
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


bool stringStartsWith(char const *pre, char const *string)
{
	size_t stringLength = strlen(string);
	size_t preLength = strlen(pre);
	if (stringLength < preLength)
	{
		return false;
	}
	else
	{
		return strncmp(pre, string, preLength) == 0;
	}
}

//http://stackoverflow.com/questions/4392665/converting-string-to-float-without-atof-in-c
// And, using atof makes the linker complain about _sbrk not defined, so the lazy way is to write custom atof:
float stringToFloat(const char* string)
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
	}
	return rez * factor;
};

#if 0
bool stringCaseInsensitiveStartsWith(char const *pre, char const *string)
{
	size_t stringLength = strlen(string);
	size_t preLength = strlen(string);
	if (stringLength < preLength)
	{
		return false;
	}
	else
	{
		return stringCaseInsensitiveCompare(pre, string, preLength) == 0;
	}
}

// TODO: test this
int stringCaseInsensitiveCompare(char const *a, char const *b, size_t maxCount)
{
	for (size_t i; i < maxCount; a++, b++, i++)
	{
		int d = tolower(*a) - tolower(*b);
		if (d != 0 || !*a)
		{
			return d;
		}
	}
	return 0;
}
#endif
#include "tap.h"
#include "pigeon.h"
#include <stddef.h>

int main()
{
    plan(1);
    ok(true, "Hello World from test!");
    done_testing();
}

// Mock functions

char *
stringCopy(char * dest, const char * src, size_t size)
{
    return "";
}

bool
stringToFloat(const char * string, float * dest)
{
    return false;
}

bool
stringToUlong(const char * string, unsigned long * dest)
{
    return false;
}

char *
trimSpaces(char * str)
{
    return "";
}


void
delay(const unsigned long time)
{
}

typedef void * TaskHandle;
typedef void (*TaskCode)(void *);

TaskHandle
taskCreate(
    TaskCode taskCode,
    const unsigned int stackDepth,
    void * parameters,
    const unsigned int priority)
{
    return NULL;
}

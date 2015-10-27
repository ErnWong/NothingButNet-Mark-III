#include "tap.h"
#include "pigeon.h"
#include <stddef.h>

// forward

void test_portalFloatHandler();
void test_portalUintHandler();
void test_portalUlongHandler();
void test_portalBoolHandler();

//

int main()
{
    plan(20);

    test_portalFloatHandler();
    test_portalUintHandler();
    test_portalUlongHandler();
    test_portalBoolHandler();

    done_testing();
}

// Subtests

void
test_portalFloatHandler()
{
    // 5 tests

    float x = -123.0f;
    char res[128];
    portalFloatHandler(&x, "", res);
    bool isUnchanged = x == -123.0f;
    is(
        res,
        "-123.000000",
        "portalFloatHandler, receiving empty input, should return value"
    );
    ok(
        isUnchanged,
        "portalFloatHandler, receiving empty input, should not modify float"
    );
    if (!isUnchanged) diag("(got) %f != %f (expected)", x, -123.0f);

    res[0] = '\0';

    portalFloatHandler(&x, "-67.8", res);
    bool isCorrect = x == -67.8f;
    is(
        res,
        "",
        "portalFloatHandler, receiving valid input, should not touch res buffer"
    );
    ok(
        isCorrect,
        "portalFloatHandler, receiving valid input, should set float's new value"
    );
    if (!isCorrect) diag("(got) %f != %f (expected)", x, -67.8f);

    res[0] = '~';
    portalFloatHandler(NULL, "9", res);
    portalFloatHandler(&x, NULL, res);
    portalFloatHandler(&x, "9", NULL);
    ok(
        x == -67.8f && res[0] == '~',
        "portalFloatHandler, receiving any NULL ptr, should cope and ignore"
    );
}

void
test_portalUintHandler()
{
    // 5 tests

    unsigned int x = 123;
    char res[128];
    portalUintHandler(&x, "", res);
    bool isUnchanged = x == 123;
    is(
        res,
        "123",
        "portalUintHandler, receiving empty input, should return value"
    );
    ok(
        isUnchanged,
        "portalUintHandler, receiving empty input, should not modify uint"
    );
    if (!isUnchanged) diag("(got) %u != %u (expected)", x, 123);
    res[0] = '\0';

    portalUintHandler(&x, "45", res);
    bool isCorrect = x == 45;
    is(
        res,
        "",
        "portalUintHandler, receiving valid input, should not touch res buffer"
    );
    ok(
        isCorrect,
        "portalUintHandler, receiving valid input, should set uint's new value"
    );
    if (!isCorrect) diag("(got) %u != %u (expected)", x, 45);

    res[0] = '~';
    portalUintHandler(NULL, "89", res);
    portalUintHandler(&x, NULL, res);
    portalUintHandler(&x, "89", NULL);
    ok(
        x == 45 && res[0] == '~',
        "portalUintHandler, receiving any NULL ptr, should cope and ignore"
    );
}

void
test_portalUlongHandler()
{
    // 5 tests

    unsigned long x = 123;
    char res[128];
    portalUlongHandler(&x, "", res);
    bool isUnchanged = x == 123;
    is(
        res,
        "123",
        "portalUlongHandler, receiving empty input, should return value"
    );
    ok(
        isUnchanged,
        "portalUintHandler, receiving empty input, should not modify ulong"
    );
    if (!isUnchanged) diag("(got) %u != %u (expected)", x, 123);
    res[0] = '\0';

    portalUlongHandler(&x, "45", res);
    bool isCorrect = x == 45;
    is(
        res,
        "",
        "portalUlongHandler, receiving valid input, should not touch res buffer"
    );
    ok(
        isCorrect,
        "portalUlongHandler, receiving valid input, should set ulong's new value"
    );
    if (!isCorrect) diag("(got) %u != %u (expected)", x, 45);

    res[0] = '~';
    portalUlongHandler(NULL, "89", res);
    portalUlongHandler(&x, NULL, res);
    portalUlongHandler(&x, "89", NULL);
    ok(
        x == 45 && res[0] == '~',
        "portalUlongHandler, receiving any NULL ptr, should cope and ignore"
    );
}

void
test_portalBoolHandler()
{
    // 5 tests

    bool x = false;
    char res[128];
    portalBoolHandler(&x, "", res);
    is(
        res,
        "false",
        "portalBoolHandler, receiving empty input, should return value"
    );
    ok(
        x == false,
        "portalBoolHandler, receiving empty input, should not modify bool"
    );
    res[0] = '\0';

    portalBoolHandler(&x, "true", res);
    is(
        res,
        "",
        "portalBoolHandler, receiving valid input, should not touch res buffer"
    );
    ok(
        x == true,
        "portalBoolHandler, receiving valid input, should set bool's new value"
    );

    res[0] = '~';
    portalBoolHandler(NULL, "false", res);
    portalBoolHandler(&x, NULL, res);
    portalBoolHandler(&x, "false", NULL);
    ok(
        x == true && res[0] == '~',
        "portalBoolHandler, receiving any NULL ptr, should cope and ignore"
    );
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
    *dest = -67.8f;
    return true;
}

bool
stringToUlong(const char * string, unsigned long * dest)
{
    *dest = 45;
    return true;
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

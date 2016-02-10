#include "main.h"

#include "reckoner.h"
#include "flywheel.h"
#include "flap.h"
#include "diffsteer-control.h"

#define UNUSED(x) (void)(x)

static void updateTask(void*);

static bool isRunning = false;

void autonomous()
{
    isRunning = true;
    TaskHandle task = taskCreate(updateTask, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT);

    diffsteerMove(diffsteer, 1.0f, 1.0f);
    waitUntilDiffsteerReady(diffsteer, 60000);

    diffsteerRotate(diffsteer, 0.0f);
    waitUntilDiffsteerReady(diffsteer, 60000);

    diffsteerStop(diffsteer);

    isRunning = false;
}

static void updateTask(void * arg)
{
    UNUSED(arg);

    flywheelRun(fwBelow);
    flywheelRun(fwAbove);
    flapRun(fwFlap);

    while (isRunning)
    {
        reckonerUpdate(reckoner);
        diffsteerUpdate(diffsteer);
        delay(20);
    }
}

#include "main.h"

#include "reckoner.h"
#include "flywheel.h"
#include "flap.h"
#include "diffsteer-control.h"

#define UNUSED(x) (void)(x)

static void updateTaskcode(void*);
static void autonTaskcode(void*);

static bool isRunning = false;
static bool autonFinished = true;
static bool updateFinished = true;
static bool isStopping = false;
static TaskHandle autonTask = NULL;

void
autonomous()
{
    motorSet(8, 0);
    isRunning = true;
    TaskHandle task = taskCreate(updateTaskcode, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT);

    // diffsteerMove(diffsteer, 1.0f, 1.0f);
    // waitUntilDiffsteerReady(diffsteer, 60000);

    // diffsteerRotate(diffsteer, 0.0f);
    // waitUntilDiffsteerReady(diffsteer, 60000);

    // diffsteerStop(diffsteer);

    flywheelSet(fwAbove, 700.0f);
    flywheelSet(fwBelow, 1850.0f);
    delay(3000);

    for (int i = 0; i < 6 && isRunning; i++)
    {

        waitUntilFlywheelReady(fwAbove, -1);
        waitUntilFlywheelReady(fwBelow, -1);

        // flapDrop(fwFlap);
        // waitUntilFlapClosed(fwFlap);

        // Conveyor:
        motorSet(8, -127);
        delay(300);
        motorSet(8, 0);
        delay(300);
        motorSet(8, -127);
        delay(1000);
        motorSet(8, 0);
        delay(300);

    }

    isRunning = false;
    autonFinished = true;
}

static void
updateTaskcode(void * arg)
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
    updateFinished = true;
}

void
autonomousRun()
{
    if (autonTask != NULL) return;
    autonTask = taskCreate(autonTaskcode, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT);
    autonFinished = false;
    updateFinished = false;
}

void
autonomousStop()
{
    // Prevent double calling
    if (isStopping) return;
    isStopping = true;

    if (autonTask == NULL) return;

    if (isRunning)
    {
        // Stop tasks peacefully
        isRunning = false;
        while (!(autonFinished && updateFinished))
        {
            delay(50);
        }
    }
    else
    {
        taskDelete(autonTask);
    }
    autonTask = NULL;
    isStopping = false;
}

static void
autonTaskcode(void * arg)
{
    UNUSED(arg);
    autonomous();
}

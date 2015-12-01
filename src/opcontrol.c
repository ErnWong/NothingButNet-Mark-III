#include "main.h"

#include "buttons.h"
#include "drive.h"
#include "flywheel.h"

#define UNUSED(x) (void)(x)

static void turnOnFlywheel(void*);
static void turnOffFlywheel(void*);
static void turnOnConveyor(void*);
static void turnOffConveyor(void*);
static void openFlap(void*);
static void closeFlap(void*);
static void changeDriveStyle(void*);

void operatorControl()
{
    buttonsInit();
    buttonOndown(JOY_SLOT1, JOY_5U, turnOnConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_5D, turnOffConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_6U, openFlap, NULL);
    buttonOndown(JOY_SLOT1, JOY_6D, closeFlap, NULL);
    buttonOndown(JOY_SLOT1, JOY_7R, turnOnFlywheel, NULL);
    buttonOndown(JOY_SLOT1, JOY_7D, turnOffFlywheel, NULL);
    buttonOndown(JOY_SLOT1, JOY_7U, changeDriveStyle, NULL);

    flywheelRun(fwBelow);
    flywheelRun(fwAbove);
    flapRun(fwFlap);

    while (true)
    {
        buttonsUpdate();
        driveUpdate(drive);
        reckonerUpdate(reckoner);
        delay(20);
    }
    // Note: never exit
}

static void
turnOnFlywheel(void * handle)
{
    UNUSED(handle);
    flywheelMutexTake(fwAbove, -1);
    flywheelSet(fwAbove, 800);
    flywheelMutexGive(fwAbove);
}

static void
turnOffFlywheel(void * handle)
{
    UNUSED(handle);
    flywheelMutexTake(fwAbove, -1);
    flywheelSet(fwAbove, 0);
    flywheelMutexGive(fwAbove);
}

static void
turnOnConveyor(void * handle)
{
    UNUSED(handle);
    motorSet(8, 127);
}

static void
turnOffConveyor(void * handle)
{
    UNUSED(handle);
    motorSet(8, 0);
}

static void
openFlap(void * handle)
{
    UNUSED(handle);
    flapOpen(fwFlap);
}

static void
closeFlap(void * handle)
{
    UNUSED(handle);
    flapClose(fwFlap);
}

static void
changeDriveStyle(void * handle)
{
    UNUSED(handle);
    driveNext(drive);
}

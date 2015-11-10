#include "main.h"

#include "buttons.h"
#include "drive.h"
#include "flywheel.h"

#define UNUSED(x) (void)(x)

static void turnOnFlywheel(ButtonHandle);
static void turnOffFlywheel(ButtonHandle);

void operatorControl()
{
    getchar();
    buttonsInit();
    buttonOndown(JOY_SLOT1, JOY_5U, turnOnFlywheel, NULL);
    buttonOndown(JOY_SLOT1, JOY_5D, turnOffFlywheel, NULL);
    flywheelRun(fwAbove);
	while (true)
    {
        buttonsUpdate();
        //printf("Motor is at %d\n", motorGet(5));
        driveUpdate(drive);
		delay(20);
	}
    // Note: never exit
}

static void
turnOnFlywheel(ButtonHandle handle)
{
    UNUSED(handle);
    puts("Turning on flywheel");
    flywheelSet(fwAbove, 800);
}

static void
turnOffFlywheel(ButtonHandle handle)
{
    UNUSED(handle);
    puts("Turning off flywheel");
    flywheelSet(fwAbove, 0);
}

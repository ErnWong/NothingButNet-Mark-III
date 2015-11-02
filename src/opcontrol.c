#include "main.h"

#include "drive.h"
#include "flywheel.h"



void operatorControl()
{
    flywheelRun(fwAbove);
    flywheelRun(fwBelow);
	while (1)
    {
        driveUpdate(drive);
		delay(20);
	}
    // Note: never exit
}

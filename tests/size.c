#include <stdio.h>
#include <stddef.h>
#include "flywheel.size.h"

int main()
{
    size_t size;
    size += 2 * sizeof_Flywheel;
    size += 2 * sizeof_FlywheelSetup;
    /*
    int size = 0;
    size += 2 * sizeof(Flywheel);
    size += 2 * sizeof(FlywheelSetup);
    size += 2 * sizeof(Tbh);
    size += 1 * sizeof(Drive);
    size += 1 * sizeof(DriveSetup);
    size += 2 * sizeof(DriveControl);
    size += 1 * sizeof(Pigeon);
    size += 5 * sizeof(Portal);
    size += 60 * sizeof(PortalEntry);
    size += 5 * sizeof(PortalEntryList);
    size += 2 * sizeof(EncoderShim);
    size += 10 * sizeof(MotorShim);
    */
    printf("Rough estimate of the RAM size needed: %d bytes", size);
}

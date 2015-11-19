#include "main.h"

#include <API.h>
#include "drive.h"
#include "drive-style.h"
#include "flywheel.h"
#include "control.h"
#include "shims.h"

Pigeon * pigeon = NULL;
Drive * drive = NULL;
Flywheel * fwAbove = NULL;
Flywheel * fwBelow = NULL;
Encoder fwAboveEncoder = NULL;
Encoder fwBelowEncoder = NULL;

static char *
pigeonGets(char * buffer, int maxSize);

static void
pigeonPuts(const char * message);

void initializeIO()
{
    // Note: kernal mode, scheduler paused
    // Purpose:
    //  - Set default pin modes (pinMode)
    //  - Set port states (digitalWrite)
    //  - Configure UART (usartOpen), but not LCD (lcdInit)
}

void initialize()
{
    // Note: no joystick, no link, exit promptly
    // Purpose:
    //  - Init sensors, LCDs, Global vars, IMEs
    fwBelowEncoder = encoderInit(1, 2, false);
    fwAboveEncoder = encoderInit(3, 4, false);
    pigeon = pigeonInit(pigeonGets, pigeonPuts, millis);
    /*

    FlywheelSetup fwBelowSetup =
    {
        .id = "flywheel-below",
        .pigeon = pigeon,

        .gearing = 5.0f,
        .smoothing = 0.2f,

        .controlSetup = tbhSetup,
        .controlUpdater = tbhUpdate,
        .controlResetter = tbhReset,
        .control = tbhInit(0.2, tbhDummyEstimator),

        .encoderGetter = encoderGetter,
        .encoderResetter = encoderResetter,
        .encoder = encoderGetHandle(fwBelowEncoder),

        .motorSetters =
        {
            motorSetter,
            motorSetter,
            motorSetter
        },
        .motors =
        {
            motorGetHandle(1, false),
            motorGetHandle(2, false),
            motorGetHandle(3, false)
        },

        .priorityReady = 2,
        .priorityActive = 2,
        .frameDelayReady = 200,
        .frameDelayActive = 20,

        .thresholdError = 1.0f,
        .thresholdDerivative = 1.0f,
        .checkCycle = 20
    };
    fwBelow = flywheelInit(fwBelowSetup);
    */

    FlywheelSetup fwAboveSetup =
    {
        .id = "fwabove",
        .pigeon = pigeon,

        .gearing = 25.0f,
        .smoothing = 0.2f,

        .controlSetup = tbhSetup,
        .controlUpdater = tbhUpdate,
        .controlResetter = tbhReset,
        .control = tbhInit(0.2, tbhDummyEstimator),

        .encoderGetter = encoderGetter,
        .encoderResetter = encoderResetter,
        .encoder = encoderGetHandle(fwAboveEncoder),

        .motorSetters =
        {
            motorSetter,
            motorSetter
        },
        .motors =
        {
            motorGetHandle(5, false),
            motorGetHandle(6, true)
        },

        .priorityReady = 2,
        .priorityActive = 2,
        .frameDelayReady = 200,
        .frameDelayActive = 20,

        .thresholdError = 1.0f,
        .thresholdDerivative = 1.0f,
        .checkCycle = 20
    };
    fwAbove = flywheelInit(fwAboveSetup);

    DriveSetup driveSetup =
    {
        .motorSetters =
        {
            motorSetter,
            motorSetter
        },
        .motors =
        {
            motorGetHandle(8, false),
            motorGetHandle(7, false)
        }
    };
    drive = driveInit(driveSetup);
    driveAdd(drive, tankStyle);
    driveAdd(drive, arcadeRightStyle);

    pigeonReady(pigeon);

}

static char *
pigeonGets(char * buffer, int maxSize)
{
    return fgets(buffer, maxSize, stdout);
}

static void
pigeonPuts(const char * message)
{
    puts(message);
}

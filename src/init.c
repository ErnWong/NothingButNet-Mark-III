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
    Encoder fwAboveEncoder = encoderInit(1, 2, false);
    Encoder fwBelowEncoder = encoderInit(3, 4, false);

    FlywheelSetup fwBelowSetup =
    {
        .id = "flywheel-above",
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
    fwAbove = flywheelInit(fwBelowSetup);

    FlywheelSetup fwAboveSetup =
    {
        .id = "flywheel-above",
        .pigeon = pigeon,

        .gearing = 5.0f,
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
            motorGetHandle(4, false),
            motorGetHandle(5, false)
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
            motorGetHandle(7, false),
            motorGetHandle(8, false)
        }
    };
    drive = driveInit(driveSetup);
    driveAdd(drive, tankStyle);
    driveAdd(drive, arcadeRightStyle);
}

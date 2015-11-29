#include "main.h"

#include <API.h>
#include "drive.h"
#include "drive-style.h"
#include "flywheel.h"
#include "control.h"
#include "flap.h"
#include "reckoner.h"
#include "diffsteer-control.h"
#include "shims.h"

Pigeon * pigeon = NULL;
Drive * drive = NULL;
Flywheel * fwAbove = NULL;
Flywheel * fwBelow = NULL;
Encoder fwAboveEncoder = NULL;
Encoder fwBelowEncoder = NULL;
Flap * fwFlap = NULL;
Reckoner * reckoner = NULL;
Diffsteer * diffsteer = NULL;


static float fwAboveEstimator(float target);
static char * pigeonGets(char * buffer, int maxSize);
static void pigeonPuts(const char * message);

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

    MotorHandle motorDriveLeft = motorGetHandle(7, false);
    MotorHandle motorDriveRight = motorGetHandle(6, false);

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
        .control = tbhInit(0.2, fwAboveEstimator),

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
            motorGetHandle(5, true)
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

    FlapSetup fwFlapSetup =
    {
        .id ="flap",
        .pigeon = pigeon,

        .slew = 1.0f,
        .motorSetter = motorSetter,
        .motor = motorGetHandle(9, false),
        .digitalOpenedGetter = digitalGetter,
        .digitalOpened = digitalGetHandle(5, true),
        .digitalClosedGetter = digitalGetter,
        .digitalClosed = digitalGetHandle(6, true),

        .initialState = FLAP_CLOSING,

        .priorityReady = 2,
        .priorityActive = 2,
        .priorityDrop = 2,
        .frameDelayReady = 20,
        .frameDelayActive = 20,
        .dropDelay = 1000
    };
    fwFlap = flapInit(fwFlapSetup);

    ReckonerSetup reckonerSetup =
    {
        .id = "reckoner",
        .pigeon = pigeon,

        .initialX = 0.0f,
        .initialY = 0.0f,
        .initialHeading = 0.0f,
        .initialVelocity = 0.0f,

        .gearingLeft = 1.0f,
        .gearingRight = 1.0f,
        .radiusLeft = 5.0f,
        .radiusRight = 5.0f,
        .wheelSeparation = 16.0f,
        .smoothing = 0.5f,

        .encoderLeftGetter = imeGetter,
        .encoderLeft = imeGetHandle(0, MOTOR_TYPE_393_TORQUE),
        .encoderRightGetter = imeGetter,
        .encoderRight = imeGetHandle(1, MOTOR_TYPE_393_TORQUE)
    };
    reckoner = reckonerInit(reckonerSetup);

    DiffsteerSetup diffsteerSetup =
    {
        .id = "diffsteer",
        .pigeon = pigeon,

        .gainDistance = 1.0f,
        .gainHeading = 1.0f,

        .motorLeftSetter = motorSetter,
        .motorLeft = motorDriveLeft,
        .motorRightSetter = motorSetter,
        .motorRight = motorDriveRight
    };
    diffsteer = diffsteerInit(diffsteerSetup);

    DriveSetup driveSetup =
    {
        .motorSetters =
        {
            motorSetter,
            motorSetter
        },
        .motors =
        {
            motorDriveLeft,
            motorDriveRight
        }
    };
    drive = driveInit(driveSetup);
    driveAdd(drive, tankStyle);
    driveAdd(drive, arcadeRightStyle);

    pigeonReady(pigeon);

}

static float
fwAboveEstimator(float target)
{
    return 18.195f + 2.2052e-5f * target * target;
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

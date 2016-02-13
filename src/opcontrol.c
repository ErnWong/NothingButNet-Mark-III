#include "main.h"

#include "buttons.h"
#include "drive.h"
#include "flywheel.h"

#define UNUSED(x) (void)(x)
#define THE_DURATION_TO_CONSIDER_THE_FLAPPY_BUTTON_AS_OVERRIDE 1000

static void checkForAutonomous();
static void setFwTarget();
static void turnOnFlywheelShortRange(void*);
static void turnOnFlywheelLongRange(void*);
static void turnOffFlywheel(void*);
static void toggleUpConveyor(void*);
static void toggleDownConveyor(void*);
static void openFlapOndown(void*);
static void openFlapOnup(void*);
static void closeFlap(void*);
static void changeDriveStyle(void*);
static void increaseFwRpm(void*);
static void decreaseFwRpm(void*);
static void checkThatFlappyThing();

typedef enum
FlywheelPreset
{
    FLYWHEEL_SHORTRANGE,
    FLYWHEEL_LONGRANGE
}
FlywheelPreset;

float fwAbovePresets[2] =
{
    500.0f,
    1000.0f
};

float fwBelowPresets[2] =
{
    1500.0f,
    2000.0f
};

typedef enum
ConveyorState
{
    CONVEYOR_UP,
    CONVEYOR_DOWN,
    CONVEYOR_OFF
}
ConveyorState;

ConveyorState conveyorState = CONVEYOR_OFF;

bool isFwOn = false;
FlywheelPreset fwPresetMode = FLYWHEEL_LONGRANGE;

bool isTheFlappyButtonPressed = false;
unsigned long theTimeWhenTheFlappyButtonStartedToBeDown = 0;

void operatorControl()
{
    buttonsInit();
    buttonOndown(JOY_SLOT1, JOY_5U, toggleUpConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_5D, toggleDownConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_6U, openFlapOndown, NULL);
    buttonOnup(JOY_SLOT1, JOY_6U, openFlapOnup, NULL);
    buttonOndown(JOY_SLOT1, JOY_6D, closeFlap, NULL);
    buttonOndown(JOY_SLOT1, JOY_7U, changeDriveStyle, NULL);
    buttonOndown(JOY_SLOT1, JOY_7R, turnOnFlywheelShortRange, NULL);
    buttonOndown(JOY_SLOT1, JOY_7D, turnOffFlywheel, NULL);
    buttonOndown(JOY_SLOT1, JOY_8L, turnOnFlywheelLongRange, NULL);
    buttonOndown(JOY_SLOT1, JOY_8D, turnOffFlywheel, NULL);
    buttonOndown(JOY_SLOT1, JOY_8U, increaseFwRpm, NULL);
    buttonOndown(JOY_SLOT1, JOY_8R, decreaseFwRpm, NULL);

    flywheelRun(fwBelow);
    flywheelRun(fwAbove);
    flapRun(fwFlap);

    while (true)
    {
        checkForAutonomous();
        buttonsUpdate();
        driveUpdate(drive);
        reckonerUpdate(reckoner);
        checkThatFlappyThing();
        delay(20);
    }
    // Note: never exit
}

static void
checkForAutonomous()
{
    if (joystickGetDigital(1, 7, JOY_LEFT))
    {
        autonomousRun();

        // Poll wait until button released and pressed again
        // Note: buttonsUpdate is not called, so buttonOndown doesn't work
        while (joystickGetDigital(1, 7, JOY_LEFT))
        {
            delay(100);
        }
        while (!joystickGetDigital(1, 7, JOY_LEFT))
        {
            delay(100);
        }

        autonomousStop();
    }
}

static void
setFwTarget()
{
    float targetAbove = 0.0f;
    float targetBelow = 0.0f;
    if (isFwOn)
    {
        targetAbove = fwAbovePresets[fwPresetMode];
        targetBelow = fwBelowPresets[fwPresetMode];
    }
    flywheelSet(fwAbove, targetAbove);
    flywheelSet(fwBelow, targetBelow);
}

static void
turnOnFlywheelLongRange(void * handle)
{
    UNUSED(handle);
    isFwOn = true;
    fwPresetMode = FLYWHEEL_LONGRANGE;
    setFwTarget();
}

static void
turnOnFlywheelShortRange(void * handle)
{
    UNUSED(handle);
    isFwOn = true;
    fwPresetMode = FLYWHEEL_SHORTRANGE;
    setFwTarget();
}

static void
increaseFwRpm(void * handle)
{
    UNUSED(handle);
    fwAbovePresets[fwPresetMode] += 10.0f;
    fwBelowPresets[fwPresetMode] += 10.0f;
    setFwTarget();
}

static void
decreaseFwRpm(void * handle)
{
    UNUSED(handle);
    fwAbovePresets[fwPresetMode] -= 10.0f;
    fwBelowPresets[fwPresetMode] -= 10.0f;
    setFwTarget();
}

static void
turnOffFlywheel(void * handle)
{
    UNUSED(handle);
    isFwOn = false;
    setFwTarget();
}

static void
toggleUpConveyor(void * handle)
{
    UNUSED(handle);
    if (conveyorState == CONVEYOR_UP)
    {
        conveyorState = CONVEYOR_OFF;
        motorSet(8, 0);
    }
    else
    {
        conveyorState = CONVEYOR_UP;
        motorSet(8, 127);
    }
}

static void
toggleDownConveyor(void * handle)
{
    UNUSED(handle);
    if (conveyorState == CONVEYOR_DOWN)
    {
        conveyorState = CONVEYOR_OFF;
        motorSet(8, 0);
    }
    else
    {
        conveyorState = CONVEYOR_DOWN;
        motorSet(8, -127);
    }
}

static void
openFlapOndown(void * handle)
{
    UNUSED(handle);
    isTheFlappyButtonPressed = true;
    theTimeWhenTheFlappyButtonStartedToBeDown = millis();
}

static void
openFlapOnup(void * handle)
{
    UNUSED(handle);
    isTheFlappyButtonPressed = false;
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

static void
checkThatFlappyThing()
{
    if (!isTheFlappyButtonPressed) return;

    bool shouldOpenTheFlappyThing = millis() - theTimeWhenTheFlappyButtonStartedToBeDown > THE_DURATION_TO_CONSIDER_THE_FLAPPY_BUTTON_AS_OVERRIDE;
    shouldOpenTheFlappyThing |= flywheelIsReady(fwBelow) && flywheelIsReady(fwAbove);

    if (shouldOpenTheFlappyThing)
    {
        flapDrop(fwFlap);
        isTheFlappyButtonPressed = false;
        return;
    }
}

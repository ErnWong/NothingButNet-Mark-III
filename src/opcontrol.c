#include "main.h"

#include "buttons.h"
#include "drive.h"
#include "flywheel.h"

#define UNUSED(x) (void)(x)

static void setFwTarget();
static void turnOnFlywheelShortRange(void*);
static void turnOnFlywheelLongRange(void*);
static void turnOffFlywheel(void*);
static void toggleUpConveyor(void*);
static void toggleDownConveyor(void*);
static void openFlap(void*);
static void closeFlap(void*);
static void changeDriveStyle(void*);
static void increaseFwRpm(void*);
static void decreaseFwRpm(void*);

typedef enum
FlywheelPreset
{
    FLYWHEEL_SHORTRANGE,
    FLYWHEEL_LONGRANGE
}
FlywheelPreset;

float fwAbovePresets[2] =
{
    100.0f,
    300.0f
};

float fwBelowPresets[2] =
{
    500.0f,
    800.0f
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

void operatorControl()
{
    buttonsInit();
    buttonOndown(JOY_SLOT1, JOY_5U, toggleUpConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_5D, toggleDownConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_6U, openFlap, NULL);
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
        buttonsUpdate();
        driveUpdate(drive);
        reckonerUpdate(reckoner);
        delay(20);
    }
    // Note: never exit
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

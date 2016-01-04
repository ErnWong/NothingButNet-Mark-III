#include "main.h"

#include "buttons.h"
#include "drive.h"
#include "flywheel.h"

#define UNUSED(x) (void)(x)

static void setFwTarget();
static void turnOnFlywheelShortRange(void*);
static void turnOnFlywheelLongRange(void*);
static void turnOffFlywheel(void*);
static void turnOnConveyor(void*);
static void turnOffConveyor(void*);
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
    1.0f,
    2.0f
};

float fwBelowPresets[2] =
{
    1.0f,
    2.0f
};

bool isFwOn = false;
FlywheelPreset fwPresetMode = FLYWHEEL_LONGRANGE;

void operatorControl()
{
    buttonsInit();
    buttonOndown(JOY_SLOT1, JOY_5U, turnOnConveyor, NULL);
    buttonOndown(JOY_SLOT1, JOY_5D, turnOffConveyor, NULL);
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
    fwAbovePresets[fwPresetMode] += 1.0f;
    fwBelowPresets[fwPresetMode] += 1.0f;
    setFwTarget();
}

static void
decreaseFwRpm(void * handle)
{
    UNUSED(handle);
    fwAbovePresets[fwPresetMode] -= 1.0f;
    fwBelowPresets[fwPresetMode] -= 1.0f;
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

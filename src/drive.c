#include "drive.h"
#include "shims.h"
#include "drive-style.h"



struct DriveControl;
typedef struct DriveControl DriveControl;

struct Drive
{
    MotorSetter motorSet[4];
    MotorHandle motors[4];
    DriveControl * control;
};


struct DriveControl
{
    DriveStyle update;
    DriveControl * next;
    DriveControl * previous;
};



static void update(Drive*);



Drive *
driveInit(DriveSetup setup)
{
    Drive * drive = malloc(sizeof(Drive));
    for (int i = 0; i < 4; i++ )
    {
        drive->motorSet[i] = setup.motorSetters[i];
        drive->motors[i] = setup.motors[i];
    }
    return drive;
}

void
driveAdd(Drive * drive, DriveStyle style)
{
    DriveControl * control = malloc(sizeof(DriveControl));
    control->update = style;
    DriveControl * head = drive->control;
    DriveControl * tail = head->previous;
    control->next = head;
    control->previous = tail;
    head->previous = control;
    tail->next = control;
}

void
driveAddBatch(Drive * drive, DriveStyle * style)
{
    while (true)
    {
        if (*style == NULL) break;
        driveAdd(drive, *style);
        style++;
    }
}

void
driveNext(Drive * drive)
{
    drive->control = drive->control->next;
}

void
drivePrevious(Drive * drive)
{
    drive->control = drive->control->previous;
}

void
driveUpdate(Drive * drive)
{
    update(drive);
}

static void
update(Drive * drive)
{
    drive->control->update(drive->motors, drive->motorSet);
}

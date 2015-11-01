#ifndef DRIVE_H_
#define DRIVE_H_

#include "drive-style.h"

#ifdef __cpluscplus
extern "C" {
#endif



struct Drive;
typedef struct Drive Drive;

typedef struct
DriveSetup
{
    MotorSetter motorSetters[4];
    MotorHandle motors[4];
}
DriveSetup;

Drive *
driveInit(DriveSetup);

void
driveAdd(Drive*, DriveStyle);

void
driveNext(Drive*);

void
drivePrevious(Drive*);

void
driveUpdate(Drive*);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

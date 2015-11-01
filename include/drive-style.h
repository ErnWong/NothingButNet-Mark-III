#ifndef DRIVESTYLE_H_
#define DRIVESTYLE_H_

#include "shims.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef void (*DriveStyle)(MotorHandle *, MotorSetter *);

typedef enum
TankDriveMotors
{
    DRIVE_TANK_LEFT,
    DRIVE_TANK_RIGHT
}
TankDriveMotors;

typedef enum
XDriveMotors
{
    DRIVE_X_TOPLEFT,
    DRIVE_X_TOPRIGHT,
    DRIVE_X_BOTTOMLEFT,
    DRIVE_X_BOTTOMRIGHT
}
XDriveMotors;


void tankStyle(MotorHandle *, MotorSetter *);
void arcadeLeftStyle(MotorHandle * motors, MotorSetter * motorSet);
void arcadeRightStyle(MotorHandle * motors, MotorSetter * motorSet);



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

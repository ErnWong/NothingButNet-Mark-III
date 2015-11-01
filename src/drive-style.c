#include "drive-style.h"

#include <API.h>

void tankStyle(MotorHandle * motors, MotorSetter * motorSet)
{
    int cmdLeft = joystickGetAnalog(1, 3);
    int cmdRight = joystickGetAnalog(1, 2);
    motorSet[DRIVE_TANK_LEFT](motors[DRIVE_TANK_LEFT], cmdLeft);
    motorSet[DRIVE_TANK_RIGHT](motors[DRIVE_TANK_RIGHT], cmdRight);
}

void arcadeLeftStyle(MotorHandle * motors, MotorSetter * motorSet)
{
    int cmdLeft = joystickGetAnalog(1, 3) + joystickGetAnalog(1, 4);
    int cmdRight = joystickGetAnalog(1, 3) - joystickGetAnalog(1, 4);
    motorSet[DRIVE_TANK_LEFT](motors[DRIVE_TANK_LEFT], cmdLeft);
    motorSet[DRIVE_TANK_RIGHT](motors[DRIVE_TANK_RIGHT], cmdRight);
}

void arcadeRightStyle(MotorHandle * motors, MotorSetter * motorSet)
{
    int cmdLeft = joystickGetAnalog(1, 2) + joystickGetAnalog(1, 1);
    int cmdRight = joystickGetAnalog(1, 2) - joystickGetAnalog(1, 1);
    motorSet[DRIVE_TANK_LEFT](motors[DRIVE_TANK_LEFT], cmdLeft);
    motorSet[DRIVE_TANK_RIGHT](motors[DRIVE_TANK_RIGHT], cmdRight);
}

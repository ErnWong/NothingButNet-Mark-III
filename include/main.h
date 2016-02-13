#ifndef MAIN_H_
#define MAIN_H_

#include <API.h>

#include "drive.h"
#include "pigeon.h"
#include "flywheel.h"
#include "flap.h"
#include "reckoner.h"
#include "diffsteer-control.h"

#ifdef __cplusplus
extern "C" {
#endif



// Competition:

void autonomous();
void initializeIO();
void initialize();
void operatorControl();


// Custom Competition:

void autonomousRun();
void autonomousStop();


// Robot:

extern Pigeon * pigeon;
extern Drive * drive;
extern Flywheel * fwAbove;
extern Flywheel * fwBelow;
extern Encoder fwBelowEncoder;
extern Encoder fwAboveEncoder;
extern Flap * fwFlap;
extern Reckoner * reckoner;
extern Diffsteer * diffsteer;



// End C++ export structure
#ifdef __cplusplus
}
#endif

#endif

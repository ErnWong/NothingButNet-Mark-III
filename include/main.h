#ifndef MAIN_H_
#define MAIN_H_

#include <API.h>

#include "utils.h"
#include "flywheel.h"

#ifdef __cplusplus
extern "C" {
#endif



// Competition:

void autonomous();
void initializeIO();
void initialize();
void operatorControl();



// End C++ export structure
#ifdef __cplusplus
}
#endif

#endif

#ifndef BUTTONS_H_
#define BUTTONS_H_

#ifdef __cplusplus
extern "C" {
#endif



typedef void * ButtonHandle;
typedef void (*ButtonHandler)(ButtonHandle);

typedef enum
JoystickSlot
{
    JOYSLOT1,
    JOYSLOT2,
    JOY_NUMOFSLOTS
}
JoystickSlot;

typedef enum
JoystickButton
{
    JOY_5U,
    JOY_5D,
    JOY_6U,
    JOY_6D,
    JOY_7U,
    JOY_7D,
    JOY_7L,
    JOY_7R,
    JOY_8U,
    JOY_8D,
    JOY_8L,
    JOY_8R,
    JOY_NUMOFBUTTONS
}
JoystickButton;

void
buttonOnChange(JoystickSlot, JoystickButton, ButtonHandler, ButtonHandle);

void
buttonOnup(JoystickSlot, JoystickButton, ButtonHandler, ButtonHandle);

void
buttonOndown(JoystickSlot, JoystickButton, ButtonHandler, ButtonHandle);

void
buttonsUpdate();

void
buttonsInit();



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

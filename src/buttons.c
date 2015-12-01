#include "buttons.h"

#include <API.h>
#include <stdbool.h>
#include <stddef.h>




// Private structs {{{

struct Button;
typedef struct Button Button;


struct HandlerList;
typedef struct HandlerList HandlerList;


struct Button
{
    HandlerList * onchange;
    HandlerList * ondown;
    HandlerList * onup;
    bool state;
    unsigned char joystick;
    unsigned char group;
    unsigned char button;
};


struct HandlerList
{
    ButtonHandler handler;
    void * handle;
    HandlerList * next;
};

// }}}




// Private functions, forward declarations {{{

static void initButton(
    Button*,
    unsigned char joystick,
    unsigned char group,
    unsigned char btnNumber
);
static void updateButton(Button*);
static void callHandlers(HandlerList*);
static HandlerList * initHandlerList(ButtonHandler, void*);
static void addToHandlerList(HandlerList * item, HandlerList ** destination);

// }}}




// Global statics {{{

static Button buttons[JOY_NUMOFSLOTS][JOY_NUMOFBUTTONS] = {0};

// }}}




// Public methods {{{

void
buttonOnchange(
    JoystickSlot slot,
    JoystickButton button,
    ButtonHandler handler,
    void * handle
){
    if (slot >= JOY_NUMOFSLOTS) return;
    if (button >= JOY_NUMOFBUTTONS) return;
    HandlerList * handlerList = initHandlerList(handler, handle);
    addToHandlerList(handlerList, &buttons[slot][button].onchange);
}


void
buttonOnup(
    JoystickSlot slot,
    JoystickButton button,
    ButtonHandler handler,
    void * handle
){
    if (slot >= JOY_NUMOFSLOTS) return;
    if (button >= JOY_NUMOFBUTTONS) return;
    HandlerList * handlerList = initHandlerList(handler, handle);
    addToHandlerList(handlerList, &buttons[slot][button].onup);
}


void
buttonOndown(
    JoystickSlot slot,
    JoystickButton button,
    ButtonHandler handler,
    void * handle
){
    if (slot >= JOY_NUMOFSLOTS) return;
    if (button >= JOY_NUMOFBUTTONS) return;
    HandlerList * handlerList = initHandlerList(handler, handle);
    addToHandlerList(handlerList, &buttons[slot][button].ondown);
}


void
buttonsUpdate()
{
    for (JoystickSlot slot = 0; slot < JOY_NUMOFSLOTS; slot++)
    {
        for (JoystickButton btn = 0; btn < JOY_NUMOFBUTTONS; btn++)
        {
            updateButton(&buttons[slot][btn]);
        }
    }
}


void
buttonsInit()
{
    for (unsigned char slot = 0; slot < JOY_NUMOFSLOTS; slot++)
    {
        unsigned char joystick = slot + 1;
        initButton(&buttons[slot][JOY_5U], joystick, 5, JOY_UP);
        initButton(&buttons[slot][JOY_5D], joystick, 5, JOY_DOWN);
        initButton(&buttons[slot][JOY_6U], joystick, 6, JOY_UP);
        initButton(&buttons[slot][JOY_6D], joystick, 6, JOY_DOWN);
        initButton(&buttons[slot][JOY_7U], joystick, 7, JOY_UP);
        initButton(&buttons[slot][JOY_7D], joystick, 7, JOY_DOWN);
        initButton(&buttons[slot][JOY_7L], joystick, 7, JOY_LEFT);
        initButton(&buttons[slot][JOY_7R], joystick, 7, JOY_RIGHT);
        initButton(&buttons[slot][JOY_8U], joystick, 8, JOY_UP);
        initButton(&buttons[slot][JOY_8D], joystick, 8, JOY_DOWN);
        initButton(&buttons[slot][JOY_8L], joystick, 8, JOY_LEFT);
        initButton(&buttons[slot][JOY_8R], joystick, 8, JOY_RIGHT);
    }
}

// }}}




// Private functions {{{

static void
initButton(Button * button, unsigned char joystick, unsigned char group, unsigned char btnNumber)
{
    button->joystick = joystick;
    button->group = group;
    button->button = btnNumber;
    button->state = false;
    button->onchange = NULL;
    button->ondown = NULL;
    button->onup = NULL;
}


static void
updateButton(Button * button)
{
    bool state = joystickGetDigital(
        button->joystick,
        button->group,
        button->button
    );
    if (state != button->state)
    {
        button->state = state;
        callHandlers(button->onchange);
        if (state) callHandlers(button->ondown);
        if (!state) callHandlers(button->onup);
    }
}


static void
callHandlers(HandlerList * list)
{
    while (list != NULL)
    {
        list->handler(list->handle);
        list = list->next;
    }
}


static HandlerList *
initHandlerList(ButtonHandler handler, void * handle)
{
    HandlerList * handlerList = malloc(sizeof(HandlerList));
    handlerList->handler = handler;
    handlerList->handle = handle;
    handlerList->next = NULL;
    return handlerList;
}


static void
addToHandlerList(HandlerList * item, HandlerList ** destination)
{
    while (*destination != NULL)
    {
        destination = &(*destination)->next;
    }
    *destination = item;
}

// }}}

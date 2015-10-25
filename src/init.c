#include "main.h"

void initializeIO()
{
    // Note: kernal mode, scheduler paused
    // Purpose:
    //  - Set default pin modes (pinMode)
    //  - Set port states (digitalWrite)
    //  - Configure UART (usartOpen), but not LCD (lcdInit)
}

void initialize()
{
    // Note: no joystick, no link, exit promptly
    // Purpose:
    //  - Init sensors, LCDs, Global vars, IMEs
}

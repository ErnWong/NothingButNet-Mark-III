#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



// {{{ Definitions

#define PIGEON_KEYSIZE 9
#define PIGEON_MESSAGESIZE 81
    
// }}}



// {{{ Typedefs

typedef void (*PortalKeyHandler)(char message[PIGEON_MESSAGESIZE]);

typedef char * (*PigeonIn)(char * buffer, int maxSize); // getline
typedef void (*PigeonOut)(const char * message); // puts
typedef unsigned long (*PigeonMillis)(); // millis

struct Pigeon;
typedef struct Pigeon Pigeon;

struct Portal;
typedef struct Portal Portal;

typedef struct
PortalKeySetup
{
    char key[PIGEON_KEYSIZE];
    PortalKeyHandler handler;
    void * target;
    bool stream;
    bool onchange;
}
PortalKeySetup;

// }}}



// {{{ Methods

Portal *portalInit(char id[PIGEON_KEYSIZE]);
//void portalAdd(Portal, char key[PIGEON_KEYSIZE], PortalKeyHandler, PortalLoudness);
void portalAdd(Portal*, PortalKeySetup); 
void portalSet(Portal*, char key[PIGEON_KEYSIZE], char message[PIGEON_MESSAGESIZE]);
void portalReady(Portal*);

Pigeon *pigeonInit(PigeonIn, PigeonOut, PigeonMillis);
Portal *pigeonCreatePortal(Pigeon*, const char id[PIGEON_KEYSIZE]);

// }}}



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

#ifndef PIGEON_H_
#define PIGEON_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



#define PIGEON_ALIGNSIZE 4
#define PIGEON_LINESIZE 128



// Typedefs {{{

typedef void
(*PortalEntryHandler)(void * target, char * message, char * response);

typedef char *
(*PigeonIn)(char * buffer, int maxSize); // getline

typedef void
(*PigeonOut)(const char * message); // puts

typedef unsigned long
(*PigeonMillis)(); // millis

struct Pigeon;
typedef struct Pigeon Pigeon;

struct Portal;
typedef struct Portal Portal;

typedef struct
PortalEntrySetup
{
    char * key;
    PortalEntryHandler handler;
    void * handle;
    bool stream;
    bool onchange;
}
PortalEntrySetup;

// }}}



// Methods {{{

void
portalAdd(Portal *, PortalEntrySetup);

void
portalSet(
    Portal *,
    const char * key,
    const char * message
);

void
portalReady(Portal *);

Pigeon *
pigeonInit(PigeonIn, PigeonOut, PigeonMillis);

Portal *
pigeonCreatePortal(Pigeon *, const char * id);

void
portalFloatHandler(void * handle, char * message, char * response);

void
portalUintHandler(void * handle, char * message, char * response);

void
portalUlongHandler(void * handle, char * message, char * response);

void
portalBoolHandler(void * handle, char * message, char * response);

// }}}



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

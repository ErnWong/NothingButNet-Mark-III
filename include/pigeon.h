#ifndef PIGEON_H_
#define PIGEON_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



#define PIGEON_ALIGNSIZE 4
#define PIGEON_LINESIZE 80



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
    bool manual;
}
PortalEntrySetup;

// }}}



// Methods {{{

void
portalAdd(Portal*, PortalEntrySetup);

void
portalAddBatch(Portal*, PortalEntrySetup*);

void
portalSet(
    Portal*,
    const char * key,
    const char * message
);

void
portalGetStreamKeys(Portal*, char * destination);

bool
portalSetStreamKeys(Portal*, char * sequence);

void
portalUpdate(Portal*, const char * key);

void
portalFlush(Portal*);

void
portalReady(Portal*);

void
portalEnable(Portal*);

void
portalDisable(Portal*);

Pigeon *
pigeonInit(PigeonIn, PigeonOut, PigeonMillis);

Portal *
pigeonCreatePortal(Pigeon*, const char * id);

void
pigeonReady(Pigeon*);

void
portalFloatHandler(void * handle, char * message, char * response);

void
portalUintHandler(void * handle, char * message, char * response);

void
portalIntHandler(void * handle, char * message, char * response);

void
portalUlongHandler(void * handle, char * message, char * response);

void
portalBoolHandler(void * handle, char * message, char * response);

void
portalStreamKeyHandler(void * handle, char * message, char * response);

// }}}



// End C++ export structure
#ifdef __cplusplus
}
#endif

// End include guard
#endif

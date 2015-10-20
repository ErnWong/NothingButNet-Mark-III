#include "pigeon.h"

#include <API.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>


// {{{ Private structs/typedefs - foward Declarations

struct PortalEntry;
typedef struct PortalEntry PortalEntry;

// }}}



// {{{ Structs

struct PortalEntry
{
    char key[PIGEON_KEYSIZE];
    char message[PIGEON_MESSAGESIZE];
    PortalKeyHandler handler;
    void * target;
    bool stream;
    bool onchange;

    // binary search tree links:
    PortalEntry * entryRight;
    PortalEntry * entryLeft;
};


struct Portal
{
    Pigeon * pigeon;
    bool ready;

    char id[PIGEON_KEYSIZE];
    PortalEntry * topEntry;
    bool enabled;
    bool stream;
    bool onchange;

    // binary search tree links:
    Portal * portalRight;
    Portal * portalLeft;
};


struct Pigeon
{
    Portal * topPortal;
    PigeonIn gets;
    PigeonOut puts;
    PigeonMillis millis;
};

// }}}



// {{{ Private functions - forward declarations

static void checkReady(Pigeon*);
static bool isPortalBranchReady(Portal *);
static void writeMessage(Pigeon*, const char id[PIGEON_KEYSIZE], const char key[PIGEON_KEYSIZE], const char message[PIGEON_MESSAGESIZE]);
static PortalEntry ** findEntry(const char key[PIGEON_KEYSIZE], PortalEntry**);
static Portal ** findPortal(const char id[PIGEON_KEYSIZE], Portal**);

// }}}



// {{{ Public pigeon methods

Pigeon *
pigeonInit(PigeonIn getter, PigeonOut putter, PigeonMillis clock)
{
    Pigeon * pigeon = malloc(sizeof(Pigeon));

    pigeon->gets = getter;
    pigeon->puts = putter;
    pigeon->millis = clock;

    pigeon->topPortal = NULL;

    return pigeon;
}


Portal *
pigeonCreatePortal(Pigeon * pigeon, const char id[PIGEON_KEYSIZE])
{
    Portal * portal = malloc(sizeof(Portal));

    portal->pigeon = pigeon;
    portal->ready = false;
    strncpy(portal->id, id, PIGEON_KEYSIZE);

    portal->enabled = false;
    portal->stream = true;
    portal->onchange = true;

    portal->topEntry = NULL;
    portal->portalLeft = NULL;
    portal->portalRight = NULL;

    Portal ** location = findPortal(portal->id, &pigeon->topPortal);
    *location = portal;

    return portal;
}

// }}}



// {{{ Public portal methods

void
portalAdd(Portal * portal, PortalKeySetup setup)
{
    if (portal->ready) return;

    PortalEntry *entry = malloc(sizeof(PortalEntry));

    // strncpy size - 1 for null terminated
    strncpy(entry->key, setup.key, PIGEON_KEYSIZE - 1);
    entry->key[PIGEON_KEYSIZE - 1] = '\0';

    memset(entry->message, 0, PIGEON_KEYSIZE);

    entry->handler = setup.handler;
    entry->target = setup.target;

    entry->stream = setup.stream;
    entry->onchange = setup.onchange;

    entry->entryLeft = NULL;
    entry->entryRight = NULL;

    PortalEntry ** entryLocation = findEntry(entry->key, &portal->topEntry);
    *entryLocation = entry;
}


void
portalSet(Portal * portal, char key[PIGEON_KEYSIZE], char message[PIGEON_MESSAGESIZE])
{
    if (!portal->enabled) return;

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL) return;

    PortalEntry *entry = *location;
    strncpy(entry->message, message, PIGEON_MESSAGESIZE - 1);
    entry->message[PIGEON_MESSAGESIZE - 1] = '\0';

    if (portal->onchange && entry->onchange)
    {
        writeMessage(portal->pigeon, portal->id, entry->key, entry->message);
    }
}


void
portalReady(Portal * portal)
{
    portal->ready = true;
    checkReady(portal->pigeon);
}

// }}}



// {{{ Private methods

static void
checkReady(Pigeon * pigeon)
{
    if (isPortalBranchReady(pigeon->topPortal))
    {
        // Start task here, a task that loops and pigeon->gets(),
        // and passes incomming messages to appropriate handlers
    }
}

static bool
isPortalBranchReady(Portal * portal)
{
    if (!portal->ready) return false;
    if (portal->portalLeft && !isPortalBranchReady(portal->portalLeft)) return false;
    if (portal->portalRight && !isPortalBranchReady(portal->portalRight)) return false;
    return true;
}

static void
writeMessage(Pigeon *pigeon, const char id[PIGEON_KEYSIZE], const char key[PIGEON_KEYSIZE], const char message[PIGEON_MESSAGESIZE])
{
    // ensure null terminated
    char safeId[PIGEON_KEYSIZE];
    strncpy(safeId, id, PIGEON_KEYSIZE);
    safeId[PIGEON_KEYSIZE - 1] = '\0';

    char safeKey[PIGEON_KEYSIZE];
    strncpy(safeKey, key, PIGEON_KEYSIZE);
    safeKey[PIGEON_KEYSIZE - 1] = '\0';

    char safeMessage[PIGEON_MESSAGESIZE];
    strncpy(safeMessage, message, PIGEON_MESSAGESIZE);
    safeMessage[PIGEON_MESSAGESIZE - 1] = '\0';

    char path[18];
    if (key[0] == '\0')
    {
        snprintf(path, 18, "%s", id);
    }
    else
    {
        snprintf(path, 18, "%s.%s", id, key);
    }

    char str[110];
    snprintf(str, 110, "[%08u|%-17s] %s", (unsigned int)pigeon->millis(), path, message);
    pigeon->puts(str);
}

static Portal **
findPortal(const char id[PIGEON_KEYSIZE], Portal ** topPortal)
{
    Portal ** visiting = topPortal;
    while (*visiting != NULL)
    {
        int comparison = strncmp(id, (*visiting)->id, PIGEON_KEYSIZE);
        if (comparison == 0)
        {
            // overwrite
            *visiting = NULL;
        }
        else if (comparison < 0)
        {
            visiting = &(*visiting)->portalLeft;
        }
        else
        {
            visiting = &(*visiting)->portalRight;
        }
    }
    return visiting;
}

static PortalEntry **
findEntry(const char key[PIGEON_KEYSIZE], PortalEntry ** topEntry)
{
    PortalEntry ** visiting = topEntry;
    while (*visiting != NULL)
    {
        int comparison = strncmp(key, (*visiting)->key, PIGEON_KEYSIZE);
        if (comparison == 0)
        {
            // overwrite
            *visiting = NULL;
        }
        else if (comparison < 0)
        {
            visiting = &(*visiting)->entryLeft;
        }
        else
        {
            visiting = &(*visiting)->entryRight;
        }
    }
    return visiting;
}

// }}}



#if 0

// {{{ Disabled playground code

//pigeonCreate

//Example

#include <API.h>

char *
pigeonIn(char * str, int maxSize)
{
    return fgets(str, maxSize, stdin);
}

void
pigeonOut(const char * message)
{
    puts(message);
}

typedef struct
Setup
{
    int x;
    int y;
    int z;
}
Setup;

int
uglyBeauty(Setup setup)
{
    return setup.x + setup.y - setup.z;
}

void
targetHandler(char message[PIGEON_MESSAGESIZE])
{
    (void)(message);
}


void
example()
{
    Pigeon * pigeon = pigeonInit(pigeonIn, pigeonOut);

    Portal * portal = pigeonCreatePortal(pigeon, "Flywhe~1");
    //portalAdd(portal, 'target', targetHandler);

    void * someObjPointer = pigeonInit(pigeonIn, pigeonOut); // would point to some data that would identify and help the handler. Dummy in this case.

    PortalKeySetup targetSetup =
    {
        .key = "target",
        .handler = targetHandler,
        .target = someObjPointer,
        .stream = true,
        .onchange = true
    };
    portalAdd(portal, targetSetup);

}

// }}}

#endif

#include "pigeon.h"

#include <API.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "utils.h"


// Private convenient definitions for clarity {{{

#define KEYSIZE PIGEON_KEYSIZE
#define MSGSIZE PIGEON_MESSAGESIZE
#define INPUTSIZE PIGEON_INPUTSIZE
#define LINESIZE PIGEON_LINESIZE

// }}}



// Private structs/typedefs - foward Declarations {{{

struct PortalEntry;
typedef struct PortalEntry PortalEntry;

// }}}



// Structs {{{

struct PortalEntry
{
    char key[KEYSIZE];
    char message[MSGSIZE];
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

    char id[KEYSIZE];
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
    TaskHandle task;
};

// }}}



// Private functions - forward declarations {{{

static void checkReady(Pigeon *);
static bool isPortalBranchReady(Portal *);
static void writeMessage(
    Pigeon *,
    const char id[KEYSIZE],
    const char key[KEYSIZE],
    const char message[MSGSIZE]
);
static PortalEntry ** findEntry(const char key[KEYSIZE], PortalEntry **);
static Portal ** findPortal(const char id[KEYSIZE], Portal **);

// }}}



// Public pigeon methods {{{

Pigeon *
pigeonInit(PigeonIn getter, PigeonOut putter, PigeonMillis clock)
{
    Pigeon * pigeon = malloc(sizeof(Pigeon));

    pigeon->gets = getter;
    pigeon->puts = putter;
    pigeon->millis = clock;

    pigeon->task = NULL;
    pigeon->topPortal = NULL;

    return pigeon;
}


Portal *
pigeonCreatePortal(Pigeon * pigeon, const char id[KEYSIZE])
{
    Portal * portal = malloc(sizeof(Portal));

    portal->pigeon = pigeon;
    portal->ready = false;
    strncpy(portal->id, id, KEYSIZE);

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



// Public portal methods {{{

void
portalAdd(Portal * portal, PortalKeySetup setup)
{
    if (portal->ready) return;

    PortalEntry * entry = malloc(sizeof(PortalEntry));

    // strncpy size - 1 for null terminated
    strncpy(entry->key, setup.key, KEYSIZE - 1);
    entry->key[KEYSIZE - 1] = '\0';

    memset(entry->message, 0, KEYSIZE);

    entry->handler = setup.handler;
    entry->target = setup.target;

    entry->stream = setup.stream;
    entry->onchange = setup.onchange;

    entry->entryLeft = NULL;
    entry->entryRight = NULL;

    PortalEntry ** entryPos = findEntry(entry->key, &portal->topEntry);
    *entryPos = entry;
}


void
portalSet(Portal * portal, char key[KEYSIZE], char message[MSGSIZE])
{
    if (!portal->enabled) return;

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL) return;

    PortalEntry * entry = *location;
    strncpy(entry->message, message, MSGSIZE - 1);
    entry->message[MSGSIZE - 1] = '\0';

    if (portal->onchange && entry->onchange)
    {
        writeMessage(
            portal->pigeon,
            portal->id,
            entry->key,
            entry->message
        );
    }
}


void
portalReady(Portal * portal)
{
    portal->ready = true;
    checkReady(portal->pigeon);
}

// }}}



// Private methods {{{

static void
task(void * pigeonData)
{
    Pigeon * pigeon = pigeonData;
    while (true)
    {
        char input[INPUTSIZE];
        fgets(input, INPUTSIZE, stdin);

        trimSpaces(input);

        char * path = strtok(input, " ");
        char * message = strtok(NULL, "");

        char * portalId = strtok(path, ".");
        char * entryKey = strtok(NULL, ".");

        trimSpaces(portalId);
        trimSpaces(entryKey);

        Portal ** portalPos = findPortal(portalId, &pigeon->topPortal);
        if (*portalPos == NULL) return;
        Portal * portal = *portalPos;

        PortalEntry ** entryPos = findEntry(entryKey, &portal->topEntry);
        if (*entryPos == NULL) return;
        PortalEntry * entry = *entryPos;

        entry->handler(entry->target, message);

        delay(40);
    }
}

static void
checkReady(Pigeon * pigeon)
{
    if (isPortalBranchReady(pigeon->topPortal))
    {
        pigeon->task = taskCreate(
            task,
            TASK_DEFAULT_STACK_SIZE,
            pigeon,
            TASK_PRIORITY_DEFAULT
        );
    }
}

static bool
isPortalBranchReady(Portal * portal)
{
    if (!portal->ready)
        return false;
    if (portal->portalLeft && !isPortalBranchReady(portal->portalLeft))
        return false;
    if (portal->portalRight && !isPortalBranchReady(portal->portalRight))
        return false;
    return true;
}

static void
writeMessage(
    Pigeon * pigeon,
    const char id[KEYSIZE],
    const char key[KEYSIZE],
    const char message[MSGSIZE]
){
    // ensure null terminated
    char safeId[KEYSIZE];
    strncpy(safeId, id, KEYSIZE);
    safeId[KEYSIZE - 1] = '\0';

    char safeKey[KEYSIZE];
    strncpy(safeKey, key, KEYSIZE);
    safeKey[KEYSIZE - 1] = '\0';

    char safeMessage[MSGSIZE];
    strncpy(safeMessage, message, MSGSIZE);
    safeMessage[MSGSIZE - 1] = '\0';

    char path[18];
    if (key[0] == '\0')
    {
        snprintf(path, 18, "%s", id);
    }
    else
    {
        snprintf(path, 18, "%s.%s", id, key);
    }

    char str[LINESIZE];
    snprintf(
        str,
        LINESIZE,
        "[%08u|%-17s] %s",
        (unsigned int)pigeon->millis(),
        path,
        message
    );
    pigeon->puts(str);
}

static Portal **
findPortal(const char id[KEYSIZE], Portal ** topPortal)
{
    Portal ** visiting = topPortal;
    while (*visiting != NULL)
    {
        int comparison = strncmp(id, (*visiting)->id, KEYSIZE);
        if (comparison == 0)
        {
            return visiting;
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
findEntry(const char key[KEYSIZE], PortalEntry ** topEntry)
{
    PortalEntry ** visiting = topEntry;
    while (*visiting != NULL)
    {
        int comparison = strncmp(key, (*visiting)->key, KEYSIZE);
        if (comparison == 0)
        {
            return visiting;
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

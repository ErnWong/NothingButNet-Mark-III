#include "pigeon.h"

#include <API.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "utils.h"


// Private, for clarity

#define LINESIZE PIGEON_LINESIZE
#define ALIGNSIZE PIGEON_ALIGNSIZE
#define UNUSED(x) (void)(x)



// Private structs/typedefs - foward Declarations

struct PortalEntry;
typedef struct PortalEntry PortalEntry;



// Structs {{{

struct PortalEntry
{
    const char * key;
    char message[LINESIZE];
    PortalEntryHandler handler;
    void * handle;
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

    const char * id;
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
    const char * id,
    const char * key,
    const char * message
);
static PortalEntry ** findEntry(const char * key, PortalEntry **);
static Portal ** findPortal(const char * id, Portal **);

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
pigeonCreatePortal(Pigeon * pigeon, const char * id)
{
    Portal * portal = malloc(sizeof(Portal));

    portal->pigeon = pigeon;
    portal->ready = false;
    portal->id = id;

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
portalAdd(Portal * portal, PortalEntrySetup setup)
{
    if (portal->ready) return;

    PortalEntry * entry = malloc(sizeof(PortalEntry));

    entry->key = setup.key;
    entry->message[0] = '\0';

    entry->handler = setup.handler;
    entry->handle = setup.handle;

    entry->stream = setup.stream;
    entry->onchange = setup.onchange;

    entry->entryLeft = NULL;
    entry->entryRight = NULL;

    PortalEntry ** entryPos = findEntry(entry->key, &portal->topEntry);
    *entryPos = entry;
}


void
portalAddBatch(Portal * portal, PortalEntrySetup * setup)
{
    while (true)
    {
        if (setup->key[0] == '~' && setup->handler == NULL)
        {
            break;
        }
        portalAdd(portal, *setup);
        setup++;
    }
}


void
portalSet(Portal * portal, const char * key, const char * message)
{
    if (!portal->enabled) return;

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL) return;

    PortalEntry * entry = *location;
    stringCopy(entry->message, message, LINESIZE);

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


void
portalFloatHandler(void * handle, char * msg, char * res)
{
    float * var = handle;
    if (msg[0] == '\0') sprintf(res, "%f", *var);
    else
    {
        bool success = stringToFloat(msg, var);
        UNUSED(success);
        // TODO: warn if not successful?
    }
}


void
portalUintHandler(void * handle, char * msg, char * res)
{
    unsigned int * var = handle;
    if (msg[0] == '\0') sprintf(res, "%u", *var);
    else
    {
        unsigned long cast;
        bool success = stringToUlong(msg, &cast);
        *var = cast;
        UNUSED(success);
        // TODO: warn if not successful?
    }
}


void
portalUlongHandler(void * handle, char * msg, char * res)
{
    unsigned long * var = handle;
    if (msg[0] == '\0') sprintf(res, "%lu", *var);
    else
    {
        bool success = stringToUlong(msg, var);
        UNUSED(success);
        // TODO: warn if not successful?
    }
}


void
portalBoolHandler(void * handle, char * msg, char * res)
{
    bool * var = handle;
    if (msg[0] == '\0') strcpy(res, *var ? "true" : "false");
    else if (strcmp(msg, "true") == 0)
    {
        *var = true;
    }
    else if (strcmp(msg, "false") == 0)
    {
        *var = false;
    }
}

// }}}



// Private methods {{{

static void
task(void * pigeonData)
{
    Pigeon * pigeon = pigeonData;
    while (true)
    {
        char input[LINESIZE];
        fgets(input, LINESIZE, stdin);

        trimSpaces(input);

        char * path = strtok(input, " ");
        char * message = strtok(NULL, "");

        char * portalId = strtok(path, ".");
        char * entryKey = strtok(NULL, ".");

        trimSpaces(portalId);
        trimSpaces(entryKey);

        Portal ** portalPos = findPortal(portalId, &pigeon->topPortal);
        if (*portalPos == NULL) break;
        Portal * portal = *portalPos;

        PortalEntry ** entryPos = findEntry(entryKey, &portal->topEntry);
        if (*entryPos == NULL) break;
        PortalEntry * entry = *entryPos;

        if (entry->handler == NULL) break;
        char response[LINESIZE];
        entry->handler(entry->handle, message, response);

        if (response[0] == '\0') break;

        writeMessage(
            pigeon,
            portal->id,
            entry->key,
            response
        );

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
    const char * id,
    const char * key,
    const char * message
){

    char path[LINESIZE];
    int pathWidth;
    if (key[0] == '\0')
    {
        snprintf(path, LINESIZE, "%s", id);
        pathWidth = strlen(id);
    }
    else
    {
        snprintf(path, LINESIZE, "%s.%s", id, key);
        pathWidth = strlen(id) + strlen(key) + 1;
    }

    // round up
    pathWidth += ALIGNSIZE - 1;
    pathWidth = (pathWidth / ALIGNSIZE) * ALIGNSIZE;

    char str[LINESIZE];
    snprintf(
        str,
        LINESIZE,
        "[%08u|%-*s] %s",
        (unsigned int)pigeon->millis(),
        pathWidth,
        path,
        message
    );
    pigeon->puts(str);
}

static Portal **
findPortal(const char * id, Portal ** topPortal)
{
    Portal ** visiting = topPortal;
    while (*visiting != NULL)
    {
        int comparison = strcmp(id, (*visiting)->id);
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
findEntry(const char * key, PortalEntry ** topEntry)
{
    PortalEntry ** visiting = topEntry;
    while (*visiting != NULL)
    {
        int comparison = strcmp(key, (*visiting)->key);
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

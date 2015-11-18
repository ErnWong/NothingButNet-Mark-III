#include "pigeon.h"

#include <API.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "utils.h"


// Private, for clarity

#define MAXMSG PIGEON_MAXMESSAGES
#define LINESIZE PIGEON_LINESIZE
#define ALIGNSIZE PIGEON_ALIGNSIZE
#define UNUSED(x) (void)(x)



// Private structs/typedefs - foward Declarations

struct PortalEntry;
typedef struct PortalEntry PortalEntry;

struct PortalEntryList;
typedef struct PortalEntryList PortalEntryList;



// Structs {{{

struct PortalEntry
{
    const char * key;
    char * message;
    PortalEntryHandler handler;
    void * handle;
    bool stream;
    bool onchange;
    bool manual;

    // binary search tree links:
    PortalEntry * entryRight;
    PortalEntry * entryLeft;
};


struct PortalEntryList
{
    PortalEntry * entry;
    PortalEntryList * next;
};


struct Portal
{
    Pigeon * pigeon;
    bool ready;

    const char * id;
    PortalEntry * topEntry;
    PortalEntryList * entryList;
    PortalEntryList * streamList;

    bool enabled;
    bool stream;
    bool onchange;

    // binary search tree links:
    Portal * portalRight;
    Portal * portalLeft;
};


struct Pigeon
{
    char messages[MAXMSG][LINESIZE];
    PortalEntry * messageUsers[MAXMSG];
    int vacantIndex;
    Portal * topPortal;
    Portal * pigeonPortal;
    PigeonIn gets;
    PigeonOut puts;
    PigeonMillis millis;
    TaskHandle task;
    bool ready;
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
static void deleteEntryList(PortalEntryList *);
static void allocateMessage(Pigeon*, PortalEntry*);
static void setupPigeonPortal(Pigeon*);
static void enablePortalHandler(void * handle, char * message, char * response);
static void disablePortalHandler(void * handle, char * message, char * response);

// }}}



// Public pigeon methods {{{

Pigeon *
pigeonInit(PigeonIn getter, PigeonOut putter, PigeonMillis clock)
{
    Pigeon * pigeon = malloc(sizeof(Pigeon));

    memset(&pigeon->messages, 0, sizeof(pigeon->messages));
    memset(&pigeon->messageUsers, 0, sizeof(pigeon->messageUsers));
    pigeon->vacantIndex = 0;

    pigeon->gets = getter;
    pigeon->puts = putter;
    pigeon->millis = clock;

    pigeon->task = NULL;
    pigeon->topPortal = NULL;
    pigeon->pigeonPortal = NULL;

    pigeon->ready = false;

    setupPigeonPortal(pigeon);

    return pigeon;
}


Portal *
pigeonCreatePortal(Pigeon * pigeon, const char * id)
{
    if (pigeon == NULL) return NULL;

    Portal * portal = malloc(sizeof(Portal));

    portal->pigeon = pigeon;
    portal->ready = false;
    portal->id = id;

    portal->enabled = false;
    portal->stream = true;
    portal->onchange = true;

    portal->topEntry = NULL;
    portal->entryList = NULL;
    portal->streamList = NULL;
    portal->portalLeft = NULL;
    portal->portalRight = NULL;

    Portal ** location = findPortal(portal->id, &pigeon->topPortal);
    *location = portal;

    return portal;
}

void
pigeonReady(Pigeon * pigeon)
{
    if (pigeon == NULL) return;
    pigeon->ready = true;
    checkReady(pigeon);
}

// }}}



// Public portal methods {{{

void
portalAdd(Portal * portal, PortalEntrySetup setup)
{
    if (portal == NULL) return;
    if (portal->ready) return;

    PortalEntry * entry = malloc(sizeof(PortalEntry));

    entry->key = setup.key;
    entry->message = NULL;

    entry->handler = setup.handler;
    entry->handle = setup.handle;

    entry->stream = setup.stream;
    entry->onchange = setup.onchange;
    entry->manual = setup.manual;

    entry->entryLeft = NULL;
    entry->entryRight = NULL;

    PortalEntry ** entryPos = findEntry(entry->key, &portal->topEntry);
    *entryPos = entry;

    PortalEntryList * entryList = malloc(sizeof(PortalEntryList));
    entryList->entry = entry;
    entryList->next = portal->entryList;
    portal->entryList = entryList;

    if (entry->stream)
    {
        PortalEntryList * streamList = malloc(sizeof(PortalEntryList));
        streamList->entry = entry;
        streamList->next = portal->streamList;
        portal->streamList = streamList;
    }
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
    if (portal == NULL) return;
    if (!portal->enabled) return;

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL) return;

    PortalEntry * entry = *location;
    if (entry->message == NULL) return;

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
portalUpdate(Portal * portal, const char * key)
{
    if (portal == NULL) return;
    if (!portal->enabled) return;

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL) return;

    PortalEntry * entry = *location;

    if (entry->message == NULL) return;

    entry->handler(entry->handle, NULL, entry->message);

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
portalFlush(Portal * portal)
{
    if (portal == NULL) return;
    if (!portal->enabled) return;

    PortalEntryList * list = portal->streamList;
    char output[LINESIZE] = {0};
    if (list == NULL)
    {
        writeMessage(portal->pigeon, portal->id, "", "");
    }
    while (true)
    {
        stringAppend(output, list->entry->message, LINESIZE);
        list = list->next;
        if (list == NULL) break;
        stringAppend(output, " ", LINESIZE);
    }
    writeMessage(portal->pigeon, portal->id, "", output);
}


void
portalReady(Portal * portal)
{
    if (portal == NULL) return;
    portal->ready = true;
    checkReady(portal->pigeon);
}


void
portalEnable(Portal * portal)
{
    if (portal == NULL) return;
    portal->enabled = true;
    PortalEntryList * entryList = portal->entryList;
    while (entryList != NULL)
    {
        allocateMessage(portal->pigeon, entryList->entry);
        entryList = entryList->next;
    }
}


void
portalDisable(Portal * portal)
{
    if (portal == NULL) return;
    portal->enabled = false;
}


void
portalGetStreamKeys(Portal * portal, char * destination)
{
    if (portal == NULL) return;
    PortalEntryList * list = portal->streamList;

    destination[0] = '\0';
    if (list == NULL) return;
    while (true)
    {
        stringAppend(destination, list->entry->key, LINESIZE);
        list = list->next;
        if (list == NULL) break;
        stringAppend(destination, " ", LINESIZE);
    }
}


bool
portalSetStreamKeys(Portal * portal, char * sequence)
{
    if (portal == NULL) return false;
    PortalEntryList * list = malloc(sizeof(PortalEntryList));
    PortalEntryList * newRoot = list;

    // Try to create the list
    char * key = strtok(sequence, " ");
    while (true)
    {
        PortalEntry ** entryPtr = findEntry(key, &portal->topEntry);

        if (entryPtr == NULL)
        {
            // Fail, stop, cleanup.
            deleteEntryList(newRoot);
            return false;
        }
        list->entry = *entryPtr;

        key = strtok(NULL, " ");
        if (key == NULL) break;

        list->next = malloc(sizeof(PortalEntryList));
        list = list->next;
    }

    deleteEntryList(portal->streamList);
    portal->streamList = NULL;
    portal->streamList = newRoot;
    return true;
}


void
portalFloatHandler(void * handle, char * msg, char * res)
{
    if (handle == NULL) return;
    if (res == NULL) return;
    float * var = handle;
    if (msg == NULL) sprintf(res, "%f", *var);
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
    if (handle == NULL) return;
    if (res == NULL) return;
    unsigned int * var = handle;
    if (msg == NULL) sprintf(res, "%u", *var);
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
    if (handle == NULL) return;
    if (res == NULL) return;
    unsigned long * var = handle;
    if (msg == NULL) sprintf(res, "%lu", *var);
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
    if (handle == NULL) return;
    if (res == NULL) return;
    bool * var = handle;
    if (msg == NULL) strcpy(res, *var ? "true" : "false");
    else if (strcmp(msg, "true") == 0)
    {
        *var = true;
    }
    else if (strcmp(msg, "false") == 0)
    {
        *var = false;
    }
}


void
portalStreamKeyHandler(void * handle, char * msg, char * res)
{
    if (handle == NULL) return;
    if (res == NULL) return;
    Portal * portal = handle;
    if (msg == NULL) portalGetStreamKeys(portal, res);
    else portalSetStreamKeys(portal, msg);
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
        char * result = fgets(input, LINESIZE, stdin);
        if (result == NULL) continue;

        trimSpaces(input);

        char * path = strtok(input, " ");
        char * message = strtok(NULL, "");

        char * portalId = strtok(path, ".");
        char * entryKey = strtok(NULL, ".");

        trimSpaces(portalId);
        trimSpaces(entryKey);

        Portal ** portalPos = findPortal(portalId, &pigeon->topPortal);
        if (*portalPos == NULL) continue;
        Portal * portal = *portalPos;

        PortalEntry ** entryPos = findEntry(entryKey, &portal->topEntry);
        if (*entryPos == NULL) continue;
        PortalEntry * entry = *entryPos;

        if (entry->handler == NULL) continue;
        char response[LINESIZE] = {0};
        entry->handler(entry->handle, message, response);

        if (!entry->manual) portalUpdate(portal, entry->key);

        if (response[0] == '\0') continue;

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
    if (pigeon->task != NULL) return;
    if (!pigeon->ready) return;
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
    if (portal == NULL) return true;
    if (!portal->ready) return false;
    if (!isPortalBranchReady(portal->portalLeft)) return false;
    if (!isPortalBranchReady(portal->portalRight)) return false;
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
    if (key[0] == '\0')
    {
        snprintf(path, LINESIZE, "%s", id);
    }
    else
    {
        snprintf(path, LINESIZE, "%s.%s", id, key);
    }

    int pathLength = strlen(path);
    int pathDisplayWidth = pathLength;

    // round up
    pathDisplayWidth += ALIGNSIZE - 1;
    pathDisplayWidth = (pathDisplayWidth / ALIGNSIZE) * ALIGNSIZE;
    if (pathDisplayWidth > LINESIZE - 1) pathDisplayWidth = LINESIZE - 1;

    while (pathLength < pathDisplayWidth)
    {
        path[pathLength] = ' ';
        pathLength++;
    }
    path[pathLength] = '\0';

    char str[LINESIZE];
    snprintf(
        str,
        LINESIZE,
        "[%08u|%s] %s",
        (unsigned int)pigeon->millis(),
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

static void
deleteEntryList(PortalEntryList * list)
{
    while (list != NULL)
    {
        PortalEntryList * old = list;
        list = list->next;
        free(old);
    }
}

static void
allocateMessage(Pigeon * pigeon, PortalEntry * entry)
{
    if (pigeon->messageUsers[pigeon->vacantIndex] != NULL)
    {
        PortalEntry * oldEntry = pigeon->messageUsers[pigeon->vacantIndex];
        oldEntry->message = NULL;
        pigeon->messageUsers[pigeon->vacantIndex] = NULL;
    }
    pigeon->messageUsers[pigeon->vacantIndex] = entry;
    entry->message = &pigeon->messages[pigeon->vacantIndex][0];
    pigeon->messages[pigeon->vacantIndex][0] = '\0';

    pigeon->vacantIndex++;
    if (pigeon->vacantIndex >= MAXMSG) pigeon->vacantIndex = 0;
}

static void
setupPigeonPortal(Pigeon * pigeon)
{
    pigeon->pigeonPortal = pigeonCreatePortal(pigeon, "pigeon");

    PortalEntrySetup setups[] =
    {
        {
            .key = "enable",
            .handler = enablePortalHandler,
            .handle = pigeon
        },
        {
            .key = "disable",
            .handler = disablePortalHandler,
            .handle = pigeon
        },

        // End terminating struct
        {
            .key = "~",
            .handler = NULL,
            .handle = NULL
        }
    };

    portalAddBatch(pigeon->pigeonPortal, setups);
    portalEnable(pigeon->pigeonPortal);
    portalReady(pigeon->pigeonPortal);
}

static void
enablePortalHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (message == NULL) return;
    if (response == NULL) return;
    Pigeon * pigeon = handle;
    char * id = strtok(message, " ");
    while (id != NULL)
    {
        Portal * portal = *findPortal(id, &pigeon->topPortal);
        portalEnable(portal);
        id = strtok(NULL, "");
    }
}

static void
disablePortalHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (message == NULL) return;
    if (response == NULL) return;
    Pigeon * pigeon = handle;
    char * id = strtok(message, " ");
    while (id != NULL)
    {
        Portal * portal = *findPortal(id, &pigeon->topPortal);
        portalDisable(portal);
        id = strtok(NULL, "");
    }
}

// }}}

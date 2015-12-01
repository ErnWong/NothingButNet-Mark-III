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
    Portal * topPortal;
    Portal * pigeonPortal;
    PigeonIn gets;
    PigeonOut puts;
    PigeonMillis millis;
    TaskHandle task;
    Mutex mutex;
    bool ready;
};

// }}}



// Private functions - forward declarations {{{

static void task(void*);
static void processInput(Pigeon*, char * portalId, char * entryKey, char * message);
static void checkReady(Pigeon*);
static bool isPortalBranchReady(Portal*);
static void writeMessage(
    Pigeon*,
    const char * id,
    const char * key,
    const char * message
);
static PortalEntry ** findEntry(const char * key, PortalEntry **);
static Portal ** findPortal(const char * id, Portal **);
static void deleteEntryList(PortalEntryList*);
static void setupPigeonPortal(Pigeon*);
static void enablePortalHandler(void * handle, char * message, char * response);
static void disablePortalHandler(void * handle, char * message, char * response);
static void getKeysHandler(void * handle, char * message, char * response);
static void logError(Pigeon*, char * message);

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
    pigeon->pigeonPortal = NULL;

    pigeon->mutex = mutexCreate();

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
    if (portal->ready)
    {
        logError(portal->pigeon, "add: portal already ready... ignoring add");
        return;
    }

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

    if (!portal->enabled)
    {
        return;
    }

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL)
    {
        char message[80];
        snprintf(message, 80, "set: cannot find entry with key %s\n", key);
        logError(portal->pigeon, message);
        return;
    }

    PortalEntry * entry = *location;
    if (entry->message == NULL)
    {
        char message[80];
        snprintf(message, 80, "set: message not allocated for key %s\n", key);
        logError(portal->pigeon, message);
        return;
    }

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

    if (!portal->enabled)
    {
        return;
    }

    PortalEntry ** location = findEntry(key, &portal->topEntry);

    if (*location == NULL)
    {
        char message[80];
        snprintf(message, 80, "update: cannot find entry with key '%s'", key);
        logError(portal->pigeon, message);
        return;
    }

    PortalEntry * entry = *location;

    if (entry->message == NULL)
    {
        char message[80];
        snprintf(message, 80, "update: message not allocated for key '%s'", key);
        logError(portal->pigeon, message);
        return;
    }

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

    if (!portal->enabled)
    {
        return;
    }

    PortalEntryList * list = portal->streamList;
    char output[LINESIZE] = {0};
    if (list == NULL)
    {
        // Don't write anything if no stream values
        return;
    }
    while (true)
    {
        if (list->entry == NULL)
        {
            logError(portal->pigeon, "flush: null entry encountered... aborting");
            return;
        }
        if (list->entry->message == NULL)
        {
            logError(portal->pigeon, "flush: entry message not allocated... aborting");
            return;
        }
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
        if (entryList->entry->message != NULL)
        {
            free(entryList->entry->message);
            entryList->entry->message = NULL;
        }
        entryList->entry->message = malloc(LINESIZE * sizeof(char));
        entryList->entry->message[0] = '\0';
        entryList = entryList->next;
    }
}


void
portalDisable(Portal * portal)
{
    if (portal == NULL) return;
    portal->enabled = false;
    PortalEntryList * entryList = portal->entryList;
    while (entryList != NULL)
    {
        free(entryList->entry->message);
        entryList->entry->message = NULL;
        entryList = entryList->next;
    }
}

void
portalGetStreamKeys(Portal * portal, char * destination)
{
    if (portal == NULL) return;

    PortalEntryList * list = portal->streamList;

    destination[0] = '\0';
    if (list == NULL)
    {
        return;
    }
    while (true)
    {
        stringAppend(destination, list->entry->key, LINESIZE);
        list = list->next;
        if (list == NULL) break;
        stringAppend(destination, " ", LINESIZE);
    }
}


// Note: sequence will be modified
bool
portalSetStreamKeys(Portal * portal, char * sequence)
{
    if (portal == NULL) return false;

    PortalEntryList initial;
    PortalEntryList * list = &initial;

    // Try to create the list

    char * key = strtok(sequence, " ");
    while (true)
    {
        PortalEntry ** entryPtr = findEntry(key, &portal->topEntry);

        if (*entryPtr == NULL)
        {
            // Fail, stop, cleanup.
            deleteEntryList(initial.next);
            char message[80];
            snprintf(message, 80, "setStreamKeys: cannot find entry with key '%s'", key);
            logError(portal->pigeon, message);
            return false;
        }
        list->next = malloc(sizeof(PortalEntryList));
        list = list->next;

        list->entry = *entryPtr;
        list->next = NULL;

        key = strtok(NULL, " ");
        if (key == NULL) break;
    }

    deleteEntryList(portal->streamList);
    portal->streamList = NULL;
    portal->streamList = initial.next;

    return true;
}


void
portalMutexTake(Portal * portal, unsigned long blockTime)
{
    mutexTake(portal->pigeon->mutex, blockTime);
}

void
portalMutexGive(Portal * portal)
{
    mutexGive(portal->pigeon->mutex);
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
portalIntHandler(void * handle, char * msg, char * res)
{
    if (handle == NULL) return;
    if (res == NULL) return;
    int * var = handle;
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

        char * inputTrimmed = trimSpaces(input);

        if (inputTrimmed[0] == '\0') continue;

        char * path = strtok(inputTrimmed, " ");
        char * message = strtok(NULL, "");

        char * portalId = strtok(path, ".");
        char * entryKey = strtok(NULL, ".");

        portalId = trimSpaces(portalId);
        entryKey = trimSpaces(entryKey);

        mutexTake(pigeon->mutex, -1);
        processInput(pigeon, portalId, entryKey, message);
        mutexGive(pigeon->mutex);

        delay(40);
    }
}

static void
processInput(Pigeon * pigeon, char * portalId, char * entryKey, char * message)
{
    Portal ** portalPos = findPortal(portalId, &pigeon->topPortal);
    if (*portalPos == NULL)
    {
        char message[80];
        snprintf(message, 80, "cannot find portal with id '%s'", portalId);
        logError(pigeon, message);
        return;
    }
    Portal * portal = *portalPos;

    PortalEntry ** entryPos = findEntry(entryKey, &portal->topEntry);
    if (*entryPos == NULL)
    {
        char message[80];
        snprintf(message, 80, "cannot find entry with key '%s'", entryKey);
        logError(pigeon, message);
        return;
    }
    PortalEntry * entry = *entryPos;

    if (entry->handler == NULL) return;
    char response[LINESIZE] = {0};
    entry->handler(entry->handle, message, response);

    if (!entry->manual) portalUpdate(portal, entry->key);

    if (response[0] == '\0') return;

    writeMessage(
        pigeon,
        portal->id,
        entry->key,
        response
    );
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
        {
            .key = "keys",
            .handler = getKeysHandler,
            .handle = pigeon
        },
        {
            .key = "error",
            .onchange = true
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

static void
getKeysHandler(void * handle, char * message, char * response)
{
    if (handle == NULL) return;
    if (message == NULL) return;
    if (response == NULL) return;
    Pigeon * pigeon = handle;
    Portal * portal = *findPortal(message, &pigeon->topPortal);
    portalGetStreamKeys(portal, response);
}

static void
logError(Pigeon * pigeon, char * message)
{
    portalSet(pigeon->pigeonPortal, "error", message);
}

// }}}

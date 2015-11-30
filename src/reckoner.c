#include "reckoner.h"

#include <math.h>
#include <stdbool.h>
#include "pigeon.h"
#include "shims.h"
#include "utils.h"

struct Reckoner;
typedef struct Reckoner Reckoner;

struct Reckoner
{
    Portal * portal;

    EncoderGetter encoderLeftGet;
    EncoderHandle encoderLeft;
    EncoderGetter encoderRightGet;
    EncoderHandle encoderRight;

    float gearingLeft;
    float gearingRight;
    float radiusLeft;
    float radiusRight;
    float wheelSeparation;
    float smoothing;

    unsigned long microTime;
    float timeChange;
    EncoderReading readingLeft;
    EncoderReading readingRight;
    float left;
    float right;
    float leftChange;
    float rightChange;
    float velocityLeftRaw;
    float velocityRightRaw;
    float velocityLeft;
    float velocityRight;
    ReckonerState state;
};

static void updateReadings(Reckoner*);
static void updateWheels(Reckoner*);
static void updateVelocity(Reckoner*);
static void updateHeading(Reckoner*);
static void updatePosition(Reckoner*);
static void updatePortal(Reckoner*);
static void setupPortal(Reckoner*, ReckonerSetup);

Reckoner *
reckonerInit(ReckonerSetup setup)
{
    Reckoner * r = malloc(sizeof(Reckoner));

    setupPortal(r, setup);

    r->microTime = micros();
    r->timeChange = 0;
    r->readingLeft.revolutions = 0;
    r->readingLeft.rpm = 0;
    r->readingRight.revolutions = 0;
    r->readingRight.rpm = 0;
    r->left = 0;
    r->right = 0;
    r->leftChange = 0;
    r->rightChange = 0;
    r->velocityLeftRaw = 0;
    r->velocityRightRaw = 0;
    r->velocityLeft = 0;
    r->velocityRight = 0;

    r->state.velocity = setup.initialVelocity;
    r->state.heading = setup.initialHeading;
    r->state.x = setup.initialY;
    r->state.y = setup.initialX;

    r->gearingLeft = setup.gearingLeft;
    r->gearingRight = setup.gearingRight;
    r->radiusLeft = setup.radiusLeft;
    r->radiusRight = setup.radiusRight;
    r->wheelSeparation = setup.wheelSeparation;
    r->smoothing = setup.smoothing;

    r->encoderLeftGet = setup.encoderLeftGetter;
    r->encoderLeft = setup.encoderLeft;
    r->encoderRightGet = setup.encoderRightGetter;
    r->encoderRight = setup.encoderRight;

    return r;
}

ReckonerState *
reckonerGetState(Reckoner * r)
{
    return &r->state;
}

void
reckonerUpdate(Reckoner * r)
{
    updateReadings(r);
    updateWheels(r);
    updateVelocity(r);
    updateHeading(r);
    updatePosition(r);
    updatePortal(r);
}

static void
updateReadings(Reckoner * r)
{
    r->timeChange = timeUpdate(&r->microTime);
    r->readingLeft = r->encoderLeftGet(r->encoderLeft);
    r->readingRight = r->encoderRightGet(r->encoderRight);
}
static void
updateWheels(Reckoner * r)
{
    float left = r->readingLeft.revolutions * SHIM_RADIANS_PER_REV;
    left *= r->gearingLeft;
    left *= r->radiusLeft;

    float right = r->readingRight.revolutions * SHIM_RADIANS_PER_REV;
    right *= r->gearingRight;
    right *= r->radiusRight;

    r->leftChange = left - r->left;
    r->rightChange = right - r->right;

    r->left = left;
    r->right = right;
}

static void
updateVelocity(Reckoner * r)
{
    r->velocityLeftRaw = r->readingLeft.rpm * SHIM_RADIANS_PER_REV / 60;
    r->velocityLeftRaw *= r->gearingLeft;
    r->velocityLeftRaw *= r->radiusLeft;

    r->velocityRightRaw = r->readingRight.rpm * SHIM_RADIANS_PER_REV / 60;
    r->velocityRightRaw *= r->gearingRight;
    r->velocityRightRaw *= r->radiusRight;

    float incrementLeft = (r->velocityLeftRaw - r->velocityLeft);
    incrementLeft *= r->timeChange;
    incrementLeft *= r->smoothing;

    float incrementRight = (r->velocityRightRaw - r->velocityRight);
    incrementRight *= r->timeChange;
    incrementRight *= r->smoothing;

    r->velocityLeft += incrementLeft;
    r->velocityRight += incrementRight;

    r->state.velocity = 0.5f * (r->velocityLeft + r->velocityRight);
}

static void
updateHeading(Reckoner * r)
{
    r->state.heading += (r->rightChange - r->leftChange) / r->wheelSeparation;
    r->state.heading = fmodf(r->state.heading + PI, TAU) - PI;
}

static void
updatePosition(Reckoner * r)
{
    r->state.x += r->timeChange * r->state.velocity * cosf(r->state.heading);
    r->state.y += r->timeChange * r->state.velocity * sinf(r->state.heading);
}

static void
updatePortal(Reckoner * r)
{
    portalUpdate(r->portal, "time");
    portalUpdate(r->portal, "left");
    portalUpdate(r->portal, "right");
    portalUpdate(r->portal, "left-change");
    portalUpdate(r->portal, "right-change");
    portalUpdate(r->portal, "velocity-left-raw");
    portalUpdate(r->portal, "velocity-right-raw");
    portalUpdate(r->portal, "velocity-left");
    portalUpdate(r->portal, "velocity-right");
    portalUpdate(r->portal, "velocity");
    portalUpdate(r->portal, "heading");
    portalUpdate(r->portal, "x");
    portalUpdate(r->portal, "y");
    portalFlush(r->portal);
}

static void
setupPortal(Reckoner * r, ReckonerSetup setup)
{
    r->portal = pigeonCreatePortal(setup.pigeon, setup.id);

    PortalEntrySetup setups[] =
    {
        {
            .key = "gearing-left",
            .handler = portalFloatHandler,
            .handle = &r->gearingLeft
        },
        {
            .key = "gearing-right",
            .handler = portalFloatHandler,
            .handle = &r->gearingRight
        },
        {
            .key = "radius-left",
            .handler = portalFloatHandler,
            .handle = &r->radiusLeft
        },
        {
            .key = "radius-right",
            .handler = portalFloatHandler,
            .handle = &r->radiusRight
        },
        {
            .key = "wheel-separation",
            .handler = portalFloatHandler,
            .handle = &r->wheelSeparation
        },
        {
            .key = "smoothing",
            .handler = portalFloatHandler,
            .handle = &r->smoothing
        },
        {
            .key = "time",
            .handler = portalUlongHandler,
            .handle = &r->microTime
        },
        {
            .key = "left",
            .handler = portalFloatHandler,
            .handle = &r->left
        },
        {
            .key = "right",
            .handler = portalFloatHandler,
            .handle = &r->right
        },
        {
            .key = "left-change",
            .handler = portalFloatHandler,
            .handle = &r->leftChange
        },
        {
            .key = "right-change",
            .handler = portalFloatHandler,
            .handle = &r->rightChange
        },
        {
            .key = "velocity-left-raw",
            .handler = portalFloatHandler,
            .handle = &r->velocityLeftRaw
        },
        {
            .key = "velocity-right-raw",
            .handler = portalFloatHandler,
            .handle = &r->velocityRightRaw
        },
        {
            .key = "velocity-left",
            .handler = portalFloatHandler,
            .handle = &r->velocityLeft
        },
        {
            .key = "velocity-right",
            .handler = portalFloatHandler,
            .handle = &r->velocityRight
        },
        {
            .key = "velocity",
            .handler = portalFloatHandler,
            .handle = &r->state.velocity,
            .stream = true
        },
        {
            .key = "heading",
            .handler = portalFloatHandler,
            .handle = &r->state.heading,
            .stream = true
        },
        {
            .key = "x",
            .handler = portalFloatHandler,
            .handle = &r->state.x,
            .stream = true
        },
        {
            .key = "y",
            .handler = portalFloatHandler,
            .handle = &r->state.x,
            .stream = true
        },
        {
            .key = "keys",
            .handler = portalStreamKeyHandler,
            .handle = r->portal
        },

        // End terminating struct
        {
            .key = "~",
            .handler = NULL,
            .handle = NULL
        }
    };
    portalAddBatch(r->portal, setups);
    char streamOrder[80] = "x y heading velocity";
    portalSetStreamKeys(r->portal, streamOrder);
    portalReady(r->portal);
}

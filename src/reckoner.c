#include "shims.h"

struct Reckoner;
typedef struct Reckoner Reckoner;

struct Reckoner
{
    float x;
    float y;
    float velocity;
    float heading;
    EncoderGetter encoderGetLeft;
    EncoderGetter encoderGetRight;

};



#ifndef Z_EN_VASE_H
#define Z_EN_VASE_H

#include "ultra64.h"
#include "global.h"

struct EnVase;

typedef void (*EnVaseActionFunc)(struct EnVase*, GlobalContext*);

typedef struct EnVase {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ EnVaseActionFunc actionFunc;
    /* 0x0150 */ s8 objBankIndex;
    /* 0x0152 */ s16 timer;
    /* 0x0154 */ ColliderCylinder collider;
} EnVase; // size = 0x01A0

#endif

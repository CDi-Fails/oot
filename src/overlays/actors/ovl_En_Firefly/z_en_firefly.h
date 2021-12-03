#ifndef Z_EN_FIREFLY_H
#define Z_EN_FIREFLY_H

#include "ultra64.h"
#include "global.h"

#define DICKBAG_HITSTUN 10
#define DICKBAG_SPEEDMODIFIER 2.5f

struct EnFirefly;

typedef void (*EnFireflyActionFunc)(struct EnFirefly*, GlobalContext*);

typedef union {
    s16 params;
    struct {
        s16 isInvisible : 1;
        s16 auraType : 8;
        s16 onFire : 1;
        s16 isKamikaze : 1;
        s16 isDickbag : 1;
        s16 action_type : 4;
    };
} KeeseParams;
// 0000000000110011: Dickbag Kamikaze
// 00000000 00 0 0 0000

typedef struct EnFirefly {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ Vec3f bodyPartsPos[3];
    /* 0x0170 */ SkelAnime skelAnime;
    /* 0x01B4 */ EnFireflyActionFunc actionFunc;
    KeeseParams config;
    /* 0x01BA */ s16 timer;
    /* 0x01BC */ s16 targetPitch;
    /* 0x01BE */ Vec3s jointTable[28];
    /* 0x0266 */ Vec3s morphTable[28];
    /* 0x0310 */ f32 maxAltitude;
    /* 0x0314 */ ColliderJntSph collider;
    /* 0x0344 */ ColliderJntSphElement colliderItems[1];
    struct {
        f32 TimerMult;
        f32 SpeedMult;
    } Hero;
} EnFirefly; // size = 0x0374

typedef enum {
    /* 0 */ KEESE_FIRE_FLY,
    /* 1 */ KEESE_FIRE_PERCH,
    /* 2 */ KEESE_NORMAL_FLY,
    /* 3 */ KEESE_NORMAL_PERCH,
    /* 4 */ KEESE_ICE_FLY,
    /* 5 */ KEESE_BIRI_FLY
} KeeseType;

#endif

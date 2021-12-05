#ifndef Z_EN_VASE_H
#define Z_EN_VASE_H

#include "ultra64.h"
#include "global.h"

struct EnVase;

typedef void (*EnVaseActionFunc)(struct EnVase*, GlobalContext*);

#define TEST_VARIABLE PROJ_ENERGYBALL
#define COLOR_COUNT 3
#define SHIELD_FLAG_DEKU 1
#define SHIELD_FLAG_HYLIAN 2
#define SHIELD_FLAG_MIRROR 4

typedef enum {
    PROJ_DEKU_NUT,
    PROJ_OCTO_ROCK,
    PROJ_BOMB,
    PROJ_ENERGYBALL
} ProjectileType;

typedef enum {
    DMG_FX_NONE,
    DMG_FX_FIRE,
    DMG_FX_ICE,
    DMG_FX_ELECTRIC,
    DMF_FX_EXPLOSIVE
} DamageEffects;

typedef enum {
    PARTICLE_NONE,
    PARTICLE_LIGHTBALL,
    PARTICLE_FLAME,
    PARTICLE_SPARK
} ParticleType;

typedef struct {
    s16 HealthDamage;
    s32 TouchEffect;
} DamageProperty;

typedef struct {
    struct {
        s16 id;
        Gfx* dlist;
    } object;
    struct {
        u8 spawnActorOnImpact : 4;
        u8 shieldCanBlock : 4;
    };
    DamageProperty Damage;
    f32 Velocity;
    f32 Gravity;
    f32 MasterScale;
    f32 Knockback;
    s16 Timer;
    s16 InvulnFrameOverride;
    s16 SoundEffect;
    Color_RGBA32 Colors[COLOR_COUNT];
    s16 MainColorFlicker;
    Vec3f TargetCoords;
    struct {
        s16 Type;
        s16 Density;
        s16 TrailLength;
        f32 Scale;
        Color_RGBA32 Colors[COLOR_COUNT];
        Color_RGBA32 SecondaryColors[COLOR_COUNT];
    } ParticleInit;
} EnVase_Config;

typedef struct EnVase {
    Actor actor;
    EnVaseActionFunc actionFunc;
    s8 objBankIndex;
    s16 timer;
    ColliderCylinder collider;
    EnVase_Config config;
    u32 end;
} EnVase;

#endif

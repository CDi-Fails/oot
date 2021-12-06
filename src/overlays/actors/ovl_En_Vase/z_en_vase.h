#ifndef Z_EN_VASE_H
#define Z_EN_VASE_H

#include "ultra64.h"
#include "global.h"

struct EnVase;

typedef void (*EnVaseActionFunc)(struct EnVase*, GlobalContext*);

#define Player_Knockback func_8002F71C

#define TEST_VARIABLE PROJ_SPARK
#define COLOR_COUNT 10
#define SHIELD_FLAG_DEKU 1
#define SHIELD_FLAG_HYLIAN 2
#define SHIELD_FLAG_MIRROR 4

typedef enum {
    PROJ_NONE,
    PROJ_DEKU_NUT,
    PROJ_OCTO_ROCK,
    PROJ_BOMB,
    PROJ_ENERGYBALL,
    PROJ_SPARK
} ProjectileType;

typedef enum {
    DMG_FX_NONE,
    DMG_FX_FIRE,
    DMG_FX_ICE,
    DMG_FX_ELECTRIC
} DamageEffects;

typedef enum {
    PARTICLE_NONE,
    PARTICLE_LIGHTBALL,
    PARTICLE_FLAME,
    PARTICLE_SPARK,
    PARTICLE_NAVISPARKLE
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
    struct {
        u8 isEnabled;
        Color_RGBA32 lightColor;
        s16 glowScale;
        LightInfo lightInfoGlow;
        LightNode* lightNodeGlow;
        LightInfo lightInfoNoGlow;
        LightNode* lightNodeNoGlow;
    } LightSource;
    struct {
        u8 bumpBoxOC;
        u8 hurtBoxAT;
        u8 hitBoxAC;
        DamageProperty Damage;
    } Collision;
    f32 Velocity;
    f32 Gravity;
    f32 MasterScale;
    f32 Knockback;
    s16 Timer;
    s16 InvulnFrameOverride;
    s16 SoundEffect;
    Color_RGBA32 CoreColors[COLOR_COUNT];
    Color_RGBA32 AuraColors[COLOR_COUNT];
    s16 MainColorFlicker;
    s16 AuraColorFlicker;
    Vec3f TargetCoords;
    struct {
        u8 isEnabled;
        s16 Type;
        s16 Density;
        Vec3f Velocity;
        Vec3f Acceleration;
        Vec3f Position;
        s16 TrailLength;
        f32 TrailVariation;
        s16 Scale;
        Color_RGBA32 Colors[COLOR_COUNT];
        Color_RGBA32 SecondaryColors[COLOR_COUNT];
        s16 ColorFlicker;
        s16 SecondaryColorFlicker;
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

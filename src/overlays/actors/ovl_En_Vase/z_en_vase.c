/*
 * File: z_en_vase.c
 * Overlay: ovl_En_Vase
 * Description: Projectile fired by deku scrubs
 */

#include "z_en_vase.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "overlays/effects/ovl_Effect_Ss_Hahen/z_eff_ss_hahen.h"
#include "objects/object_dekunuts/object_dekunuts.h"
#include "objects/object_okuta/object_okuta.h"
#include "objects/object_fhg/object_fhg.h"
#include "overlays/actors/ovl_En_Fhg_Fire/z_en_fhg_fire.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/effects/ovl_Effect_Ss_Fhg_Flash/z_eff_ss_fhg_flash.h"

#define FLAGS 0x00000010

#define THIS ((EnVase*)thisx)

void EnVase_Init(Actor* thisx, GlobalContext* globalCtx);
void EnVase_Destroy(Actor* thisx, GlobalContext* globalCtx);
void EnVase_Update(Actor* thisx, GlobalContext* globalCtx);
void EnVase_Draw(Actor* thisx, GlobalContext* globalCtx);

void EnVase_UpdateSetup(EnVase* this, GlobalContext* globalCtx);
void EnVase_ModeUpdate(EnVase* this, GlobalContext* globalCtx);
void EnVase_SpawnSparkles(EnVase* this, GlobalContext* globalCtx, s32 sparkleLife);
void EnVase_SpawnFlameTrail(EnVase* this, GlobalContext* globalCtx, s32 sparkleLife);
void EnVase_SpawnLightningTrail(EnVase* this, GlobalContext* globalCtx);

const ActorInit En_Vase_InitVars = {
    ACTOR_EN_VASE,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(EnVase),
    (ActorFunc)EnVase_Init,
    (ActorFunc)EnVase_Destroy,
    (ActorFunc)EnVase_Update,
    (ActorFunc)NULL,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x00, 0x08 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_ON,
    },
    { 13, 13, 0, { 0 } },
};

void EnVase_Init(Actor* thisx, GlobalContext* globalCtx) {
    EnVase* this = THIS;
    s32 pad;

    this->config.object.id = OBJECT_FHG;
    //this->config.object.dlist = gDekuNutsDekuNutDL;
    this->config.spawnActorOnImpact = true;
    this->config.shieldCanBlock = SHIELD_FLAG_DEKU | SHIELD_FLAG_MIRROR;
    this->config.Collision.bumpBoxOC = true;
    this->config.Collision.hurtBoxAT = true;
    this->config.Collision.hitBoxAC = false;
    this->config.Collision.Damage.HealthDamage = 16;
    this->config.Collision.Damage.TouchEffect = DMG_FX_ELECTRIC;
    this->config.Velocity = 15.0f;
    this->config.Gravity = 0.0f;
    this->config.MasterScale = 6.0f;
    this->config.Knockback = 7.0f;
    this->config.Timer = 30;
    this->config.SoundEffect = NA_SE_EN_BALINADE_BL_SPARK;
    this->config.CoreColors[0].rgba = 0xEDFFFDFF;
    this->config.CoreColors[1].rgba = 0xBFE8FDFF;
    this->config.CoreColors[2].rgba = 0x91D1FEFF;
    this->config.AuraColors[0].rgba = 0xBFE8FDFF;
    this->config.AuraColors[1].rgba = 0x91D1FEFF;
    this->config.AuraColors[2].rgba = 0x64BAFEFF;
    this->config.MainColorFlicker = 2;
    this->config.AuraColorFlicker = 2;
    this->config.LightSource.isEnabled = true;
    this->config.LightSource.glowScale = 48;
    this->config.LightSource.lightColor = this->config.CoreColors[this->config.AuraColorFlicker];
    this->config.TargetCoords.x = 0.0f;
    this->config.TargetCoords.y = 0.0f;
    this->config.TargetCoords.z = 0.0f;
    this->config.ParticleInit.isEnabled = true;
    this->config.ParticleInit.Type = PARTICLE_SPARK;
    this->config.ParticleInit.Density = 2;
    this->config.ParticleInit.TrailLength = 12;
    this->config.ParticleInit.TrailVariation = 12.0f;
    this->config.ParticleInit.Scale = (s16)(3.0f);
    this->config.ParticleInit.Colors[0].rgba = 0xEDFFFDFF;
    this->config.ParticleInit.Colors[1].rgba = 0xBFE8FDFF;
    this->config.ParticleInit.Colors[2].rgba = 0x91D1FEFF;
    this->config.ParticleInit.SecondaryColors[0].rgba = 0xBFE8FDFF;
    this->config.ParticleInit.SecondaryColors[1].rgba = 0x91D1FEFF;
    this->config.ParticleInit.SecondaryColors[2].rgba = 0x64BAFEFF;
    this->config.ParticleInit.ColorFlicker=2;
    this->config.ParticleInit.SecondaryColorFlicker=2;
    this->collider.dim.radius = (s16)(1 + (this->config.MasterScale / 2));
    this->collider.dim.height = (s16)(1 + (this->config.MasterScale));
    this->end = 0xDEADBEEF;

    //ActorShape_Init(&this->actor.shape, 400.0f, ActorShadow_DrawCircle, 13.0f);
    Collider_InitCylinder(globalCtx, &this->collider);
    Collider_SetCylinder(globalCtx, &this->collider, &this->actor, &sCylinderInit);
    this->objBankIndex = Object_GetIndex(&globalCtx->objectCtx, this->config.object.id);

    if (this->objBankIndex < 0) {
        Actor_Kill(&this->actor);
    } else {
        this->actionFunc = EnVase_UpdateSetup;
    }
}

void EnVase_Destroy(Actor* thisx, GlobalContext* globalCtx) {
    EnVase* this = THIS;

    LightContext_RemoveLight(globalCtx, &globalCtx->lightCtx, this->config.LightSource.lightNodeGlow);
    LightContext_RemoveLight(globalCtx, &globalCtx->lightCtx, this->config.LightSource.lightNodeNoGlow);
    Collider_DestroyCylinder(globalCtx, &this->collider);
}

void EnVase_UpdateSetup(EnVase* this, GlobalContext* globalCtx) {
    if (Object_IsLoaded(&globalCtx->objectCtx, this->objBankIndex)) {
        this->actor.objBankIndex = this->objBankIndex;
        this->actor.draw = EnVase_Draw;
        this->actor.shape.rot.y = 0;
        this->timer = this->config.Timer;
        this->actionFunc = EnVase_ModeUpdate;
        this->actor.speedXZ = this->config.Velocity;
        Actor_SetScale(&this->actor, this->config.MasterScale);
        this->collider.info.toucher.damage = this->config.Collision.Damage.HealthDamage;
        this->collider.info.toucher.effect = this->config.Collision.Damage.TouchEffect;

        if (this->config.LightSource.isEnabled) {
                Lights_PointGlowSetInfo(&this->config.LightSource.lightInfoGlow, this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z,
                this->config.LightSource.lightColor.r, this->config.LightSource.lightColor.g, this->config.LightSource.lightColor.b, 0);
                this->config.LightSource.lightNodeGlow = LightContext_InsertLight(globalCtx, &globalCtx->lightCtx, &this->config.LightSource.lightInfoGlow);

                Lights_PointNoGlowSetInfo(&this->config.LightSource.lightInfoNoGlow, this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z,
                this->config.LightSource.lightColor.r, this->config.LightSource.lightColor.g, this->config.LightSource.lightColor.b, 0);
                this->config.LightSource.lightNodeNoGlow = LightContext_InsertLight(globalCtx, &globalCtx->lightCtx, &this->config.LightSource.lightInfoNoGlow);
        }
    }
}

/* Shield Code is Here */
void EnVase_ModeUpdate(EnVase* this, GlobalContext* globalCtx) {
    EnBom* bomb;
    Player* player = GET_PLAYER(globalCtx);
    f32 intensity;
    Vec3s sp4C;
    Vec3f sp40;

    this->timer--;

    if (this->timer == 0) {
        this->actor.gravity = -1.0f;
    }

    this->actor.home.rot.z += 0x2AA8;

    if ((this->actor.bgCheckFlags & 8) || (this->actor.bgCheckFlags & 1) || (this->collider.base.atFlags & AT_HIT) ||
        (this->collider.base.acFlags & AC_HIT) || (this->collider.base.ocFlags1 & OC1_HIT)) {
        
        // Checking if the player is using a shield that reflects projectiles
        // And if so, reflects the projectile on impact
        if (BIT_CHECK(LibAxis_PowI(2, player->currentShield - 1), this->config.shieldCanBlock)) {
            //*(u32*)0x80700000 = 1;
            if ((this->collider.base.atFlags & AT_HIT) && (this->collider.base.atFlags & AT_TYPE_ENEMY) &&
                (this->collider.base.atFlags & AT_BOUNCED)) {
                this->collider.base.atFlags &= ~AT_TYPE_ENEMY & ~AT_BOUNCED & ~AT_HIT;
                this->collider.base.atFlags |= AT_TYPE_PLAYER;

                this->collider.info.toucher.dmgFlags = 2;
                Matrix_MtxFToYXZRotS(&player->shieldMf, &sp4C, 0);
                this->actor.world.rot.y = sp4C.y + 0x8000;
                this->timer = this->config.Timer;
                return;
            }
        }

        if (player->isTakeDamage) {
            Player_Knockback(globalCtx, &player->actor, this->config.Knockback, player->actor.world.rot.y, 1);
            if(this->config.InvulnFrameOverride > 0) {
               player->invincibilityTimer = this->config.InvulnFrameOverride;
            }
        }

        sp40.x = this->actor.world.pos.x;
        sp40.y = this->actor.world.pos.y + 4;
        sp40.z = this->actor.world.pos.z;

        switch(this->actor.params) {
            case PROJ_DEKU_NUT: {
                EffectSsHahen_SpawnBurst(globalCtx, &sp40, 6.0f, 0, 7, 3, 15, HAHEN_OBJECT_DEFAULT, 10, NULL);
                Audio_PlaySoundAtPosition(globalCtx, &this->actor.world.pos, 20, NA_SE_EN_OCTAROCK_ROCK);
            } break;
            case PROJ_OCTO_ROCK: { 
                EffectSsHahen_SpawnBurst(globalCtx, &sp40, 6.0f, 0, 1, 2, 15, 7, 10, gOctorokProjectileDL);
                Audio_PlaySoundAtPosition(globalCtx, &this->actor.world.pos, 20, NA_SE_EN_OCTAROCK_ROCK);
            } break;
            case PROJ_BOMB: {
                bomb = (EnBom*)Actor_Spawn(&globalCtx->actorCtx, globalCtx, ACTOR_EN_BOM, this->actor.world.pos.x,
                                   this->actor.world.pos.y, this->actor.world.pos.z, 0, 0, 0x6FF, BOMB_BODY);

                if (bomb != NULL) {
                    bomb->timer = 0;
                }
            } break;
            case PROJ_ENERGYBALL: {
                Audio_PlaySoundAtPosition(globalCtx, &this->actor.world.pos, 20, NA_SE_IT_BOMB_UNEXPLOSION);
            } break;
        }
        Actor_Kill(&this->actor);
    } else {
        if (this->timer == -300) {
            Actor_Kill(&this->actor);
        }
    }
}

void EnVase_Update(Actor* thisx, GlobalContext* globalCtx) {
    EnVase* this = THIS;
    Player* player = GET_PLAYER(globalCtx);
    s32 pad;

    if (!(player->stateFlags1 & 0x300000C0) || (this->actionFunc == EnVase_UpdateSetup)) {
        this->actionFunc(this, globalCtx);

        if (this->config.SoundEffect != 0) {
            func_8002F974(&this->actor, this->config.SoundEffect - SFX_FLAG);
        }

        if (this->config.LightSource.isEnabled) {
            Lights_PointGlowSetInfo(
                &this->config.LightSource.lightInfoGlow,
                this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z,
                this->config.LightSource.lightColor.r, this->config.LightSource.lightColor.g, this->config.LightSource.lightColor.b,
                this->config.LightSource.glowScale);
        }

        if (this->config.ParticleInit.isEnabled) {
            switch(this->config.ParticleInit.Type) {
                case PARTICLE_FLAME: {
                    //EnVase_SpawnFlame(this, globalCtx, (s32)this->config.ParticleInit.TrailLength);
                } break;
                case PARTICLE_NAVISPARKLE: {
                    EnVase_SpawnSparkles(this, globalCtx, this->config.ParticleInit.TrailLength);
                } break;
                case PARTICLE_SPARK:{
                    EnVase_SpawnLightningTrail(this, globalCtx);
                } break;
            }
        }

        Actor_MoveForward(&this->actor);
        Actor_UpdateBgCheckInfo(globalCtx, &this->actor, 10, sCylinderInit.dim.radius, sCylinderInit.dim.height, 5);
        Collider_UpdateCylinder(&this->actor, &this->collider);

        this->actor.flags |= 0x1000000;

        if (this->config.Collision.hurtBoxAT) {
            CollisionCheck_SetAT(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
        }

        if (this->config.Collision.hitBoxAC) {
            CollisionCheck_SetAC(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
        }

        if (this->config.Collision.bumpBoxOC) {
            CollisionCheck_SetOC(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
        }
    }
}

void EnVase_DrawDekuNut(EnVase* this, GlobalContext* globalCtx) {

    this->config.object.dlist = gDekuNutsDekuNutDL;
    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
    {
            func_80093D18(globalCtx->state.gfxCtx);
            Matrix_Mult(&globalCtx->billboardMtxF, MTXMODE_APPLY);
            Matrix_RotateZ(this->actor.home.rot.z * 9.58738e-05f, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD);
            gSPDisplayList(POLY_OPA_DISP++, this->config.object.dlist);
    }
    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);

}

void EnVase_DrawOctoRock(EnVase* this, GlobalContext* globalCtx) {
    
    this->config.object.dlist = gOctorokProjectileDL;
    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
    {
            func_80093D18(globalCtx->state.gfxCtx);
            Matrix_Mult(&globalCtx->billboardMtxF, MTXMODE_APPLY);
            Matrix_RotateZ(this->actor.home.rot.z * 9.58738e-05f, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD);
            gSPDisplayList(POLY_OPA_DISP++, this->config.object.dlist);
    }
    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);

}

void EnVase_DrawBomb(EnVase* this, GlobalContext* globalCtx) {

    this->config.object.dlist = gBombBodyDL;
    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
    {
        func_80093D18(globalCtx->state.gfxCtx);
        func_800D1FD4(&globalCtx->billboardMtxF);
        Matrix_RotateZYX(0x4000, 0, 0, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, this->config.object.dlist);
    }
    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);

}

void EnVase_SpawnSparkles(EnVase* this, GlobalContext* globalCtx, s32 sparkleLife) {
    static Vec3f sparkleVelocity = { 0.0f, -0.05f, 0.0f };
    static Vec3f sparkleAccel = { 0.0f, -0.025f, 0.0f };
    s32 pad;
    Color_RGBA8 primColor;
    Color_RGBA8 envColor;
    s16 setColor;

    this->config.ParticleInit.Position.x = Rand_CenteredFloat(this->config.ParticleInit.TrailVariation) + this->actor.world.pos.x;
    this->config.ParticleInit.Position.y = (Rand_ZeroOne() * this->config.ParticleInit.TrailVariation) + this->actor.world.pos.y;
    this->config.ParticleInit.Position.z = Rand_CenteredFloat(this->config.ParticleInit.TrailVariation) + this->actor.world.pos.z;

    setColor = Rand_S16Offset(0, this->config.ParticleInit.ColorFlicker);

    primColor.r = this->config.ParticleInit.Colors[setColor].r;
    primColor.g = this->config.ParticleInit.Colors[setColor].g;
    primColor.b = this->config.ParticleInit.Colors[setColor].b;

    setColor = Rand_S16Offset(0, this->config.ParticleInit.SecondaryColorFlicker);

    envColor.r = this->config.ParticleInit.SecondaryColors[setColor].r;
    envColor.g = this->config.ParticleInit.SecondaryColors[setColor].g;
    envColor.b = this->config.ParticleInit.SecondaryColors[setColor].b;

    EffectSsKiraKira_SpawnDispersed(
        globalCtx, &this->config.ParticleInit.Position, &sparkleVelocity,
        &sparkleAccel, &primColor, &envColor,
        this->config.ParticleInit.Scale,
        sparkleLife
    );
}


void EnVase_SpawnFlameTrail(EnVase* this, GlobalContext* globalCtx, s32 sparkleLife) {

}

void EnVase_SpawnLightningTrail(EnVase* this, GlobalContext* globalCtx){
    s16 effectYaw;
    Vec3f effPos;
    s32 i;
    s16 setColor;
    Color_RGBA8 primColor;
    Color_RGBA8 envColor;

    setColor = Rand_S16Offset(0, this->config.ParticleInit.ColorFlicker);
    primColor.r = this->config.ParticleInit.Colors[setColor].r;
    primColor.g = this->config.ParticleInit.Colors[setColor].g;
    primColor.b = this->config.ParticleInit.Colors[setColor].b;

    setColor = Rand_S16Offset(0, this->config.ParticleInit.SecondaryColorFlicker);
    envColor.r = this->config.ParticleInit.SecondaryColors[setColor].r;
    envColor.g = this->config.ParticleInit.SecondaryColors[setColor].g;
    envColor.b = this->config.ParticleInit.SecondaryColors[setColor].b;

    for(i = this->config.ParticleInit.Density; i > 0 && this->config.ParticleInit.Density>0; --i) {
        effectYaw = (s16)Rand_CenteredFloat(12288.0f) + (65536/this->config.ParticleInit.Density*i) + 0x2000;
        this->config.ParticleInit.Position.x = Rand_CenteredFloat(this->config.ParticleInit.TrailVariation) + this->actor.world.pos.x;
        this->config.ParticleInit.Position.y = (Rand_ZeroOne() * this->config.ParticleInit.TrailVariation) + this->actor.world.pos.y;
        this->config.ParticleInit.Position.z = Rand_CenteredFloat(this->config.ParticleInit.TrailVariation) + this->actor.world.pos.z;

        //func_8002F974(&this->actor, NA_SE_EN_BIRI_SPARK);
        // EffectSsLightning_Spawn(globalCtx, &this->config.ParticleInit.Position, &primColor, &envColor, this->config.ParticleInit.Scale, effectYaw, this->config.ParticleInit.TrailLength, 2);
    }
}


void EnVase_DrawEnergyBall(EnVase* this, GlobalContext* globalCtx) {
    f32 sp6C = globalCtx->state.frames & 0x1F;
    s16 setColor;
    this->config.object.dlist = gPhantomEnergyBallDL;

    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
    {
        func_80093D84(globalCtx->state.gfxCtx);
        if(this->config.MainColorFlicker >0) { //Flickering
            setColor = Rand_S16Offset(0, this->config.MainColorFlicker);
            gDPSetPrimColor(
                POLY_XLU_DISP++, 0, 0x80,
                this->config.CoreColors[setColor].r,
                this->config.CoreColors[setColor].g,
                this->config.CoreColors[setColor].b,
                this->config.CoreColors[setColor].a
            );
        }
        else { //Singlecolor
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, this->config.CoreColors[0].r, this->config.CoreColors[0].g, this->config.CoreColors[0].b, this->config.CoreColors[0].a);
        }
        
        if(this->config.AuraColorFlicker >0) { //Flickering
            setColor = Rand_S16Offset(0, this->config.AuraColorFlicker);
            gDPSetEnvColor(POLY_XLU_DISP++, this->config.AuraColors[setColor].r, this->config.AuraColors[setColor].g, this->config.AuraColors[setColor].b, this->config.AuraColors[setColor].a);
        }
        else{ //Singlecolor
             gDPSetEnvColor(POLY_XLU_DISP++, this->config.AuraColors[0].r, this->config.AuraColors[0].g, this->config.AuraColors[0].b, this->config.AuraColors[0].a);
        }

        Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y + 4, this->actor.world.pos.z, MTXMODE_NEW);
        Matrix_Scale(this->config.MasterScale, this->config.MasterScale, this->config.MasterScale, MTXMODE_APPLY);
        Matrix_Mult(&globalCtx->billboardMtxF, MTXMODE_APPLY);
        Matrix_Push();
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Matrix_RotateZ(sp6C * (M_PI / 32), MTXMODE_APPLY);
        gSPDisplayList(POLY_XLU_DISP++, this->config.object.dlist);
        Matrix_Pop();
        Matrix_RotateZ(-sp6C * (M_PI / 32), MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, this->config.object.dlist);

    }
    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}

void EnVase_SpawnLightningBall(EnVase* this, GlobalContext* globalCtx){
    s16 effectYaw;
    Vec3f effect;
    s32 i;
    s16 setColor;
    Color_RGBA8 primColor;
    Color_RGBA8 envColor;

    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);

    this->config.object.dlist = gPhantomEnergyBallDL;
    func_80093D84(globalCtx->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, this->config.object.dlist);

    setColor = Rand_S16Offset(0, this->config.ParticleInit.ColorFlicker);
    primColor.r = this->config.CoreColors[setColor].r;
    primColor.g = this->config.CoreColors[setColor].g;
    primColor.b = this->config.CoreColors[setColor].b;

    setColor = Rand_S16Offset(0, this->config.ParticleInit.SecondaryColorFlicker);
    envColor.r = this->config.AuraColors[setColor].r;
    envColor.g = this->config.AuraColors[setColor].g;
    envColor.b = this->config.AuraColors[setColor].b;

    for(i = 5; i > 0; --i) {
        effectYaw = (s16)Rand_CenteredFloat(12288.0f) + (65536/5*i) + 0x2000;
        this->config.ParticleInit.Position.x = Rand_CenteredFloat(this->config.MasterScale*2) + this->actor.world.pos.x;
        this->config.ParticleInit.Position.y = (Rand_ZeroOne() * this->config.MasterScale*2) + this->actor.world.pos.y;
        this->config.ParticleInit.Position.z = Rand_CenteredFloat(this->config.MasterScale*2) + this->actor.world.pos.z;

        //func_8002F974(&this->actor, NA_SE_EN_BIRI_SPARK);
        //EffectSsLightning_Spawn(globalCtx, &this->actor.world.pos, &primColor, &envColor, this->config.MasterScale, effectYaw, 2, 4); //Failed implementation 1

        EffectSsFhgFlash_SpawnShock(globalCtx, &this->actor, &this->config.ParticleInit.Position, this->config.MasterScale * 8, FHGFLASH_SHOCK_NO_ACTOR); //Failed implementation 2
    }

    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}


void EnVase_Draw(Actor* thisx, GlobalContext* globalCtx) {
    EnVase* this = THIS;
    s32 pad;

    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);

    switch(this->actor.params) {
        case PROJ_DEKU_NUT: {
            EnVase_DrawDekuNut(this, globalCtx);
        } break;
        case PROJ_OCTO_ROCK: { 
            EnVase_DrawOctoRock(this, globalCtx);
        } break;
        case PROJ_BOMB: {
            EnVase_DrawBomb(this, globalCtx);
        } break;
        case PROJ_ENERGYBALL: {
            EnVase_DrawEnergyBall(this, globalCtx);
        } break;
        case PROJ_SPARK: {
            EnVase_SpawnLightningBall(this, globalCtx);
        } break;
    }

    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}

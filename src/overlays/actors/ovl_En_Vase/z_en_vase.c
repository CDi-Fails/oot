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

#define FLAGS 0x00000010

#define THIS ((EnVase*)thisx)

void EnVase_Init(Actor* thisx, GlobalContext* globalCtx);
void EnVase_Destroy(Actor* thisx, GlobalContext* globalCtx);
void EnVase_Update(Actor* thisx, GlobalContext* globalCtx);
void EnVase_Draw(Actor* thisx, GlobalContext* globalCtx);

void EnVase_UpdateSetup(EnVase* this, GlobalContext* globalCtx);
void EnVase_ModeUpdate(EnVase* this, GlobalContext* globalCtx);

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
        TOUCH_ON | TOUCH_SFX_WOOD,
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
    this->config.shieldCanBlock = SHIELD_FLAG_HYLIAN | SHIELD_FLAG_MIRROR;
    this->config.Damage.HealthDamage = 48;
    this->config.Damage.TouchEffect = DMG_FX_FIRE;
    this->config.Velocity = 8.0f;
    this->config.Gravity = 0.0f;
    this->config.MasterScale = 6.0f;
    this->config.Knockback = 5.0f;
    this->config.Timer = 30;
    this->config.SoundEffect = NA_SE_PL_ARROW_CHARGE_FIRE;
    this->config.Colors[0].rgba = 0xFFFFFFFF;
    this->config.Colors[1].rgba = 0xFF1500FF;
    this->config.Colors[2].rgba = 0xFF7C00FF;
    this->config.MainColorFlicker = 0;
    this->config.TargetCoords.x = 0.0f;
    this->config.TargetCoords.y = 0.0f;
    this->config.TargetCoords.z = 0.0f;
    this->config.ParticleInit.Type = 0;
    this->config.ParticleInit.Density = 0;
    this->config.ParticleInit.TrailLength = 0;
    this->config.ParticleInit.Scale = 0.0f;
    this->config.ParticleInit.Colors[0].rgba = 0xFFFFFFFF;
    this->config.ParticleInit.Colors[1].rgba = 0xFFFFFFFF;
    this->config.ParticleInit.Colors[2].rgba = 0xFFFFFFFF;
    this->config.ParticleInit.SecondaryColors[0].rgba = 0xFFFFFFFF;
    this->config.ParticleInit.SecondaryColors[1].rgba = 0xFFFFFFFF;
    this->config.ParticleInit.SecondaryColors[2].rgba = 0xFFFFFFFF;
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
        this->collider.info.toucher.damage = this->config.Damage.HealthDamage;
        this->collider.info.toucher.effect = this->config.Damage.TouchEffect;
    }
}

/* Shield Code is Here */
void EnVase_ModeUpdate(EnVase* this, GlobalContext* globalCtx) {
    EnBom* bomb;
    Player* player = GET_PLAYER(globalCtx);
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

        Actor_MoveForward(&this->actor);
        Actor_UpdateBgCheckInfo(globalCtx, &this->actor, 10, sCylinderInit.dim.radius, sCylinderInit.dim.height, 5);
        Collider_UpdateCylinder(&this->actor, &this->collider);

        this->actor.flags |= 0x1000000;

        CollisionCheck_SetAT(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
        CollisionCheck_SetAC(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
        CollisionCheck_SetOC(globalCtx, &globalCtx->colChkCtx, &this->collider.base);
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

void EnVase_DrawEnergyBall(EnVase* this, GlobalContext* globalCtx) {
    f32 sp6C = globalCtx->state.frames & 0x1F;
    this->config.object.dlist = gPhantomEnergyBallDL;

    OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
    {
        func_80093D84(globalCtx->state.gfxCtx);
        if(this->config.MainColorFlicker != 0 && this->timer % 2 < 1) {
            gDPSetPrimColor(
                POLY_XLU_DISP++, 0, 0x80,
                this->config.Colors[this->config.MainColorFlicker].r,
                this->config.Colors[this->config.MainColorFlicker].g,
                this->config.Colors[this->config.MainColorFlicker].b,
                this->config.Colors[this->config.MainColorFlicker].a
            );
        }
        else { //Normal color
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, this->config.Colors[0].r, this->config.Colors[0].g, this->config.Colors[0].b, this->config.Colors[0].a);
        }
        
        if(this->timer % 2 < 1) {
             gDPSetEnvColor(POLY_XLU_DISP++, this->config.Colors[1].r, this->config.Colors[1].g, this->config.Colors[1].b, this->config.Colors[1].a);
        }
        else{
             gDPSetEnvColor(POLY_XLU_DISP++, this->config.Colors[2].r, this->config.Colors[2].g, this->config.Colors[2].b, this->config.Colors[2].a);
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
    }

    switch(this->config.ParticleInit.Type) {
        case PARTICLE_NONE: {
            //Something might happen here in the future
        } break;
        case PARTICLE_LIGHTBALL: {
            
        } break;
        case PARTICLE_FLAME: {

        } break;
        case PARTICLE_SPARK: {

        } break;
    }

    CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}

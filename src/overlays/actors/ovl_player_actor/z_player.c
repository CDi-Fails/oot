/*
 * File: z_player.c
 * Overlay: ovl_player_actor
 * Description: Link
 */

#include "ultra64.h"
#include "global.h"
#include "quake.h"

#include "overlays/actors/ovl_Bg_Heavy_Block/z_bg_heavy_block.h"
#include "overlays/actors/ovl_Demo_Kankyo/z_demo_kankyo.h"
#include "overlays/actors/ovl_Door_Shutter/z_door_shutter.h"
#include "overlays/actors/ovl_En_Boom/z_en_boom.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "overlays/actors/ovl_En_Fish/z_en_fish.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "overlays/actors/ovl_En_Insect/z_en_insect.h"
#include "overlays/effects/ovl_Effect_Ss_Fhg_Flash/z_eff_ss_fhg_flash.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "assets/objects/object_link_child/object_link_child.h"

typedef struct {
    /* 0x00 */ u8 itemId;
    /* 0x01 */ u8 field; // various bit-packed data
    /* 0x02 */ s8 gi;    // defines the draw id and chest opening animation
    /* 0x03 */ u8 textId;
    /* 0x04 */ u16 objectId;
} GetItemEntry; // size = 0x06

#define GET_ITEM(itemId, objectId, drawId, textId, field, chestAnim) \
    { itemId, field, (chestAnim != CHEST_ANIM_SHORT ? 1 : -1) * (drawId + 1), textId, objectId }

#define CHEST_ANIM_SHORT 0
#define CHEST_ANIM_LONG 1

#define GET_ITEM_NONE \
    { ITEM_NONE, 0, 0, 0, OBJECT_INVALID }

typedef struct {
    /* 0x00 */ u8 itemId;
    /* 0x02 */ s16 actorId;
} ExplosiveInfo; // size = 0x04

typedef struct {
    /* 0x00 */ s16 actorId;
    /* 0x02 */ u8 itemId;
    /* 0x03 */ u8 itemAction;
    /* 0x04 */ u8 textId;
} BottleCatchInfo; // size = 0x06

typedef struct {
    /* 0x00 */ s16 actorId;
    /* 0x02 */ s16 actorParams;
} BottleDropInfo; // size = 0x04

typedef struct {
    /* 0x00 */ s8 damage;
    /* 0x01 */ u8 rumbleStrength;
    /* 0x02 */ u8 rumbleDuration;
    /* 0x03 */ u8 rumbleDecreaseRate;
    /* 0x04 */ u16 sfxId;
} FallImpactInfo; // size = 0x06

typedef struct {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ s16 yaw;
} SpecialRespawnInfo; // size = 0x10

typedef struct {
    /* 0x00 */ u16 sfxId;
    /* 0x02 */ s16 field; // contains frame to play anim on, sign determines whether to stop processing entries
} PlayerAnimSfxEntry;     // size = 0x04

typedef struct {
    /* 0x00 */ u16 swordSfx;
    /* 0x02 */ s16 voiceSfx;
} SwordPedestalSfx; // size = 0x04

typedef struct {
    /* 0x00 */ LinkAnimationHeader* anim;
    /* 0x04 */ u8 itemChangeFrame;
} ItemChangeAnimInfo; // size = 0x08

typedef struct {
    /* 0x00 */ LinkAnimationHeader* bottleSwingAnim;
    /* 0x04 */ LinkAnimationHeader* bottleCatchAnim;
    /* 0x08 */ u8 unk_08;
    /* 0x09 */ u8 unk_09;
} BottleSwingAnimInfo; // size = 0x0C

typedef struct {
    /* 0x00 */ LinkAnimationHeader* startAnim;
    /* 0x04 */ LinkAnimationHeader* endAnim;
    /* 0x08 */ LinkAnimationHeader* fightEndAnim;
    /* 0x0C */ u8 startFrame;
    /* 0x0D */ u8 endFrame;
} MeleeAttackAnimInfo; // size = 0x10

typedef struct {
    /* 0x00 */ LinkAnimationHeader* anim;
    /* 0x04 */ f32 riderOffsetX;
    /* 0x04 */ f32 riderOffsetZ;
} HorseMountAnimInfo; // size = 0x0C

typedef struct {
    /* 0x00 */ s8 playbackFuncID;
    /* 0x04 */ union {
        void* ptr;
        void (*func)(PlayState*, Player*, CsCmdActorAction*);
    };
} CutsceneModeEntry; // size = 0x08

typedef struct {
    /* 0x00 */ s16 unk_00;
    /* 0x02 */ s16 unk_02;
    /* 0x04 */ s16 unk_04;
    /* 0x06 */ s16 unk_06;
    /* 0x08 */ s16 unk_08;
} struct_80858AC8; // size = 0x0A

void Player_DoNothing(PlayState* play, Player* this);
void Player_DoNothing2(PlayState* play, Player* this);
void Player_SetupBowOrSlingshot(PlayState* play, Player* this);
void Player_SetupDekuStick(PlayState* play, Player* this);
void Player_SetupExplosive(PlayState* play, Player* this);
void Player_SetupHookshot(PlayState* play, Player* this);
void Player_SetupBoomerang(PlayState* play, Player* this);
void Player_ChangeItem(PlayState* play, Player* this, s8 itemAction);
s32 Player_SetupStartZTargetDefend(Player* this, PlayState* play);
s32 Player_SetupStartZTargetDefend2(Player* this, PlayState* play);
s32 Player_StartChangeItem(Player* this, PlayState* play);
s32 Player_StandingDefend(Player* this, PlayState* play);
s32 Player_EndDefend(Player* this, PlayState* play);
s32 Player_HoldFpsItem(Player* this, PlayState* play);
s32 Player_ReadyFpsItemToShoot(Player* this, PlayState* play);
s32 Player_AimFpsItem(Player* this, PlayState* play);
s32 Player_EndAimFpsItem(Player* this, PlayState* play);
s32 Player_HoldActor(Player* this, PlayState* play);
s32 Player_HoldBoomerang(Player* this, PlayState* play);
s32 Player_SetupAimBoomerang(Player* this, PlayState* play);
s32 Player_AimBoomerang(Player* this, PlayState* play);
s32 Player_ThrowBoomerang(Player* this, PlayState* play);
s32 Player_WaitForThrownBoomerang(Player* this, PlayState* play);
s32 Player_CatchBoomerang(Player* this, PlayState* play);
void Player_UseItem(PlayState* play, Player* this, s32 item);
void Player_SetupStandingStillType(Player* this, PlayState* play);
s32 Player_SetupWallJumpBehavior(Player* this, PlayState* play);
s32 Player_SetupOpenDoor(Player* this, PlayState* play);
s32 Player_SetupItemCutsceneOrFirstPerson(Player* this, PlayState* play);
s32 Player_SetupCUpBehavior(Player* this, PlayState* play);
s32 Player_SetupSpeakOrCheck(Player* this, PlayState* play);
s32 Player_SetupJumpSlashOrRoll(Player* this, PlayState* play);
s32 Player_SetupRollOrPutAway(Player* this, PlayState* play);
s32 Player_SetupDefend(Player* this, PlayState* play);
s32 Player_SetupStartChargeSpinAttack(Player* this, PlayState* play);
s32 Player_SetupThrowDekuNut(PlayState* play, Player* this);
void Player_SpawnNoMomentum(PlayState* play, Player* this);
void Player_SpawnWalkingSlow(PlayState* play, Player* this);
void Player_SpawnWalkingPreserveMomentum(PlayState* play, Player* this);
s32 Player_SetupMountHorse(Player* this, PlayState* play);
s32 Player_SetupGetItemOrHoldBehavior(Player* this, PlayState* play);
s32 Player_SetupPutDownOrThrowActor(Player* this, PlayState* play);
s32 Player_SetupSpecialWallInteraction(Player* this, PlayState* play);
void Player_UnfriendlyZTargetStandingStill(Player* this, PlayState* play);
void Player_FriendlyZTargetStandingStill(Player* this, PlayState* play);
void Player_StandingStill(Player* this, PlayState* play);
void Player_EndSidewalk(Player* this, PlayState* play);
void Player_FriendlyBackwalk(Player* this, PlayState* play);
void Player_HaltFriendlyBackwalk(Player* this, PlayState* play);
void Player_EndHaltFriendlyBackwalk(Player* this, PlayState* play);
void Player_Sidewalk(Player* this, PlayState* play);
void Player_Turn(Player* this, PlayState* play);
void Player_Run(Player* this, PlayState* play);
void Player_ZTargetingRun(Player* this, PlayState* play);
void func_8084279C(Player* this, PlayState* play);
void Player_UnfriendlyBackwalk(Player* this, PlayState* play);
void Player_EndUnfriendlyBackwalk(Player* this, PlayState* play);
void Player_AimShieldCrouched(Player* this, PlayState* play);
void Player_DeflectAttackWithShield(Player* this, PlayState* play);
void func_8084370C(Player* this, PlayState* play);
void Player_StartKnockback(Player* this, PlayState* play);
void Player_DownFromKnockback(Player* this, PlayState* play);
void Player_GetUpFromKnockback(Player* this, PlayState* play);
void Player_Die(Player* this, PlayState* play);
void Player_UpdateMidair(Player* this, PlayState* play);
void Player_Rolling(Player* this, PlayState* play);
void Player_FallingDive(Player* this, PlayState* play);
void Player_JumpSlash(Player* this, PlayState* play);
void Player_ChargeSpinAttack(Player* this, PlayState* play);
void Player_WalkChargingSpinAttack(Player* this, PlayState* play);
void Player_SidewalkChargingSpinAttack(Player* this, PlayState* play);
void Player_JumpUpToLedge(Player* this, PlayState* play);
void Player_RunMiniCutsceneFunc(Player* this, PlayState* play);
void Player_MiniCsMovement(Player* this, PlayState* play);
void Player_OpenDoor(Player* this, PlayState* play);
void Player_LiftActor(Player* this, PlayState* play);
void Player_ThrowStonePillar(Player* this, PlayState* play);
void Player_LiftSilverBoulder(Player* this, PlayState* play);
void Player_ThrowSilverBoulder(Player* this, PlayState* play);
void Player_FailToLiftActor(Player* this, PlayState* play);
void Player_SetupPutDownActor(Player* this, PlayState* play);
void Player_StartThrowActor(Player* this, PlayState* play);
void Player_SpawnNoUpdateOrDraw(PlayState* play, Player* this);
void Player_SetupSpawnFromBlueWarp(PlayState* play, Player* this);
void Player_SpawnFromTimeTravel(PlayState* play, Player* this);
void Player_SpawnOpeningDoor(PlayState* play, Player* this);
void Player_SpawnExitingGrotto(PlayState* play, Player* this);
void Player_SpawnWithKnockback(PlayState* play, Player* this);
void Player_SetupSpawnFromWarpSong(PlayState* play, Player* this);
void Player_SetupSpawnFromFaroresWind(PlayState* play, Player* this);
void Player_FirstPersonAiming(Player* this, PlayState* play);
void Player_TalkWithActor(Player* this, PlayState* play);
void Player_GrabPushPullWall(Player* this, PlayState* play);
void Player_PushWall(Player* this, PlayState* play);
void Player_PullWall(Player* this, PlayState* play);
void Player_GrabLedge(Player* this, PlayState* play);
void Player_ClimbOntoLedge(Player* this, PlayState* play);
void Player_ClimbingWallOrDownLedge(Player* this, PlayState* play);
void Player_UpdateCommon(Player* this, PlayState* play, Input* input);
void Player_EndClimb(Player* this, PlayState* play);
void Player_InsideCrawlspace(Player* this, PlayState* play);
void Player_ExitCrawlspace(Player* this, PlayState* play);
void Player_RideHorse(Player* this, PlayState* play);
void Player_DismountHorse(Player* this, PlayState* play);
void Player_UpdateSwimIdle(Player* this, PlayState* play);
void Player_SpawnSwimming(Player* this, PlayState* play);
void Player_Swim(Player* this, PlayState* play);
void Player_ZTargetSwimming(Player* this, PlayState* play);
void Player_Dive(Player* this, PlayState* play);
void Player_GetItemInWater(Player* this, PlayState* play);
void Player_DamagedSwim(Player* this, PlayState* play);
void Player_Drown(Player* this, PlayState* play);
void Player_PlayOcarina(Player* this, PlayState* play);
void Player_ThrowDekuNut(Player* this, PlayState* play);
void Player_GetItem(Player* this, PlayState* play);
void Player_EndTimeTravel(Player* this, PlayState* play);
void Player_DrinkFromBottle(Player* this, PlayState* play);
void Player_SwingBottle(Player* this, PlayState* play);
void Player_HealWithFairy(Player* this, PlayState* play);
void Player_DropItemFromBottle(Player* this, PlayState* play);
void Player_PresentExchangeItem(Player* this, PlayState* play);
void Player_SlipOnSlope(Player* this, PlayState* play);
void Player_SetDrawAndStartCutsceneAfterTimer(Player* this, PlayState* play);
void Player_SpawnFromWarpSong(Player* this, PlayState* play);
void Player_SpawnFromBlueWarp(Player* this, PlayState* play);
void Player_EnterGrotto(Player* this, PlayState* play);
void Player_SetupOpenDoorFromSpawn(Player* this, PlayState* play);
void Player_JumpFromGrotto(Player* this, PlayState* play);
void Player_ShootingGalleryPlay(Player* this, PlayState* play);
void Player_FrozenInIce(Player* this, PlayState* play);
void Player_SetupElectricShock(Player* this, PlayState* play);
s32 Player_CheckNoDebugModeCombo(Player* this, PlayState* play);
void Player_BowStringMoveAfterShot(Player* this);
void Player_BunnyHoodPhysics(Player* this);
s32 Player_SetupStartMeleeWeaponAttack(Player* this, PlayState* play);
void Player_MeleeWeaponAttack(Player* this, PlayState* play);
void Player_MeleeWeaponRebound(Player* this, PlayState* play);
void Player_ChooseFaroresWindOption(Player* this, PlayState* play);
void Player_SpawnFromFaroresWind(Player* this, PlayState* play);
void Player_UpdateMagicSpell(Player* this, PlayState* play);
void Player_MoveAlongHookshotPath(Player* this, PlayState* play);
void Player_CastFishingRod(Player* this, PlayState* play);
void Player_ReleaseCaughtFish(Player* this, PlayState* play);
void Player_AnimPlaybackType0(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType1(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType13(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType2(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType3(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType4(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType5(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType6(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType7(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType8(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType9(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType14(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType15(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType10(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType11(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType16(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType12(PlayState* play, Player* this, void* anim);
void Player_AnimPlaybackType17(PlayState* play, Player* this, void* animSfxEntry);
void Player_CutsceneSetupSwimIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSurfaceFromDive(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneTurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneWait(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneTurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupEnterWarp(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneEnterWarp(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupFightStance(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneFightStance(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneUnk3Update(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneUnk4Update(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupSwordPedestal(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSwordPedestal(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupWarpToSages(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneWarpToSages(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneKnockedToGround(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupStartPlayOcarina(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneDrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneCloseEyes(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneOpenEyes(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupGetItemInWater(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupSleeping(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSleeping(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupSleepingRestless(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneAwaken(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneGetOffBed(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupBlownBackward(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneBlownBackward(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneRaisedByWarp(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupIdle3(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneIdle3(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupStop(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetDraw(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneInspectGroundCarefully(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneStartPassOcarina(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneDrawSwordChild(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneTurnAroundSlowly(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneDesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneStepBackCautiously(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneInspectWeapon(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_SetupDoNothing4(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_DoNothing5(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetupGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneGanonKillCombo(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneEnd(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneSetPosAndYaw(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_CutsceneUnk6Update(PlayState* play, Player* this, CsCmdActorAction* linkCsAction);
void Player_StartCutscene(Player* this, PlayState* play);
s32 Player_IsDroppingFish(PlayState* play);
s32 Player_StartFishing(PlayState* play);
s32 Player_SetupRestrainedByEnemy(PlayState* play, Player* this);
s32 Player_SetupPlayerCutscene(PlayState* play, Actor* actor, s32 csMode);
void Player_SetupStandingStillMorph(Player* this, PlayState* play);
s32 Player_InflictDamageAndCheckForDeath(PlayState* play, s32 damage);
void Player_StartTalkingWithActor(PlayState* play, Actor* actor);

// .bss part 1
static s32 sPrevSkelAnimeMoveFlags;
static s32 sCurrentMask;
static Vec3f sWallIntersectPos;
static Input* sControlInput;

// .data

static u8 D_80853410[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

static PlayerAgeProperties sAgeProperties[] = {
    {
        56.0f,            // ageProperties->ceilingCheckHeight
        90.0f,            // ageProperties->unk_04
        1.0f,             // ageProperties->translationScale
        111.0f,           // ageProperties->unk_0C
        70.0f,            // ageProperties->unk_10
        79.4f,            // ageProperties->unk_14
        59.0f,            // ageProperties->unk_18
        41.0f,            // ageProperties->unk_1C
        19.0f,            // ageProperties->unk_20
        36.0f,            // ageProperties->unk_24
        44.8f,            // ageProperties->waterSurface
        56.0f,            // ageProperties->diveWaterSurface
        68.0f,            // ageProperties->unk_30
        70.0f,            // ageProperties->unk_34
        18.0f,            // ageProperties->wallCheckRadius
        15.0f,            // ageProperties->unk_3C
        70.0f,            // ageProperties->unk_40
        { 9, 4671, 359 }, // ageProperties->unk_44
        {
            { 8, 4694, 380 }, // ageProperties->unk_4A[0]
            { 9, 6122, 359 }, // ageProperties->unk_4A[1]
            { 8, 4694, 380 }, // ageProperties->unk_4A[2]
            { 9, 6122, 359 }, // ageProperties->unk_4A[3]
        },
        {
            { 9, 6122, 359 }, // ageProperties->unk_62[0]
            { 9, 7693, 380 }, // ageProperties->unk_62[1]
            { 9, 6122, 359 }, // ageProperties->unk_62[2]
            { 9, 7693, 380 }, // ageProperties->unk_62[3]
        },
        {
            { 8, 4694, 380 }, // ageProperties->unk_7A[0]
            { 9, 6122, 359 }, // ageProperties->unk_7A[1]
        },
        {
            { -1592, 4694, 380 }, // ageProperties->unk_86[0]
            { -1591, 6122, 359 }, // ageProperties->unk_86[1]
        },
        0,                                     // ageProperties->ageVoiceSfxOffset
        0x80,                                  // ageProperties->ageMoveSfxOffset
        &gPlayerAnim_link_demo_Tbox_open,      // ageProperties->chestOpenAnim
        &gPlayerAnim_link_demo_back_to_past,   // ageProperties->timeTravelStartAnim
        &gPlayerAnim_link_demo_return_to_past, // ageProperties->timeTravelEndAnim
        &gPlayerAnim_link_normal_climb_startA, // ageProperties->startClimb1Anim
        &gPlayerAnim_link_normal_climb_startB, // ageProperties->startClimb2Anim
        {
            &gPlayerAnim_link_normal_climb_upL,  // ageProperties->verticalClimbAnim[0]
            &gPlayerAnim_link_normal_climb_upR,  // ageProperties->verticalClimbAnim[1]
            &gPlayerAnim_link_normal_Fclimb_upL, // ageProperties->verticalClimbAnim[2]
            &gPlayerAnim_link_normal_Fclimb_upR, // ageProperties->verticalClimbAnim[3]
        },
        {
            &gPlayerAnim_link_normal_Fclimb_sideL, // ageProperties->horizontalClimbAnim[0]
            &gPlayerAnim_link_normal_Fclimb_sideR  // ageProperties->horizontalClimbAnim[1]
        },
        {
            &gPlayerAnim_link_normal_climb_endAL, // ageProperties->endClimb1Anim[0]
            &gPlayerAnim_link_normal_climb_endAR  // ageProperties->endClimb1Anim[1]
        },
        {
            &gPlayerAnim_link_normal_climb_endBR, // ageProperties->endClimb2Anim[0]
            &gPlayerAnim_link_normal_climb_endBL  // ageProperties->endClimb2Anim[1]
        },
    },
    {
        40.0f,                   // ageProperties->ceilingCheckHeight
        60.0f,                   // ageProperties->unk_04
        11.0f / 17.0f,           // ageProperties->translationScale
        71.0f,                   // ageProperties->unk_0C
        50.0f,                   // ageProperties->unk_10
        47.0f,                   // ageProperties->unk_14
        39.0f,                   // ageProperties->unk_18
        27.0f,                   // ageProperties->unk_1C
        19.0f,                   // ageProperties->unk_20
        22.0f,                   // ageProperties->unk_24
        29.6f,                   // ageProperties->waterSurface
        32.0f,                   // ageProperties->diveWaterSurface
        48.0f,                   // ageProperties->unk_30
        70.0f * (11.0f / 17.0f), // ageProperties->unk_34
        14.0f,                   // ageProperties->wallCheckRadius
        12.0f,                   // ageProperties->unk_3C
        55.0f,                   // ageProperties->unk_40
        { -24, 3565, 876 },      // ageProperties->unk_44
        {
            { -24, 3474, 862 }, // ageProperties->unk_4A[0]
            { -24, 4977, 937 }, // ageProperties->unk_4A[1]
            { 8, 4694, 380 },   // ageProperties->unk_4A[2]
            { 9, 6122, 359 },   // ageProperties->unk_4A[3]
        },
        {
            { -24, 4977, 937 }, // ageProperties->unk_62[0]
            { -24, 6495, 937 }, // ageProperties->unk_62[1]
            { 9, 6122, 359 },   // ageProperties->unk_62[2]
            { 9, 7693, 380 },   // ageProperties->unk_62[3]
        },
        {
            { 8, 4694, 380 }, // ageProperties->unk_7A[0]
            { 9, 6122, 359 }, // ageProperties->unk_7A[1]
        },
        {
            { -1592, 4694, 380 }, // ageProperties->unk_86[0]
            { -1591, 6122, 359 }, // ageProperties->unk_86[1]
        },
        0x20,                                     // ageProperties->ageVoiceSfxOffset
        0,                                        // ageProperties->ageMoveSfxOffset
        &gPlayerAnim_clink_demo_Tbox_open,        // ageProperties->chestOpenAnim
        &gPlayerAnim_clink_demo_goto_future,      // ageProperties->timeTravelStartAnim
        &gPlayerAnim_clink_demo_return_to_future, // ageProperties->timeTravelEndAnim
        &gPlayerAnim_clink_normal_climb_startA,   // ageProperties->startClimb1Anim
        &gPlayerAnim_clink_normal_climb_startB,   // ageProperties->startClimb2Anim
        {
            &gPlayerAnim_clink_normal_climb_upL, // ageProperties->verticalClimbAnim[0]
            &gPlayerAnim_clink_normal_climb_upR, // ageProperties->verticalClimbAnim[1]
            &gPlayerAnim_link_normal_Fclimb_upL, // ageProperties->verticalClimbAnim[2]
            &gPlayerAnim_link_normal_Fclimb_upR  // ageProperties->verticalClimbAnim[3]
        },
        {
            &gPlayerAnim_link_normal_Fclimb_sideL, // ageProperties->horizontalClimbAnim[0]
            &gPlayerAnim_link_normal_Fclimb_sideR  // ageProperties->horizontalClimbAnim[1]
        },
        {
            &gPlayerAnim_clink_normal_climb_endAL, // ageProperties->endClimb1Anim[0]
            &gPlayerAnim_clink_normal_climb_endAR  // ageProperties->endClimb1Anim[1]
        },
        {
            &gPlayerAnim_clink_normal_climb_endBR, // ageProperties->endClimb2Anim[0]
            &gPlayerAnim_clink_normal_climb_endBL  // ageProperties->endClimb2Anim[1]
        },
    },
};

static u32 sDebugModeFlag = false;
static f32 sAnalogStickDistance = 0.0f;
static s16 sAnalogStickAngle = 0;
static s16 sCameraOffsetAnalogStickAngle = 0;
static s32 D_808535E0 = 0;
static s32 sFloorType = FLOOR_TYPE_NONE;
static f32 sWaterSpeedScale = 1.0f;
static f32 sInvertedWaterSpeedScale = 1.0f;
static u32 sInteractWallFlags = 0;
static u32 sConveyorSpeedIndex = CONVEYOR_SPEED_DISABLED;
static s16 sIsFloorConveyor = false;
static s16 sConveyorYaw = 0;
static f32 sPlayerYDistToFloor = 0.0f;
static s32 sFloorProperty = FLOOR_PROPERTY_NONE;
static s32 sYawToTouchedWall = 0;
static s32 sYawToTouchedWall2 = 0;
static s16 sAngleToFloorX = 0;
static s32 sUsingItemAlreadyInHand = 0;
static s32 sUsingItemAlreadyInHand2 = 0;

static u16 sInterruptableSfx[] = {
    NA_SE_VO_LI_SWEAT,
    NA_SE_VO_LI_SNEEZE,
    NA_SE_VO_LI_RELAX,
    NA_SE_VO_LI_FALL_L,
};

static GetItemEntry sGetItemTable[] = {
    // GI_BOMBS_5
    GET_ITEM(ITEM_BOMBS_5, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_DEKU_NUTS_5
    GET_ITEM(ITEM_DEKU_NUTS_5, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    // GI_BOMBCHUS_10
    GET_ITEM(ITEM_BOMBCHU, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    // GI_BOW
    GET_ITEM(ITEM_BOW, OBJECT_GI_BOW, GID_BOW, 0x31, 0x80, CHEST_ANIM_LONG),
    // GI_SLINGSHOT
    GET_ITEM(ITEM_SLINGSHOT, OBJECT_GI_PACHINKO, GID_SLINGSHOT, 0x30, 0x80, CHEST_ANIM_LONG),
    // GI_BOOMERANG
    GET_ITEM(ITEM_BOOMERANG, OBJECT_GI_BOOMERANG, GID_BOOMERANG, 0x35, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_STICKS_1
    GET_ITEM(ITEM_DEKU_STICK, OBJECT_GI_STICK, GID_DEKU_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    // GI_HOOKSHOT
    GET_ITEM(ITEM_HOOKSHOT, OBJECT_GI_HOOKSHOT, GID_HOOKSHOT, 0x36, 0x80, CHEST_ANIM_LONG),
    // GI_LONGSHOT
    GET_ITEM(ITEM_LONGSHOT, OBJECT_GI_HOOKSHOT, GID_LONGSHOT, 0x4F, 0x80, CHEST_ANIM_LONG),
    // GI_LENS_OF_TRUTH
    GET_ITEM(ITEM_LENS_OF_TRUTH, OBJECT_GI_GLASSES, GID_LENS_OF_TRUTH, 0x39, 0x80, CHEST_ANIM_LONG),
    // GI_ZELDAS_LETTER
    GET_ITEM(ITEM_ZELDAS_LETTER, OBJECT_GI_LETTER, GID_ZELDAS_LETTER, 0x69, 0x80, CHEST_ANIM_LONG),
    // GI_OCARINA_OF_TIME
    GET_ITEM(ITEM_OCARINA_OF_TIME, OBJECT_GI_OCARINA, GID_OCARINA_OF_TIME, 0x3A, 0x80, CHEST_ANIM_LONG),
    // GI_HAMMER
    GET_ITEM(ITEM_HAMMER, OBJECT_GI_HAMMER, GID_HAMMER, 0x38, 0x80, CHEST_ANIM_LONG),
    // GI_COJIRO
    GET_ITEM(ITEM_COJIRO, OBJECT_GI_NIWATORI, GID_COJIRO, 0x02, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_EMPTY
    GET_ITEM(ITEM_BOTTLE_EMPTY, OBJECT_GI_BOTTLE, GID_BOTTLE_EMPTY, 0x42, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POTION_RED
    GET_ITEM(ITEM_BOTTLE_POTION_RED, OBJECT_GI_LIQUID, GID_BOTTLE_POTION_RED, 0x43, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POTION_GREEN
    GET_ITEM(ITEM_BOTTLE_POTION_GREEN, OBJECT_GI_LIQUID, GID_BOTTLE_POTION_GREEN, 0x44, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POTION_BLUE
    GET_ITEM(ITEM_BOTTLE_POTION_BLUE, OBJECT_GI_LIQUID, GID_BOTTLE_POTION_BLUE, 0x45, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_FAIRY
    GET_ITEM(ITEM_BOTTLE_FAIRY, OBJECT_GI_BOTTLE, GID_BOTTLE_EMPTY, 0x46, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_MILK_FULL
    GET_ITEM(ITEM_BOTTLE_MILK_FULL, OBJECT_GI_MILK, GID_BOTTLE_MILK_FULL, 0x98, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_RUTOS_LETTER
    GET_ITEM(ITEM_BOTTLE_RUTOS_LETTER, OBJECT_GI_BOTTLE_LETTER, GID_BOTTLE_RUTOS_LETTER, 0x99, 0x80, CHEST_ANIM_LONG),
    // GI_MAGIC_BEAN
    GET_ITEM(ITEM_MAGIC_BEAN, OBJECT_GI_BEAN, GID_MAGIC_BEAN, 0x48, 0x80, CHEST_ANIM_SHORT),
    // GI_MASK_SKULL
    GET_ITEM(ITEM_MASK_SKULL, OBJECT_GI_SKJ_MASK, GID_MASK_SKULL, 0x10, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_SPOOKY
    GET_ITEM(ITEM_MASK_SPOOKY, OBJECT_GI_REDEAD_MASK, GID_MASK_SPOOKY, 0x11, 0x80, CHEST_ANIM_LONG),
    // GI_CHICKEN
    GET_ITEM(ITEM_CHICKEN, OBJECT_GI_NIWATORI, GID_CUCCO, 0x48, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_KEATON
    GET_ITEM(ITEM_MASK_KEATON, OBJECT_GI_KI_TAN_MASK, GID_MASK_KEATON, 0x12, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_BUNNY_HOOD
    GET_ITEM(ITEM_MASK_BUNNY_HOOD, OBJECT_GI_RABIT_MASK, GID_MASK_BUNNY_HOOD, 0x13, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_TRUTH
    GET_ITEM(ITEM_MASK_TRUTH, OBJECT_GI_TRUTH_MASK, GID_MASK_TRUTH, 0x17, 0x80, CHEST_ANIM_LONG),
    // GI_POCKET_EGG
    GET_ITEM(ITEM_POCKET_EGG, OBJECT_GI_EGG, GID_EGG, 0x01, 0x80, CHEST_ANIM_LONG),
    // GI_POCKET_CUCCO
    GET_ITEM(ITEM_POCKET_CUCCO, OBJECT_GI_NIWATORI, GID_CUCCO, 0x48, 0x80, CHEST_ANIM_LONG),
    // GI_ODD_MUSHROOM
    GET_ITEM(ITEM_ODD_MUSHROOM, OBJECT_GI_MUSHROOM, GID_ODD_MUSHROOM, 0x03, 0x80, CHEST_ANIM_LONG),
    // GI_ODD_POTION
    GET_ITEM(ITEM_ODD_POTION, OBJECT_GI_POWDER, GID_ODD_POTION, 0x04, 0x80, CHEST_ANIM_LONG),
    // GI_POACHERS_SAW
    GET_ITEM(ITEM_POACHERS_SAW, OBJECT_GI_SAW, GID_POACHERS_SAW, 0x05, 0x80, CHEST_ANIM_LONG),
    // GI_BROKEN_GORONS_SWORD
    GET_ITEM(ITEM_BROKEN_GORONS_SWORD, OBJECT_GI_BROKENSWORD, GID_BROKEN_GORONS_SWORD, 0x08, 0x80, CHEST_ANIM_LONG),
    // GI_PRESCRIPTION
    GET_ITEM(ITEM_PRESCRIPTION, OBJECT_GI_PRESCRIPTION, GID_PRESCRIPTION, 0x09, 0x80, CHEST_ANIM_LONG),
    // GI_EYEBALL_FROG
    GET_ITEM(ITEM_EYEBALL_FROG, OBJECT_GI_FROG, GID_EYEBALL_FROG, 0x0D, 0x80, CHEST_ANIM_LONG),
    // GI_EYE_DROPS
    GET_ITEM(ITEM_EYE_DROPS, OBJECT_GI_EYE_LOTION, GID_EYE_DROPS, 0x0E, 0x80, CHEST_ANIM_LONG),
    // GI_CLAIM_CHECK
    GET_ITEM(ITEM_CLAIM_CHECK, OBJECT_GI_TICKETSTONE, GID_CLAIM_CHECK, 0x0A, 0x80, CHEST_ANIM_LONG),
    // GI_SWORD_KOKIRI
    GET_ITEM(ITEM_SWORD_KOKIRI, OBJECT_GI_SWORD_1, GID_SWORD_KOKIRI, 0xA4, 0x80, CHEST_ANIM_LONG),
    // GI_SWORD_KNIFE
    GET_ITEM(ITEM_SWORD_BIGGORON, OBJECT_GI_LONGSWORD, GID_SWORD_BIGGORON, 0x4B, 0x80, CHEST_ANIM_LONG),
    // GI_SHIELD_DEKU
    GET_ITEM(ITEM_SHIELD_DEKU, OBJECT_GI_SHIELD_1, GID_SHIELD_DEKU, 0x4C, 0xA0, CHEST_ANIM_SHORT),
    // GI_SHIELD_HYLIAN
    GET_ITEM(ITEM_SHIELD_HYLIAN, OBJECT_GI_SHIELD_2, GID_SHIELD_HYLIAN, 0x4D, 0xA0, CHEST_ANIM_SHORT),
    // GI_SHIELD_MIRROR
    GET_ITEM(ITEM_SHIELD_MIRROR, OBJECT_GI_SHIELD_3, GID_SHIELD_MIRROR, 0x4E, 0x80, CHEST_ANIM_LONG),
    // GI_TUNIC_GORON
    GET_ITEM(ITEM_TUNIC_GORON, OBJECT_GI_CLOTHES, GID_TUNIC_GORON, 0x50, 0xA0, CHEST_ANIM_LONG),
    // GI_TUNIC_ZORA
    GET_ITEM(ITEM_TUNIC_ZORA, OBJECT_GI_CLOTHES, GID_TUNIC_ZORA, 0x51, 0xA0, CHEST_ANIM_LONG),
    // GI_BOOTS_IRON
    GET_ITEM(ITEM_BOOTS_IRON, OBJECT_GI_BOOTS_2, GID_BOOTS_IRON, 0x53, 0x80, CHEST_ANIM_LONG),
    // GI_BOOTS_HOVER
    GET_ITEM(ITEM_BOOTS_HOVER, OBJECT_GI_HOVERBOOTS, GID_BOOTS_HOVER, 0x54, 0x80, CHEST_ANIM_LONG),
    // GI_QUIVER_40
    GET_ITEM(ITEM_QUIVER_40, OBJECT_GI_ARROWCASE, GID_QUIVER_40, 0x56, 0x80, CHEST_ANIM_LONG),
    // GI_QUIVER_50
    GET_ITEM(ITEM_QUIVER_50, OBJECT_GI_ARROWCASE, GID_QUIVER_50, 0x57, 0x80, CHEST_ANIM_LONG),
    // GI_BOMB_BAG_20
    GET_ITEM(ITEM_BOMB_BAG_20, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_20, 0x58, 0x80, CHEST_ANIM_LONG),
    // GI_BOMB_BAG_30
    GET_ITEM(ITEM_BOMB_BAG_30, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_30, 0x59, 0x80, CHEST_ANIM_LONG),
    // GI_BOMB_BAG_40
    GET_ITEM(ITEM_BOMB_BAG_40, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_40, 0x5A, 0x80, CHEST_ANIM_LONG),
    // GI_SILVER_GAUNTLETS
    GET_ITEM(ITEM_STRENGTH_SILVER_GAUNTLETS, OBJECT_GI_GLOVES, GID_SILVER_GAUNTLETS, 0x5B, 0x80, CHEST_ANIM_LONG),
    // GI_GOLD_GAUNTLETS
    GET_ITEM(ITEM_STRENGTH_GOLD_GAUNTLETS, OBJECT_GI_GLOVES, GID_GOLD_GAUNTLETS, 0x5C, 0x80, CHEST_ANIM_LONG),
    // GI_SCALE_SILVER
    GET_ITEM(ITEM_SCALE_SILVER, OBJECT_GI_SCALE, GID_SCALE_SILVER, 0xCD, 0x80, CHEST_ANIM_LONG),
    // GI_SCALE_GOLDEN
    GET_ITEM(ITEM_SCALE_GOLDEN, OBJECT_GI_SCALE, GID_SCALE_GOLDEN, 0xCE, 0x80, CHEST_ANIM_LONG),
    // GI_STONE_OF_AGONY
    GET_ITEM(ITEM_STONE_OF_AGONY, OBJECT_GI_MAP, GID_STONE_OF_AGONY, 0x68, 0x80, CHEST_ANIM_LONG),
    // GI_GERUDOS_CARD
    GET_ITEM(ITEM_GERUDOS_CARD, OBJECT_GI_GERUDO, GID_GERUDOS_CARD, 0x7B, 0x80, CHEST_ANIM_LONG),
    // GI_OCARINA_FAIRY
    GET_ITEM(ITEM_OCARINA_FAIRY, OBJECT_GI_OCARINA_0, GID_OCARINA_FAIRY, 0x3A, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_SEEDS_5
    GET_ITEM(ITEM_DEKU_SEEDS, OBJECT_GI_SEED, GID_DEKU_SEEDS, 0xDC, 0x50, CHEST_ANIM_SHORT),
    // GI_HEART_CONTAINER
    GET_ITEM(ITEM_HEART_CONTAINER, OBJECT_GI_HEARTS, GID_HEART_CONTAINER, 0xC6, 0x80, CHEST_ANIM_LONG),
    // GI_HEART_PIECE
    GET_ITEM(ITEM_HEART_PIECE_2, OBJECT_GI_HEARTS, GID_HEART_PIECE, 0xC2, 0x80, CHEST_ANIM_LONG),
    // GI_BOSS_KEY
    GET_ITEM(ITEM_DUNGEON_BOSS_KEY, OBJECT_GI_BOSSKEY, GID_BOSS_KEY, 0xC7, 0x80, CHEST_ANIM_LONG),
    // GI_COMPASS
    GET_ITEM(ITEM_DUNGEON_COMPASS, OBJECT_GI_COMPASS, GID_COMPASS, 0x67, 0x80, CHEST_ANIM_LONG),
    // GI_DUNGEON_MAP
    GET_ITEM(ITEM_DUNGEON_MAP, OBJECT_GI_MAP, GID_DUNGEON_MAP, 0x66, 0x80, CHEST_ANIM_LONG),
    // GI_SMALL_KEY
    GET_ITEM(ITEM_SMALL_KEY, OBJECT_GI_KEY, GID_SMALL_KEY, 0x60, 0x80, CHEST_ANIM_SHORT),
    // GI_MAGIC_JAR_SMALL
    GET_ITEM(ITEM_MAGIC_JAR_SMALL, OBJECT_GI_MAGICPOT, GID_MAGIC_JAR_SMALL, 0x52, 0x6F, CHEST_ANIM_SHORT),
    // GI_MAGIC_JAR_LARGE
    GET_ITEM(ITEM_MAGIC_JAR_BIG, OBJECT_GI_MAGICPOT, GID_MAGIC_JAR_LARGE, 0x52, 0x6E, CHEST_ANIM_SHORT),
    // GI_WALLET_ADULT
    GET_ITEM(ITEM_ADULTS_WALLET, OBJECT_GI_PURSE, GID_WALLET_ADULT, 0x5E, 0x80, CHEST_ANIM_LONG),
    // GI_WALLET_GIANT
    GET_ITEM(ITEM_GIANTS_WALLET, OBJECT_GI_PURSE, GID_WALLET_GIANT, 0x5F, 0x80, CHEST_ANIM_LONG),
    // GI_WEIRD_EGG
    GET_ITEM(ITEM_WEIRD_EGG, OBJECT_GI_EGG, GID_EGG, 0x9A, 0x80, CHEST_ANIM_LONG),
    // GI_RECOVERY_HEART
    GET_ITEM(ITEM_RECOVERY_HEART, OBJECT_GI_HEART, GID_RECOVERY_HEART, 0x55, 0x80, CHEST_ANIM_LONG),
    // GI_ARROWS_5
    GET_ITEM(ITEM_ARROWS_5, OBJECT_GI_ARROW, GID_ARROWS_5, 0xE6, 0x48, CHEST_ANIM_SHORT),
    // GI_ARROWS_10
    GET_ITEM(ITEM_ARROWS_10, OBJECT_GI_ARROW, GID_ARROWS_10, 0xE6, 0x49, CHEST_ANIM_SHORT),
    // GI_ARROWS_30
    GET_ITEM(ITEM_ARROWS_30, OBJECT_GI_ARROW, GID_ARROWS_30, 0xE6, 0x4A, CHEST_ANIM_SHORT),
    // GI_RUPEE_GREEN
    GET_ITEM(ITEM_RUPEE_GREEN, OBJECT_GI_RUPY, GID_RUPEE_GREEN, 0x6F, 0x00, CHEST_ANIM_SHORT),
    // GI_RUPEE_BLUE
    GET_ITEM(ITEM_RUPEE_BLUE, OBJECT_GI_RUPY, GID_RUPEE_BLUE, 0xCC, 0x01, CHEST_ANIM_SHORT),
    // GI_RUPEE_RED
    GET_ITEM(ITEM_RUPEE_RED, OBJECT_GI_RUPY, GID_RUPEE_RED, 0xF0, 0x02, CHEST_ANIM_SHORT),
    // GI_HEART_CONTAINER_2
    GET_ITEM(ITEM_HEART_CONTAINER, OBJECT_GI_HEARTS, GID_HEART_CONTAINER, 0xC6, 0x80, CHEST_ANIM_LONG),
    // GI_MILK
    GET_ITEM(ITEM_MILK, OBJECT_GI_MILK, GID_BOTTLE_MILK_FULL, 0x98, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_GORON
    GET_ITEM(ITEM_MASK_GORON, OBJECT_GI_GOLONMASK, GID_MASK_GORON, 0x14, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_ZORA
    GET_ITEM(ITEM_MASK_ZORA, OBJECT_GI_ZORAMASK, GID_MASK_ZORA, 0x15, 0x80, CHEST_ANIM_LONG),
    // GI_MASK_GERUDO
    GET_ITEM(ITEM_MASK_GERUDO, OBJECT_GI_GERUDOMASK, GID_MASK_GERUDO, 0x16, 0x80, CHEST_ANIM_LONG),
    // GI_GORONS_BRACELET
    GET_ITEM(ITEM_STRENGTH_GORONS_BRACELET, OBJECT_GI_BRACELET, GID_GORONS_BRACELET, 0x79, 0x80, CHEST_ANIM_LONG),
    // GI_RUPEE_PURPLE
    GET_ITEM(ITEM_RUPEE_PURPLE, OBJECT_GI_RUPY, GID_RUPEE_PURPLE, 0xF1, 0x14, CHEST_ANIM_SHORT),
    // GI_RUPEE_GOLD
    GET_ITEM(ITEM_RUPEE_GOLD, OBJECT_GI_RUPY, GID_RUPEE_GOLD, 0xF2, 0x13, CHEST_ANIM_SHORT),
    // GI_SWORD_BIGGORON
    GET_ITEM(ITEM_SWORD_BIGGORON, OBJECT_GI_LONGSWORD, GID_SWORD_BIGGORON, 0x0C, 0x80, CHEST_ANIM_LONG),
    // GI_ARROW_FIRE
    GET_ITEM(ITEM_ARROW_FIRE, OBJECT_GI_M_ARROW, GID_ARROW_FIRE, 0x70, 0x80, CHEST_ANIM_LONG),
    // GI_ARROW_ICE
    GET_ITEM(ITEM_ARROW_ICE, OBJECT_GI_M_ARROW, GID_ARROW_ICE, 0x71, 0x80, CHEST_ANIM_LONG),
    // GI_ARROW_LIGHT
    GET_ITEM(ITEM_ARROW_LIGHT, OBJECT_GI_M_ARROW, GID_ARROW_LIGHT, 0x72, 0x80, CHEST_ANIM_LONG),
    // GI_SKULL_TOKEN
    GET_ITEM(ITEM_SKULL_TOKEN, OBJECT_GI_SUTARU, GID_SKULL_TOKEN, 0xB4, 0x80, CHEST_ANIM_SHORT),
    // GI_DINS_FIRE
    GET_ITEM(ITEM_DINS_FIRE, OBJECT_GI_GODDESS, GID_DINS_FIRE, 0xAD, 0x80, CHEST_ANIM_LONG),
    // GI_FARORES_WIND
    GET_ITEM(ITEM_FARORES_WIND, OBJECT_GI_GODDESS, GID_FARORES_WIND, 0xAE, 0x80, CHEST_ANIM_LONG),
    // GI_NAYRUS_LOVE
    GET_ITEM(ITEM_NAYRUS_LOVE, OBJECT_GI_GODDESS, GID_NAYRUS_LOVE, 0xAF, 0x80, CHEST_ANIM_LONG),
    // GI_BULLET_BAG_30
    GET_ITEM(ITEM_BULLET_BAG_30, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG, 0x07, 0x80, CHEST_ANIM_LONG),
    // GI_BULLET_BAG_40
    GET_ITEM(ITEM_BULLET_BAG_40, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG, 0x07, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_STICKS_5
    GET_ITEM(ITEM_DEKU_STICKS_5, OBJECT_GI_STICK, GID_DEKU_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    // GI_DEKU_STICKS_10
    GET_ITEM(ITEM_DEKU_STICKS_10, OBJECT_GI_STICK, GID_DEKU_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    // GI_DEKU_NUTS_5_2
    GET_ITEM(ITEM_DEKU_NUTS_5, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    // GI_DEKU_NUTS_10
    GET_ITEM(ITEM_DEKU_NUTS_10, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    // GI_BOMBS_1
    GET_ITEM(ITEM_BOMB, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_BOMBS_10
    GET_ITEM(ITEM_BOMBS_10, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_BOMBS_20
    GET_ITEM(ITEM_BOMBS_20, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_BOMBS_30
    GET_ITEM(ITEM_BOMBS_30, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    // GI_DEKU_SEEDS_30
    GET_ITEM(ITEM_DEKU_SEEDS_30, OBJECT_GI_SEED, GID_DEKU_SEEDS, 0xDC, 0x50, CHEST_ANIM_SHORT),
    // GI_BOMBCHUS_5
    GET_ITEM(ITEM_BOMBCHUS_5, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    // GI_BOMBCHUS_20
    GET_ITEM(ITEM_BOMBCHUS_20, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    // GI_BOTTLE_FISH
    GET_ITEM(ITEM_BOTTLE_FISH, OBJECT_GI_FISH, GID_FISH, 0x47, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_BUGS
    GET_ITEM(ITEM_BOTTLE_BUG, OBJECT_GI_INSECT, GID_BUG, 0x7A, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_BLUE_FIRE
    GET_ITEM(ITEM_BOTTLE_BLUE_FIRE, OBJECT_GI_FIRE, GID_BLUE_FIRE, 0x5D, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_POE
    GET_ITEM(ITEM_BOTTLE_POE, OBJECT_GI_GHOST, GID_POE, 0x97, 0x80, CHEST_ANIM_LONG),
    // GI_BOTTLE_BIG_POE
    GET_ITEM(ITEM_BOTTLE_BIG_POE, OBJECT_GI_GHOST, GID_BIG_POE, 0xF9, 0x80, CHEST_ANIM_LONG),
    // GI_DOOR_KEY
    GET_ITEM(ITEM_SMALL_KEY, OBJECT_GI_KEY, GID_SMALL_KEY, 0xF3, 0x80, CHEST_ANIM_SHORT),
    // GI_RUPEE_GREEN_LOSE
    GET_ITEM(ITEM_RUPEE_GREEN, OBJECT_GI_RUPY, GID_RUPEE_GREEN, 0xF4, 0x00, CHEST_ANIM_SHORT),
    // GI_RUPEE_BLUE_LOSE
    GET_ITEM(ITEM_RUPEE_BLUE, OBJECT_GI_RUPY, GID_RUPEE_BLUE, 0xF5, 0x01, CHEST_ANIM_SHORT),
    // GI_RUPEE_RED_LOSE
    GET_ITEM(ITEM_RUPEE_RED, OBJECT_GI_RUPY, GID_RUPEE_RED, 0xF6, 0x02, CHEST_ANIM_SHORT),
    // GI_RUPEE_PURPLE_LOSE
    GET_ITEM(ITEM_RUPEE_PURPLE, OBJECT_GI_RUPY, GID_RUPEE_PURPLE, 0xF7, 0x14, CHEST_ANIM_SHORT),
    // GI_HEART_PIECE_WIN
    GET_ITEM(ITEM_HEART_PIECE_2, OBJECT_GI_HEARTS, GID_HEART_PIECE, 0xFA, 0x80, CHEST_ANIM_LONG),
    // GI_DEKU_STICK_UPGRADE_20
    GET_ITEM(ITEM_DEKU_STICK_UPGRADE_20, OBJECT_GI_STICK, GID_DEKU_STICK, 0x90, 0x80, CHEST_ANIM_SHORT),
    // GI_DEKU_STICK_UPGRADE_30
    GET_ITEM(ITEM_DEKU_STICK_UPGRADE_30, OBJECT_GI_STICK, GID_DEKU_STICK, 0x91, 0x80, CHEST_ANIM_SHORT),
    // GI_DEKU_NUT_UPGRADE_30
    GET_ITEM(ITEM_DEKU_NUT_UPGRADE_30, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0xA7, 0x80, CHEST_ANIM_SHORT),
    // GI_DEKU_NUT_UPGRADE_40
    GET_ITEM(ITEM_DEKU_NUT_UPGRADE_40, OBJECT_GI_NUTS, GID_DEKU_NUTS, 0xA8, 0x80, CHEST_ANIM_SHORT),
    // GI_BULLET_BAG_50
    GET_ITEM(ITEM_BULLET_BAG_50, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG_50, 0x6C, 0x80, CHEST_ANIM_LONG),
    // GI_ICE_TRAP
    GET_ITEM_NONE,
    // GI_TEXT_0
    GET_ITEM_NONE,
};

#define GET_PLAYER_ANIM(group, type) sPlayerAnimations[group * PLAYER_ANIMTYPE_MAX + type]

static LinkAnimationHeader* sPlayerAnimations[PLAYER_ANIMGROUP_MAX * PLAYER_ANIMTYPE_MAX] = {
    /* PLAYER_ANIMGROUP_wait */
    &gPlayerAnim_link_normal_wait_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_wait,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_wait,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_wait_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_wait_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_wait_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_walk */
    &gPlayerAnim_link_normal_walk_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_walk,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_walk,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_walk_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_walk_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_run */
    &gPlayerAnim_link_normal_run_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_fighter_run,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_run,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_run_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_run_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_run_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_damage_run */
    &gPlayerAnim_link_normal_damage_run_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_fighter_damage_run,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_damage_run_free,  // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_damage_run_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_damage_run_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_damage_run_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_heavy_run */
    &gPlayerAnim_link_normal_heavy_run_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_heavy_run,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_heavy_run_free,  // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_heavy_run_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_heavy_run_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_heavy_run_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_waitL */
    &gPlayerAnim_link_normal_waitL_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_waitL,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_waitL,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitL_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_waitL_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_waitL_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_waitR */
    &gPlayerAnim_link_normal_waitR_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_waitR,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_waitR,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_waitR_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_waitR_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_wait2waitR */
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_wait2waitR,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_wait2waitR,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_normal2fighter */
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_fighter_normal2fighter,     // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_fighter_normal2fighter,     // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_doorA_free */
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_demo_doorA_link,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_demo_doorA_link,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_doorA */
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_doorB_free */
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_demo_doorB_link,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_demo_doorB_link,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_doorB */
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_carryB */
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_carryB,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_carryB,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_landing */
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_landing,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_landing,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_short_landing */
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_short_landing,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_short_landing,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_landing_roll */
    &gPlayerAnim_link_normal_landing_roll_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_landing_roll,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_landing_roll,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_landing_roll_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_landing_roll_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_landing_roll_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_hip_down */
    &gPlayerAnim_link_normal_hip_down_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_hip_down,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_hip_down,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_hip_down_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_hip_down_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_hip_down_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_walk_endL */
    &gPlayerAnim_link_normal_walk_endL_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_walk_endL,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endL,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_walk_endL_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endL_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_walk_endL_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_walk_endR */
    &gPlayerAnim_link_normal_walk_endR_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_walk_endR,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endR,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_walk_endR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endR_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_walk_endR_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_defense */
    &gPlayerAnim_link_normal_defense_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_defense,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_defense,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_defense_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_bow_defense,         // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_defense_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_defense_wait */
    &gPlayerAnim_link_normal_defense_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_defense_wait,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_defense_wait,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_defense_wait_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_bow_defense_wait,         // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_defense_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_defense_end */
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_defense_end,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_defense_end,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_side_walk */
    &gPlayerAnim_link_normal_side_walk_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_side_walk,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walk,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_side_walk_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walk_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_side_walk_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_side_walkL */
    &gPlayerAnim_link_normal_side_walkL_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_side_walkL,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_side_walkL,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_side_walkL_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walkL_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_side_walkL_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_side_walkR */
    &gPlayerAnim_link_normal_side_walkR_free,  // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_side_walkR,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_side_walkR,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_side_walkR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walkR_free,  // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_side_walkR_free,  // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_45_turn */
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_45_turn,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_45_turn,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_waitL2wait */
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_waitL2wait,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_waitL2wait,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_waitR2wait */
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_waitR2wait,       // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_waitR2wait,       // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_throw */
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_throw,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_throw,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_put */
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_put,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_put,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_back_walk */
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_check */
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_check,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_check,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_check_wait */
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_check_wait,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_check_wait,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_check_end */
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_check_end,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_check_end,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_pull_start */
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_pull_start,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_start,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_pulling */
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_pulling,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_pulling,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_pull_end */
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_pull_end,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_end,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_fall_up */
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_fall_up,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_fall_up,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_jump_climb_hold */
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_jump_climb_hold,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_hold,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_jump_climb_wait */
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_jump_climb_wait,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_wait,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_jump_climb_up */
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_jump_climb_up,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_up,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_down_slope_slip_end */
    &gPlayerAnim_link_normal_down_slope_slip_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_down_slope_slip_end,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_down_slope_slip_end,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_down_slope_slip_end_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_down_slope_slip_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_down_slope_slip_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_up_slope_slip_end */
    &gPlayerAnim_link_normal_up_slope_slip_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_up_slope_slip_end,      // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_up_slope_slip_end,      // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_up_slope_slip_end_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_up_slope_slip_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_up_slope_slip_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE,
    /* PLAYER_ANIMGROUP_nwait */
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_lkt_nwait,  // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_lkt_nwait,  // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
};

static LinkAnimationHeader* sManualJumpAnims[][3] = {
    { &gPlayerAnim_link_fighter_front_jump, &gPlayerAnim_link_fighter_front_jump_end,
      &gPlayerAnim_link_fighter_front_jump_endR },
    { &gPlayerAnim_link_fighter_Lside_jump, &gPlayerAnim_link_fighter_Lside_jump_end,
      &gPlayerAnim_link_fighter_Lside_jump_endL },
    { &gPlayerAnim_link_fighter_backturn_jump, &gPlayerAnim_link_fighter_backturn_jump_end,
      &gPlayerAnim_link_fighter_backturn_jump_endR },
    { &gPlayerAnim_link_fighter_Rside_jump, &gPlayerAnim_link_fighter_Rside_jump_end,
      &gPlayerAnim_link_fighter_Rside_jump_endR },
};

// First anim when holding one-handed sword, second anim all other times
static LinkAnimationHeader* sIdleAnims[][2] = {
    { &gPlayerAnim_link_normal_wait_typeA_20f, &gPlayerAnim_link_normal_waitF_typeA_20f },
    { &gPlayerAnim_link_normal_wait_typeC_20f, &gPlayerAnim_link_normal_waitF_typeC_20f },
    { &gPlayerAnim_link_normal_wait_typeB_20f, &gPlayerAnim_link_normal_waitF_typeB_20f },
    { &gPlayerAnim_link_normal_wait_typeB_20f, &gPlayerAnim_link_normal_waitF_typeB_20f },
    { &gPlayerAnim_link_wait_typeD_20f, &gPlayerAnim_link_waitF_typeD_20f },
    { &gPlayerAnim_link_wait_typeD_20f, &gPlayerAnim_link_waitF_typeD_20f },
    { &gPlayerAnim_link_wait_typeD_20f, &gPlayerAnim_link_waitF_typeD_20f },
    { &gPlayerAnim_link_wait_heat1_20f, &gPlayerAnim_link_waitF_heat1_20f },
    { &gPlayerAnim_link_wait_heat2_20f, &gPlayerAnim_link_waitF_heat2_20f },
    { &gPlayerAnim_link_wait_itemD1_20f, &gPlayerAnim_link_wait_itemD1_20f },
    { &gPlayerAnim_link_wait_itemA_20f, &gPlayerAnim_link_waitF_itemA_20f },
    { &gPlayerAnim_link_wait_itemB_20f, &gPlayerAnim_link_waitF_itemB_20f },
    { &gPlayerAnim_link_wait_itemC_20f, &gPlayerAnim_link_wait_itemC_20f },
    { &gPlayerAnim_link_wait_itemD2_20f, &gPlayerAnim_link_wait_itemD2_20f }
};

static PlayerAnimSfxEntry sIdleSneezeAnimSfx[] = {
    { NA_SE_VO_LI_SNEEZE, -0x2008 },
};

static PlayerAnimSfxEntry sIdleSweatAnimSfx[] = {
    { NA_SE_VO_LI_SWEAT, -0x2012 },
};

static PlayerAnimSfxEntry D_80853DF4[] = {
    { NA_SE_VO_LI_BREATH_REST, -0x200D },
};

static PlayerAnimSfxEntry D_80853DF8[] = {
    { NA_SE_VO_LI_BREATH_REST, -0x200A },
};

static PlayerAnimSfxEntry D_80853DFC[] = {
    { NA_SE_PL_CALM_HIT, 0x82C }, { NA_SE_PL_CALM_HIT, 0x830 },  { NA_SE_PL_CALM_HIT, 0x834 },
    { NA_SE_PL_CALM_HIT, 0x838 }, { NA_SE_PL_CALM_HIT, -0x83C },
};

static PlayerAnimSfxEntry D_80853E10[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4019 }, { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x401E },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x402C }, { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4030 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4034 }, { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4038 },
};

static PlayerAnimSfxEntry D_80853E28[] = {
    { NA_SE_IT_SHIELD_POSTURE, 0x810 },
    { NA_SE_IT_SHIELD_POSTURE, 0x814 },
    { NA_SE_IT_SHIELD_POSTURE, -0x846 },
};

static PlayerAnimSfxEntry D_80853E34[] = {
    { NA_SE_IT_HAMMER_SWING, 0x80A },
    { NA_SE_VO_LI_AUTO_JUMP, 0x200A },
    { NA_SE_IT_SWORD_SWING, 0x816 },
    { NA_SE_VO_LI_SWORD_N, -0x2016 },
};

static PlayerAnimSfxEntry D_80853E44[] = {
    { NA_SE_IT_SWORD_SWING, 0x827 },
    { NA_SE_VO_LI_SWORD_N, -0x2027 },
};

static PlayerAnimSfxEntry D_80853E4C[] = {
    { NA_SE_VO_LI_RELAX, -0x2014 },
};

static PlayerAnimSfxEntry* sIdleAnimSfx[] = {
    sIdleSneezeAnimSfx, sIdleSweatAnimSfx, D_80853DF4, D_80853DF8, D_80853DFC, D_80853E10,
    D_80853E28,         D_80853E34,        D_80853E44, D_80853E4C, NULL,
};

static u8 sIdleAnimOffsetToAnimSfx[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 10, 10, 10, 10, 10, 10, 3, 3, 4, 4, 8, 8, 5, 5, 6, 6, 7, 7, 9, 9, 0,
};

// Used to map item IDs to item actions
static s8 sItemActions[] = {
    PLAYER_IA_DEKU_STICK,          // ITEM_DEKU_STICK
    PLAYER_IA_DEKU_NUT,            // ITEM_DEKU_NUT
    PLAYER_IA_BOMB,                // ITEM_BOMB
    PLAYER_IA_BOW,                 // ITEM_BOW
    PLAYER_IA_BOW_FIRE,            // ITEM_ARROW_FIRE
    PLAYER_IA_DINS_FIRE,           // ITEM_DINS_FIRE
    PLAYER_IA_SLINGSHOT,           // ITEM_SLINGSHOT
    PLAYER_IA_OCARINA_FAIRY,       // ITEM_OCARINA_FAIRY
    PLAYER_IA_OCARINA_OF_TIME,     // ITEM_OCARINA_OF_TIME
    PLAYER_IA_BOMBCHU,             // ITEM_BOMBCHU
    PLAYER_IA_HOOKSHOT,            // ITEM_HOOKSHOT
    PLAYER_IA_LONGSHOT,            // ITEM_LONGSHOT
    PLAYER_IA_BOW_ICE,             // ITEM_ARROW_ICE
    PLAYER_IA_FARORES_WIND,        // ITEM_FARORES_WIND
    PLAYER_IA_BOOMERANG,           // ITEM_BOOMERANG
    PLAYER_IA_LENS_OF_TRUTH,       // ITEM_LENS_OF_TRUTH
    PLAYER_IA_MAGIC_BEAN,          // ITEM_MAGIC_BEAN
    PLAYER_IA_HAMMER,              // ITEM_HAMMER
    PLAYER_IA_BOW_LIGHT,           // ITEM_ARROW_LIGHT
    PLAYER_IA_NAYRUS_LOVE,         // ITEM_NAYRUS_LOVE
    PLAYER_IA_BOTTLE,              // ITEM_BOTTLE_EMPTY
    PLAYER_IA_BOTTLE_POTION_RED,   // ITEM_BOTTLE_POTION_RED
    PLAYER_IA_BOTTLE_POTION_GREEN, // ITEM_BOTTLE_POTION_GREEN
    PLAYER_IA_BOTTLE_POTION_BLUE,  // ITEM_BOTTLE_POTION_BLUE
    PLAYER_IA_BOTTLE_FAIRY,        // ITEM_BOTTLE_FAIRY
    PLAYER_IA_BOTTLE_FISH,         // ITEM_BOTTLE_FISH
    PLAYER_IA_BOTTLE_MILK_FULL,    // ITEM_BOTTLE_MILK_FULL
    PLAYER_IA_BOTTLE_RUTOS_LETTER, // ITEM_BOTTLE_RUTOS_LETTER
    PLAYER_IA_BOTTLE_FIRE,         // ITEM_BOTTLE_BLUE_FIRE
    PLAYER_IA_BOTTLE_BUG,          // ITEM_BOTTLE_BUG
    PLAYER_IA_BOTTLE_BIG_POE,      // ITEM_BOTTLE_BIG_POE
    PLAYER_IA_BOTTLE_MILK_HALF,    // ITEM_BOTTLE_MILK_HALF
    PLAYER_IA_BOTTLE_POE,          // ITEM_BOTTLE_POE
    PLAYER_IA_WEIRD_EGG,           // ITEM_WEIRD_EGG
    PLAYER_IA_CHICKEN,             // ITEM_CHICKEN
    PLAYER_IA_ZELDAS_LETTER,       // ITEM_ZELDAS_LETTER
    PLAYER_IA_MASK_KEATON,         // ITEM_MASK_KEATON
    PLAYER_IA_MASK_SKULL,          // ITEM_MASK_SKULL
    PLAYER_IA_MASK_SPOOKY,         // ITEM_MASK_SPOOKY
    PLAYER_IA_MASK_BUNNY_HOOD,     // ITEM_MASK_BUNNY_HOOD
    PLAYER_IA_MASK_GORON,          // ITEM_MASK_GORON
    PLAYER_IA_MASK_ZORA,           // ITEM_MASK_ZORA
    PLAYER_IA_MASK_GERUDO,         // ITEM_MASK_GERUDO
    PLAYER_IA_MASK_TRUTH,          // ITEM_MASK_TRUTH
    PLAYER_IA_SWORD_MASTER,        // ITEM_SOLD_OUT
    PLAYER_IA_POCKET_EGG,          // ITEM_POCKET_EGG
    PLAYER_IA_POCKET_CUCCO,        // ITEM_POCKET_CUCCO
    PLAYER_IA_COJIRO,              // ITEM_COJIRO
    PLAYER_IA_ODD_MUSHROOM,        // ITEM_ODD_MUSHROOM
    PLAYER_IA_ODD_POTION,          // ITEM_ODD_POTION
    PLAYER_IA_POACHERS_SAW,        // ITEM_POACHERS_SAW
    PLAYER_IA_BROKEN_GORONS_SWORD, // ITEM_BROKEN_GORONS_SWORD
    PLAYER_IA_PRESCRIPTION,        // ITEM_PRESCRIPTION
    PLAYER_IA_FROG,                // ITEM_EYEBALL_FROG
    PLAYER_IA_EYEDROPS,            // ITEM_EYE_DROPS
    PLAYER_IA_CLAIM_CHECK,         // ITEM_CLAIM_CHECK
    PLAYER_IA_BOW_FIRE,            // ITEM_BOW_FIRE
    PLAYER_IA_BOW_ICE,             // ITEM_BOW_ICE
    PLAYER_IA_BOW_LIGHT,           // ITEM_BOW_LIGHT
    PLAYER_IA_SWORD_KOKIRI,        // ITEM_SWORD_KOKIRI
    PLAYER_IA_SWORD_MASTER,        // ITEM_SWORD_MASTER
    PLAYER_IA_SWORD_BGS,           // ITEM_SWORD_BIGGORON
};

static s32 (*sUpperBodyItemFuncs[])(Player* this, PlayState* play) = {
    Player_SetupStartZTargetDefend,  // PLAYER_IA_NONE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_LAST_USED
    Player_SetupStartZTargetDefend,  // PLAYER_IA_FISHING_POLE
    Player_SetupStartZTargetDefend2, // PLAYER_IA_SWORD_MASTER
    Player_SetupStartZTargetDefend2, // PLAYER_IA_SWORD_KOKIRI
    Player_SetupStartZTargetDefend2, // PLAYER_IA_SWORD_BGS
    Player_SetupStartZTargetDefend,  // PLAYER_IA_DEKU_STICK
    Player_SetupStartZTargetDefend,  // PLAYER_IA_HAMMER
    Player_HoldFpsItem,              // PLAYER_IA_BOW
    Player_HoldFpsItem,              // PLAYER_IA_BOW_FIRE
    Player_HoldFpsItem,              // PLAYER_IA_BOW_ICE
    Player_HoldFpsItem,              // PLAYER_IA_BOW_LIGHT
    Player_HoldFpsItem,              // PLAYER_IA_BOW_0C
    Player_HoldFpsItem,              // PLAYER_IA_BOW_0D
    Player_HoldFpsItem,              // PLAYER_IA_BOW_0E
    Player_HoldFpsItem,              // PLAYER_IA_SLINGSHOT
    Player_HoldFpsItem,              // PLAYER_IA_HOOKSHOT
    Player_HoldFpsItem,              // PLAYER_IA_LONGSHOT
    Player_HoldActor,                // PLAYER_IA_BOMB
    Player_HoldActor,                // PLAYER_IA_BOMBCHU
    Player_HoldBoomerang,            // PLAYER_IA_BOOMERANG
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MAGIC_SPELL_15
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MAGIC_SPELL_16
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MAGIC_SPELL_17
    Player_SetupStartZTargetDefend,  // PLAYER_IA_FARORES_WIND
    Player_SetupStartZTargetDefend,  // PLAYER_IA_NAYRUS_LOVE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_DINS_FIRE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_DEKU_NUT
    Player_SetupStartZTargetDefend,  // PLAYER_IA_OCARINA_FAIRY
    Player_SetupStartZTargetDefend,  // PLAYER_IA_OCARINA_OF_TIME
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_FISH
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_FIRE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_BUG
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_POE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_BIG_POE
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_RUTOS_LETTER
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_POTION_REDD,
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_POTION_BLUEUE,
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_POTION_GREENEEN,
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_MILK_FULL
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_MILK_HALF,
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BOTTLE_FAIRY
    Player_SetupStartZTargetDefend,  // PLAYER_IA_ZELDAS_LETTER
    Player_SetupStartZTargetDefend,  // PLAYER_IA_WEIRD_EGG
    Player_SetupStartZTargetDefend,  // PLAYER_IA_CHICKEN
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MAGIC_BEAN
    Player_SetupStartZTargetDefend,  // PLAYER_IA_POCKET_EGG
    Player_SetupStartZTargetDefend,  // PLAYER_IA_POCKET_CUCCO
    Player_SetupStartZTargetDefend,  // PLAYER_IA_COJIRO
    Player_SetupStartZTargetDefend,  // PLAYER_IA_ODD_MUSHROOM
    Player_SetupStartZTargetDefend,  // PLAYER_IA_ODD_POTION
    Player_SetupStartZTargetDefend,  // PLAYER_IA_POACHERS_SAW
    Player_SetupStartZTargetDefend,  // PLAYER_IA_BROKEN_GORONS_SWORD
    Player_SetupStartZTargetDefend,  // PLAYER_IA_PRESCRIPTION
    Player_SetupStartZTargetDefend,  // PLAYER_IA_FROG
    Player_SetupStartZTargetDefend,  // PLAYER_IA_EYEDROPS
    Player_SetupStartZTargetDefend,  // PLAYER_IA_CLAIM_CHECK
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_KEATON
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_SKULL
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_SPOOKY
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_BUNNY_HOOD
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_GORON
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_ZORA
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_GERUDO
    Player_SetupStartZTargetDefend,  // PLAYER_IA_MASK_TRUTH
    Player_SetupStartZTargetDefend,  // PLAYER_IA_LENS_OF_TRUTH
};

static void (*sItemChangeFuncs[])(PlayState* play, Player* this) = {
    Player_DoNothing,           // PLAYER_IA_NONE
    Player_DoNothing,           // PLAYER_IA_LAST_USED
    Player_DoNothing,           // PLAYER_IA_FISHING_POLE
    Player_DoNothing,           // PLAYER_IA_SWORD_MASTER
    Player_DoNothing,           // PLAYER_IA_SWORD_KOKIRI
    Player_DoNothing,           // PLAYER_IA_SWORD_BGS
    Player_SetupDekuStick,      // PLAYER_IA_DEKU_STICK
    Player_DoNothing2,          // PLAYER_IA_HAMMER
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW_FIRE
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW_ICE
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW_LIGHT
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW_0C
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW_0D
    Player_SetupBowOrSlingshot, // PLAYER_IA_BOW_0E
    Player_SetupBowOrSlingshot, // PLAYER_IA_SLINGSHOT
    Player_SetupHookshot,       // PLAYER_IA_HOOKSHOT
    Player_SetupHookshot,       // PLAYER_IA_LONGSHOT
    Player_SetupExplosive,      // PLAYER_IA_BOMB
    Player_SetupExplosive,      // PLAYER_IA_BOMBCHU
    Player_SetupBoomerang,      // PLAYER_IA_BOOMERANG
    Player_DoNothing,           // PLAYER_IA_MAGIC_SPELL_15
    Player_DoNothing,           // PLAYER_IA_MAGIC_SPELL_16
    Player_DoNothing,           // PLAYER_IA_MAGIC_SPELL_17
    Player_DoNothing,           // PLAYER_IA_FARORES_WIND
    Player_DoNothing,           // PLAYER_IA_NAYRUS_LOVE
    Player_DoNothing,           // PLAYER_IA_DINS_FIRE
    Player_DoNothing,           // PLAYER_IA_DEKU_NUT
    Player_DoNothing,           // PLAYER_IA_OCARINA_FAIRY
    Player_DoNothing,           // PLAYER_IA_OCARINA_OF_TIME
    Player_DoNothing,           // PLAYER_IA_BOTTLE
    Player_DoNothing,           // PLAYER_IA_BOTTLE_FISH
    Player_DoNothing,           // PLAYER_IA_BOTTLE_FIRE
    Player_DoNothing,           // PLAYER_IA_BOTTLE_BUG
    Player_DoNothing,           // PLAYER_IA_BOTTLE_POE
    Player_DoNothing,           // PLAYER_IA_BOTTLE_BIG_POE
    Player_DoNothing,           // PLAYER_IA_BOTTLE_RUTOS_LETTER
    Player_DoNothing,           // PLAYER_IA_BOTTLE_POTION_RED
    Player_DoNothing,           // PLAYER_IA_BOTTLE_POTION_BLUE
    Player_DoNothing,           // PLAYER_IA_BOTTLE_POTION_GREEN,
    Player_DoNothing,           // PLAYER_IA_BOTTLE_MILK_FULL
    Player_DoNothing,           // PLAYER_IA_BOTTLE_MILK_HALF
    Player_DoNothing,           // PLAYER_IA_BOTTLE_FAIRY
    Player_DoNothing,           // PLAYER_IA_ZELDAS_LETTER
    Player_DoNothing,           // PLAYER_IA_WEIRD_EGG
    Player_DoNothing,           // PLAYER_IA_CHICKEN
    Player_DoNothing,           // PLAYER_IA_MAGIC_BEAN
    Player_DoNothing,           // PLAYER_IA_POCKET_EGG
    Player_DoNothing,           // PLAYER_IA_POCKET_CUCCO
    Player_DoNothing,           // PLAYER_IA_COJIRO
    Player_DoNothing,           // PLAYER_IA_ODD_MUSHROOM
    Player_DoNothing,           // PLAYER_IA_ODD_POTION
    Player_DoNothing,           // PLAYER_IA_POACHERS_SAW
    Player_DoNothing,           // PLAYER_IA_BROKEN_GORONS_SWORD
    Player_DoNothing,           // PLAYER_IA_PRESCRIPTION
    Player_DoNothing,           // PLAYER_IA_FROG
    Player_DoNothing,           // PLAYER_IA_EYEDROPS
    Player_DoNothing,           // PLAYER_IA_CLAIM_CHECK
    Player_DoNothing,           // PLAYER_IA_MASK_KEATON
    Player_DoNothing,           // PLAYER_IA_MASK_SKULL
    Player_DoNothing,           // PLAYER_IA_MASK_SPOOKY
    Player_DoNothing,           // PLAYER_IA_MASK_BUNNY_HOOD
    Player_DoNothing,           // PLAYER_IA_MASK_GORON
    Player_DoNothing,           // PLAYER_IA_MASK_ZORA
    Player_DoNothing,           // PLAYER_IA_MASK_GERUDO
    Player_DoNothing,           // PLAYER_IA_MASK_TRUTH
    Player_DoNothing,           // PLAYER_IA_LENS_OF_TRUTH
};

typedef enum {
    /*  0 */ PLAYER_ITEM_CHANGE_DEFAULT,
    /*  1 */ PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND,
    /*  2 */ PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND,
    /*  3 */ PLAYER_ITEM_CHANGE_SHIELD,
    /*  4 */ PLAYER_ITEM_CHANGE_2HAND_TO_1HAND,
    /*  5 */ PLAYER_ITEM_CHANGE_1HAND,
    /*  6 */ PLAYER_ITEM_CHANGE_2HAND,
    /*  7 */ PLAYER_ITEM_CHANGE_2HAND_TO_2HAND,
    /*  8 */ PLAYER_ITEM_CHANGE_DEFAULT_2,
    /*  9 */ PLAYER_ITEM_CHANGE_1HAND_TO_BOMB,
    /* 10 */ PLAYER_ITEM_CHANGE_2HAND_TO_BOMB,
    /* 11 */ PLAYER_ITEM_CHANGE_BOMB,
    /* 12 */ PLAYER_ITEM_CHANGE_UNK_12, // Unused
    /* 13 */ PLAYER_ITEM_CHANGE_LEFT_HAND,
    /* 14 */ PLAYER_ITEM_CHANGE_MAX
} PlayersItemChangeAnimsIndex;

static ItemChangeAnimInfo sItemChangeAnimsInfo[PLAYER_ITEM_CHANGE_MAX] = {
    { &gPlayerAnim_link_normal_free2free, 12 },     /* PLAYER_ITEM_CHANGE_DEFAULT */
    { &gPlayerAnim_link_normal_normal2fighter, 6 }, /* PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND */
    { &gPlayerAnim_link_hammer_normal2long, 8 },    /* PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND */
    { &gPlayerAnim_link_normal_normal2free, 8 },    /* PLAYER_ITEM_CHANGE_SHIELD */
    { &gPlayerAnim_link_fighter_fighter2long, 8 },  /* PLAYER_ITEM_CHANGE_2HAND_TO_1HAND */
    { &gPlayerAnim_link_normal_fighter2free, 10 },  /* PLAYER_ITEM_CHANGE_1HAND */
    { &gPlayerAnim_link_hammer_long2free, 7 },      /* PLAYER_ITEM_CHANGE_2HAND */
    { &gPlayerAnim_link_hammer_long2long, 11 },     /* PLAYER_ITEM_CHANGE_2HAND_TO_2HAND */
    { &gPlayerAnim_link_normal_free2free, 12 },     /* PLAYER_ITEM_CHANGE_DEFAULT_2 */
    { &gPlayerAnim_link_normal_normal2bom, 4 },     /* PLAYER_ITEM_CHANGE_1HAND_TO_BOMB */
    { &gPlayerAnim_link_normal_long2bom, 4 },       /* PLAYER_ITEM_CHANGE_2HAND_TO_BOMB */
    { &gPlayerAnim_link_normal_free2bom, 4 },       /* PLAYER_ITEM_CHANGE_BOMB */
    { &gPlayerAnim_link_anchor_anchor2fighter, 5 }, /* PLAYER_ITEM_CHANGE_UNK_12 */
    { &gPlayerAnim_link_normal_free2freeB, 13 },    /* PLAYER_ITEM_CHANGE_LEFT_HAND */
};

static s8 sAnimtypeToItemChangeAnims[PLAYER_ANIMTYPE_MAX][PLAYER_ANIMTYPE_MAX] = {
    // From: PLAYER_ANIMTYPE_DEFAULT, to:
    {
        PLAYER_ITEM_CHANGE_DEFAULT_2, // PLAYER_ANIMTYPE_DEFAULT
        -PLAYER_ITEM_CHANGE_1HAND,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
        -PLAYER_ITEM_CHANGE_SHIELD,   // PLAYER_ANIMTYPE_HOLDING_SHIELD
        -PLAYER_ITEM_CHANGE_2HAND,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
        PLAYER_ITEM_CHANGE_DEFAULT_2, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
        PLAYER_ITEM_CHANGE_BOMB       // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    },
    // From: PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON, to:
    {
        PLAYER_ITEM_CHANGE_1HAND,            // PLAYER_ANIMTYPE_DEFAULT
        PLAYER_ITEM_CHANGE_DEFAULT,          // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
        -PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND, // PLAYER_ANIMTYPE_HOLDING_SHIELD
        PLAYER_ITEM_CHANGE_2HAND_TO_1HAND,   // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
        PLAYER_ITEM_CHANGE_1HAND,            // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
        PLAYER_ITEM_CHANGE_1HAND_TO_BOMB     // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    },
    // From: PLAYER_ANIMTYPE_HOLDING_SHIELD, to:
    {
        PLAYER_ITEM_CHANGE_SHIELD,          // PLAYER_ANIMTYPE_DEFAULT
        PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
        PLAYER_ITEM_CHANGE_DEFAULT,         // PLAYER_ANIMTYPE_HOLDING_SHIELD
        PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
        PLAYER_ITEM_CHANGE_SHIELD,          // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
        PLAYER_ITEM_CHANGE_1HAND_TO_BOMB    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    },
    // From: PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON, to:
    {
        PLAYER_ITEM_CHANGE_2HAND,            // PLAYER_ANIMTYPE_DEFAULT
        -PLAYER_ITEM_CHANGE_2HAND_TO_1HAND,  // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
        -PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND, // PLAYER_ANIMTYPE_HOLDING_SHIELD
        PLAYER_ITEM_CHANGE_2HAND_TO_2HAND,   // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
        PLAYER_ITEM_CHANGE_2HAND,            // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
        PLAYER_ITEM_CHANGE_2HAND_TO_BOMB     // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    },
    // From: PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND, to:
    {
        PLAYER_ITEM_CHANGE_DEFAULT_2, // PLAYER_ANIMTYPE_DEFAULT
        -PLAYER_ITEM_CHANGE_1HAND,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
        -PLAYER_ITEM_CHANGE_SHIELD,   // PLAYER_ANIMTYPE_HOLDING_SHIELD
        -PLAYER_ITEM_CHANGE_2HAND,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
        PLAYER_ITEM_CHANGE_DEFAULT_2, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
        PLAYER_ITEM_CHANGE_BOMB       // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    },
    // From: PLAYER_ANIMTYPE_USED_EXPLOSIVE, to:
    {
        PLAYER_ITEM_CHANGE_DEFAULT_2, // PLAYER_ANIMTYPE_DEFAULT
        -PLAYER_ITEM_CHANGE_1HAND,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
        -PLAYER_ITEM_CHANGE_SHIELD,   // PLAYER_ANIMTYPE_HOLDING_SHIELD
        -PLAYER_ITEM_CHANGE_2HAND,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
        PLAYER_ITEM_CHANGE_DEFAULT_2, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
        PLAYER_ITEM_CHANGE_BOMB       // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    },
};

static ExplosiveInfo sExplosiveInfos[] = {
    { ITEM_BOMB, ACTOR_EN_BOM },
    { ITEM_BOMBCHU, ACTOR_EN_BOM_CHU },
};

static MeleeAttackAnimInfo sMeleeAttackAnims[PLAYER_MELEEATKTYPE_MAX] = {
    /* PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H */
    { &gPlayerAnim_link_fighter_normal_kiru, &gPlayerAnim_link_fighter_normal_kiru_end,
      &gPlayerAnim_link_fighter_normal_kiru_endR, 1, 4 },
    /* PLAYER_MELEEATKTYPE_FORWARD_SLASH_2H */
    { &gPlayerAnim_link_fighter_Lnormal_kiru, &gPlayerAnim_link_fighter_Lnormal_kiru_end,
      &gPlayerAnim_link_anchor_Lnormal_kiru_endR, 1, 4 },
    /* PLAYER_MELEEATKTYPE_FORWARD_COMBO_1H */
    { &gPlayerAnim_link_fighter_normal_kiru_finsh, &gPlayerAnim_link_fighter_normal_kiru_finsh_end,
      &gPlayerAnim_link_anchor_normal_kiru_finsh_endR, 0, 5 },
    /* PLAYER_MELEEATKTYPE_FORWARD_COMBO_2H */
    { &gPlayerAnim_link_fighter_Lnormal_kiru_finsh, &gPlayerAnim_link_fighter_Lnormal_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Lnormal_kiru_finsh_endR, 1, 7 },
    /* PLAYER_MELEEATKTYPE_LEFT_SLASH_1H */
    { &gPlayerAnim_link_fighter_Lside_kiru, &gPlayerAnim_link_fighter_Lside_kiru_end,
      &gPlayerAnim_link_anchor_Lside_kiru_endR, 1, 4 },
    /* PLAYER_MELEEATKTYPE_LEFT_SLASH_2H */
    { &gPlayerAnim_link_fighter_LLside_kiru, &gPlayerAnim_link_fighter_LLside_kiru_end,
      &gPlayerAnim_link_anchor_LLside_kiru_endL, 0, 5 },
    /* PLAYER_MELEEATKTYPE_LEFT_COMBO_1H */
    { &gPlayerAnim_link_fighter_Lside_kiru_finsh, &gPlayerAnim_link_fighter_Lside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Lside_kiru_finsh_endR, 2, 8 },
    /* PLAYER_MELEEATKTYPE_LEFT_COMBO_2H */
    { &gPlayerAnim_link_fighter_LLside_kiru_finsh, &gPlayerAnim_link_fighter_LLside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_LLside_kiru_finsh_endR, 3, 8 },
    /* PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H */
    { &gPlayerAnim_link_fighter_Rside_kiru, &gPlayerAnim_link_fighter_Rside_kiru_end,
      &gPlayerAnim_link_anchor_Rside_kiru_endR, 0, 4 },
    /* PLAYER_MELEEATKTYPE_RIGHT_SLASH_2H */
    { &gPlayerAnim_link_fighter_LRside_kiru, &gPlayerAnim_link_fighter_LRside_kiru_end,
      &gPlayerAnim_link_anchor_LRside_kiru_endR, 0, 5 },
    /* PLAYER_MELEEATKTYPE_RIGHT_COMBO_1H */
    { &gPlayerAnim_link_fighter_Rside_kiru_finsh, &gPlayerAnim_link_fighter_Rside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Rside_kiru_finsh_endR, 0, 6 },
    /* PLAYER_MELEEATKTYPE_RIGHT_COMBO_2H */
    { &gPlayerAnim_link_fighter_LRside_kiru_finsh, &gPlayerAnim_link_fighter_LRside_kiru_finsh_end,
      &gPlayerAnim_link_anchor_LRside_kiru_finsh_endL, 1, 5 },
    /* PLAYER_MELEEATKTYPE_STAB_1H */
    { &gPlayerAnim_link_fighter_pierce_kiru, &gPlayerAnim_link_fighter_pierce_kiru_end,
      &gPlayerAnim_link_anchor_pierce_kiru_endR, 0, 3 },
    /* PLAYER_MELEEATKTYPE_STAB_2H */
    { &gPlayerAnim_link_fighter_Lpierce_kiru, &gPlayerAnim_link_fighter_Lpierce_kiru_end,
      &gPlayerAnim_link_anchor_Lpierce_kiru_endL, 0, 3 },
    /* PLAYER_MELEEATKTYPE_STAB_COMBO_1H */
    { &gPlayerAnim_link_fighter_pierce_kiru_finsh, &gPlayerAnim_link_fighter_pierce_kiru_finsh_end,
      &gPlayerAnim_link_anchor_pierce_kiru_finsh_endR, 1, 9 },
    /* PLAYER_MELEEATKTYPE_STAB_COMBO_2H */
    { &gPlayerAnim_link_fighter_Lpierce_kiru_finsh, &gPlayerAnim_link_fighter_Lpierce_kiru_finsh_end,
      &gPlayerAnim_link_anchor_Lpierce_kiru_finsh_endR, 1, 8 },
    /* PLAYER_MELEEATKTYPE_FLIPSLASH_START */
    { &gPlayerAnim_link_fighter_jump_rollkiru, &gPlayerAnim_link_fighter_jump_kiru_finsh,
      &gPlayerAnim_link_fighter_jump_kiru_finsh, 1, 10 },
    /* PLAYER_MELEEATKTYPE_JUMPSLASH_START */
    { &gPlayerAnim_link_fighter_Lpower_jump_kiru, &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit,
      &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit, 1, 11 },
    /* PLAYER_MELEEATKTYPE_FLIPSLASH_FINISH */
    { &gPlayerAnim_link_fighter_jump_kiru_finsh, &gPlayerAnim_link_fighter_jump_kiru_finsh_end,
      &gPlayerAnim_link_fighter_jump_kiru_finsh_end, 1, 2 },
    /* PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH */
    { &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit, &gPlayerAnim_link_fighter_Lpower_jump_kiru_end,
      &gPlayerAnim_link_fighter_Lpower_jump_kiru_end, 1, 2 },
    /* PLAYER_MELEEATKTYPE_BACKSLASH_RIGHT */
    { &gPlayerAnim_link_fighter_turn_kiruR, &gPlayerAnim_link_fighter_turn_kiruR_end,
      &gPlayerAnim_link_fighter_turn_kiruR_end, 1, 5 },
    /* PLAYER_MELEEATKTYPE_BACKSLASH_LEFT */
    { &gPlayerAnim_link_fighter_turn_kiruL, &gPlayerAnim_link_fighter_turn_kiruL_end,
      &gPlayerAnim_link_fighter_turn_kiruL_end, 1, 4 },
    /* PLAYER_MELEEATKTYPE_HAMMER_FORWARD */
    { &gPlayerAnim_link_hammer_hit, &gPlayerAnim_link_hammer_hit_end, &gPlayerAnim_link_hammer_hit_endR, 3, 10 },
    /* PLAYER_MELEEATKTYPE_HAMMER_SIDE */
    { &gPlayerAnim_link_hammer_side_hit, &gPlayerAnim_link_hammer_side_hit_end, &gPlayerAnim_link_hammer_side_hit_endR,
      2, 11 },
    /* PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H */
    { &gPlayerAnim_link_fighter_rolling_kiru, &gPlayerAnim_link_fighter_rolling_kiru_end,
      &gPlayerAnim_link_anchor_rolling_kiru_endR, 0, 12 },
    /* PLAYER_MELEEATKTYPE_SPIN_ATTACK_2H */
    { &gPlayerAnim_link_fighter_Lrolling_kiru, &gPlayerAnim_link_fighter_Lrolling_kiru_end,
      &gPlayerAnim_link_anchor_Lrolling_kiru_endR, 0, 15 },
    /* PLAYER_MELEEATKTYPE_BIG_SPIN_1H */
    { &gPlayerAnim_link_fighter_Wrolling_kiru, &gPlayerAnim_link_fighter_Wrolling_kiru_end,
      &gPlayerAnim_link_anchor_rolling_kiru_endR, 0, 16 },
    /* PLAYER_MELEEATKTYPE_BIG_SPIN_2H */
    { &gPlayerAnim_link_fighter_Wrolling_kiru, &gPlayerAnim_link_fighter_Wrolling_kiru_end,
      &gPlayerAnim_link_anchor_Lrolling_kiru_endR, 0, 16 },
};

static LinkAnimationHeader* sSpinAttackAnims2[] = {
    &gPlayerAnim_link_fighter_power_kiru_start,  // One-handed
    &gPlayerAnim_link_fighter_Lpower_kiru_start, // Two-handed
};

static LinkAnimationHeader* sSpinAttackAnims1[] = {
    &gPlayerAnim_link_fighter_power_kiru_startL, // One-handed
    &gPlayerAnim_link_fighter_Lpower_kiru_start, // Two-handed
};

static LinkAnimationHeader* sSpinAttackChargeAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_wait,  // One-handed
    &gPlayerAnim_link_fighter_Lpower_kiru_wait, // Two-handed
};

static LinkAnimationHeader* sCancelSpinAttackChargeAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_wait_end,  // One-handed
    &gPlayerAnim_link_fighter_Lpower_kiru_wait_end, // Two-handed
};

static LinkAnimationHeader* sSpinAttackChargeWalkAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_walk,  // One-handed
    &gPlayerAnim_link_fighter_Lpower_kiru_walk, // Two-handed
};

static LinkAnimationHeader* sSpinAttackChargeSidewalkAnims[] = {
    &gPlayerAnim_link_fighter_power_kiru_side_walk,  // One-handed
    &gPlayerAnim_link_fighter_Lpower_kiru_side_walk, // Two-handed
};

static u8 sSmallSpinAttackMWAs[2] = { PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H, PLAYER_MELEEATKTYPE_SPIN_ATTACK_2H };
static u8 sBigSpinAttackMWAs[2] = { PLAYER_MELEEATKTYPE_BIG_SPIN_1H, PLAYER_MELEEATKTYPE_BIG_SPIN_2H };

static u16 sUseItemButtons[] = { BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };

static u8 sMagicSpellCosts[] = { 12, 24, 24, 12, 24, 12 };

static u16 sFpsItemReadySfx[] = { NA_SE_IT_BOW_DRAW, NA_SE_IT_SLING_DRAW, NA_SE_IT_HOOKSHOT_READY };

static u8 sMagicArrowCosts[] = { 4, 4, 8 };

static LinkAnimationHeader* sRightDefendStandingAnims[] = {
    &gPlayerAnim_link_anchor_waitR2defense,
    &gPlayerAnim_link_anchor_waitR2defense_long,
};

static LinkAnimationHeader* sLeftDefendStandingAnims[] = {
    &gPlayerAnim_link_anchor_waitL2defense,
    &gPlayerAnim_link_anchor_waitL2defense_long,
};

static LinkAnimationHeader* sLeftStandingDeflectWithShieldAnims[] = {
    &gPlayerAnim_link_anchor_defense_hit,
    &gPlayerAnim_link_anchor_defense_long_hitL,
};

static LinkAnimationHeader* sRightStandingDeflectWithShieldAnims[] = {
    &gPlayerAnim_link_anchor_defense_hit,
    &gPlayerAnim_link_anchor_defense_long_hitR,
};

static LinkAnimationHeader* sDeflectWithShieldAnims[] = {
    &gPlayerAnim_link_normal_defense_hit,
    &gPlayerAnim_link_fighter_defense_long_hit,
};

static LinkAnimationHeader* sReadyFpsItemWhileWalkingAnims[] = {
    &gPlayerAnim_link_bow_walk2ready,
    &gPlayerAnim_link_hook_walk2ready,
};

static LinkAnimationHeader* sReadyFpsItemAnims[] = {
    &gPlayerAnim_link_bow_bow_wait,
    &gPlayerAnim_link_hook_wait,
};

// return type can't be void due to regalloc in Player_CheckNoDebugModeCombo
BAD_RETURN(s32) Player_StopMovement(Player* this) {
    this->actor.speedXZ = 0.0f;
    this->linearVelocity = 0.0f;
}

// return type can't be void due to regalloc in Player_StartGrabPushPullWall
BAD_RETURN(s32) Player_ClearAttentionModeAndStopMoving(Player* this) {
    Player_StopMovement(this);
    this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
}

s32 Player_CheckActorTalkRequested(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_8);
}

void Player_PlayAnimOnce(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayOnce(play, &this->skelAnime, anim);
}

void Player_PlayAnimLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayLoop(play, &this->skelAnime, anim);
}

void Player_PlayAnimLoopSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, anim, 2.0f / 3.0f);
}

void Player_PlayAnimOnceSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 2.0f / 3.0f);
}

void Player_AddRootYawToShapeYaw(Player* this) {
    this->actor.shape.rot.y += this->skelAnime.jointTable[1].y;
    this->skelAnime.jointTable[1].y = 0;
}

void Player_InactivateMeleeWeapon(Player* this) {
    this->stateFlags2 &= ~PLAYER_STATE2_RELEASING_SPIN_ATTACK;
    this->isMeleeWeaponAttacking = 0;
    this->meleeWeaponInfo[0].active = this->meleeWeaponInfo[1].active = this->meleeWeaponInfo[2].active = 0;
}

void Player_ResetSubCam(PlayState* play, Player* this) {
    Camera* subCam;

    if (this->subCamId != CAM_ID_NONE) {
        subCam = play->cameraPtrs[this->subCamId];
        if ((subCam != NULL) && (subCam->csId == 1100)) {
            OnePointCutscene_EndCutscene(play, this->subCamId);
            this->subCamId = CAM_ID_NONE;
        }
    }

    this->stateFlags2 &= ~(PLAYER_STATE2_DIVING | PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER);
}

void Player_DetatchHeldActor(PlayState* play, Player* this) {
    Actor* heldActor = this->heldActor;

    if ((heldActor != NULL) && !Player_HoldsHookshot(this)) {
        this->actor.child = NULL;
        this->heldActor = NULL;
        this->interactRangeActor = NULL;
        heldActor->parent = NULL;
        this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
    }

    if (Player_GetExplosiveHeld(this) >= 0) {
        Player_ChangeItem(play, this, PLAYER_IA_NONE);
        this->heldItemId = ITEM_NONE_FE;
    }
}

void Player_ResetAttributes(PlayState* play, Player* this) {
    if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor == NULL)) {
        if (this->interactRangeActor != NULL) {
            if (this->getItemId == GI_NONE) {
                this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
                this->interactRangeActor = NULL;
            }
        } else {
            this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
        }
    }

    Player_InactivateMeleeWeapon(this);
    this->attentionMode = PLAYER_ATTENTIONMODE_NONE;

    Player_ResetSubCam(play, this);
    func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));

    this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                           PLAYER_STATE1_IN_FIRST_PERSON_MODE | PLAYER_STATE1_CLIMBING);
    this->stateFlags2 &=
        ~(PLAYER_STATE2_MOVING_PUSH_PULL_WALL | PLAYER_STATE2_RESTRAINED_BY_ENEMY | PLAYER_STATE2_CRAWLING);

    this->actor.shape.rot.x = 0;
    this->actor.shape.yOffset = 0.0f;

    this->slashCounter = this->comboTimer = 0;
}

s32 Player_UnequipItem(PlayState* play, Player* this) {
    if (this->heldItemAction >= PLAYER_IA_FISHING_POLE) {
        Player_UseItem(play, this, ITEM_NONE);
        return 1;
    } else {
        return 0;
    }
}

void Player_ResetAttributesAndHeldActor(PlayState* play, Player* this) {
    Player_ResetAttributes(play, this);
    Player_DetatchHeldActor(play, this);
}

s32 Player_MashTimerThresholdExceeded(Player* this, s32 timerStep, s32 timerThreshold) {
    s16 temp = this->analogStickAngle - sAnalogStickAngle;

    this->mashTimer += timerStep + (s16)(ABS(temp) * fabsf(sAnalogStickDistance) * 2.5415802156203426e-06f);

    if (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B)) {
        this->mashTimer += 5;
    }

    return this->mashTimer > timerThreshold;
}

void Player_SetFreezeFlashTimer(PlayState* play) {
    if (play->actorCtx.freezeFlashTimer == 0) {
        play->actorCtx.freezeFlashTimer = 1;
    }
}

void Player_RequestRumble(Player* this, s32 sourceStrength, s32 duration, s32 decreaseRate, s32 distSq) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        Rumble_Request(distSq, sourceStrength, duration, decreaseRate);
    }
}

void Player_PlayVoiceSfxForAge(Player* this, u16 sfxId) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        func_8002F7DC(&this->actor, sfxId + this->ageProperties->ageVoiceSfxOffset);
    } else {
        func_800F4190(&this->actor.projectedPos, sfxId);
    }
}

void Player_StopInterruptableSfx(Player* this) {
    u16* entry = &sInterruptableSfx[0];
    s32 i;

    for (i = 0; i < 4; i++) {
        Audio_StopSfxById((u16)(*entry + this->ageProperties->ageVoiceSfxOffset));
        entry++;
    }
}

u16 Player_GetMoveSfx(Player* this, u16 sfxId) {
    return sfxId + this->floorSfxOffset;
}

void Player_PlayMoveSfx(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, Player_GetMoveSfx(this, sfxId));
}

u16 Player_GetMoveSfxForAge(Player* this, u16 sfxId) {
    return sfxId + this->floorSfxOffset + this->ageProperties->ageMoveSfxOffset;
}

void Player_PlayMoveSfxForAge(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, Player_GetMoveSfxForAge(this, sfxId));
}

void Player_PlayWalkSfx(Player* this, f32 arg1) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_IRON_BOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_WALK_GROUND);
    }

    func_800F4010(&this->actor.projectedPos, sfxId, arg1);
}

void Player_PlayJumpSfx(Player* this) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_JUMP + SURFACE_SFX_OFFSET_IRON_BOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_JUMP);
    }

    func_8002F7DC(&this->actor, sfxId);
}

void Player_PlayLandingSfx(Player* this) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_LAND + SURFACE_SFX_OFFSET_IRON_BOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_LAND);
    }

    func_8002F7DC(&this->actor, sfxId);
}

void Player_PlayReactableSfx(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, sfxId);
    this->stateFlags2 |= PLAYER_STATE2_MAKING_REACTABLE_NOISE;
}

#define PLAYER_ANIMSFX_GET_FLAGS(data) ((data)&0x7800)
#define PLAYER_ANIMSFX_GET_PLAY_FRAME(data) ((data)&0x7FF)

void Player_PlayAnimSfx(Player* this, PlayerAnimSfxEntry* animSfxEntry) {
    s32 data;
    s32 flags;
    u32 cont;
    s32 pad;

    do {
        data = ABS(animSfxEntry->field);
        flags = PLAYER_ANIMSFX_GET_FLAGS(data);
        if (LinkAnimation_OnFrame(&this->skelAnime, fabsf(PLAYER_ANIMSFX_GET_PLAY_FRAME(data)))) {
            if (flags == PLAYER_ANIMSFXFLAGS_0) {
                func_8002F7DC(&this->actor, animSfxEntry->sfxId);
            } else if (flags == PLAYER_ANIMSFXFLAGS_1) {
                Player_PlayMoveSfx(this, animSfxEntry->sfxId);
            } else if (flags == (PLAYER_ANIMSFXFLAGS_0 | PLAYER_ANIMSFXFLAGS_1)) {
                Player_PlayMoveSfxForAge(this, animSfxEntry->sfxId);
            } else if (flags == PLAYER_ANIMSFXFLAGS_2) {
                Player_PlayVoiceSfxForAge(this, animSfxEntry->sfxId);
            } else if (flags == (PLAYER_ANIMSFXFLAGS_0 | PLAYER_ANIMSFXFLAGS_2)) {
                Player_PlayLandingSfx(this);
            } else if (flags == (PLAYER_ANIMSFXFLAGS_1 | PLAYER_ANIMSFXFLAGS_2)) {
                Player_PlayWalkSfx(this, 6.0f);
            } else if (flags == (PLAYER_ANIMSFXFLAGS_0 | PLAYER_ANIMSFXFLAGS_1 | PLAYER_ANIMSFXFLAGS_2)) {
                Player_PlayJumpSfx(this);
            } else if (flags == (PLAYER_ANIMSFXFLAGS_3)) {
                Player_PlayWalkSfx(this, 0.0f);
            } else if (flags == (PLAYER_ANIMSFXFLAGS_0 | PLAYER_ANIMSFXFLAGS_3)) {
                func_800F4010(&this->actor.projectedPos, this->ageProperties->ageMoveSfxOffset + NA_SE_PL_WALK_LADDER,
                              0.0f);
            }
        }
        cont = (animSfxEntry->field >= 0);
        animSfxEntry++;
    } while (cont);
}

void Player_ChangeAnimMorphToLastFrame(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -6.0f);
}

void Player_ChangeAnimSlowedMorphToLastFrame(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 2.0f / 3.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE,
                         -6.0f);
}

void Player_ChangeAnimShortMorphLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -6.0f);
}

void Player_ChangeAnimOnce(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, 0.0f);
}

void Player_ChangeAnimLongMorphLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -16.0f);
}

s32 Player_LoopAnimContinuously(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, anim);
        return 1;
    } else {
        return 0;
    }
}

void Player_AnimUpdatePrevTranslRot(Player* this) {
    this->skelAnime.prevTransl = this->skelAnime.baseTransl;
    this->skelAnime.prevRot = this->actor.shape.rot.y;
}

void Player_AnimUpdatePrevTranslRotApplyAgeScale(Player* this) {
    Player_AnimUpdatePrevTranslRot(this);
    this->skelAnime.prevTransl.x *= this->ageProperties->translationScale;
    this->skelAnime.prevTransl.y *= this->ageProperties->translationScale;
    this->skelAnime.prevTransl.z *= this->ageProperties->translationScale;
}

void Player_ClearRootLimbPosY(Player* this) {
    this->skelAnime.jointTable[1].y = 0;
}

void Player_EndAnimMovement(Player* this) {
    if (this->skelAnime.moveFlags != 0) {
        Player_AddRootYawToShapeYaw(this);
        this->skelAnime.jointTable[0].x = this->skelAnime.baseTransl.x;
        this->skelAnime.jointTable[0].z = this->skelAnime.baseTransl.z;
        if (this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION) {
            if (this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_UPDATE_Y) {
                this->skelAnime.jointTable[0].y = this->skelAnime.prevTransl.y;
            }
        } else {
            this->skelAnime.jointTable[0].y = this->skelAnime.baseTransl.y;
        }
        Player_AnimUpdatePrevTranslRot(this);
        this->skelAnime.moveFlags = 0;
    }
}

void Player_UpdateAnimMovement(Player* this, s32 flags) {
    Vec3f pos;

    this->skelAnime.moveFlags = flags;
    this->skelAnime.prevTransl = this->skelAnime.baseTransl;
    SkelAnime_UpdateTranslation(&this->skelAnime, &pos, this->actor.shape.rot.y);

    if (flags & PLAYER_ANIMMOVEFLAGS_UPDATE_XZ) {
        if (!LINK_IS_ADULT) {
            pos.x *= 0.64f;
            pos.z *= 0.64f;
        }
        this->actor.world.pos.x += pos.x * this->actor.scale.x;
        this->actor.world.pos.z += pos.z * this->actor.scale.z;
    }

    if (flags & PLAYER_ANIMMOVEFLAGS_UPDATE_Y) {
        if (!(flags & PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE)) {
            pos.y *= this->ageProperties->translationScale;
        }
        this->actor.world.pos.y += pos.y * this->actor.scale.y;
    }

    Player_AddRootYawToShapeYaw(this);
}

void Player_SetupAnimMovement(PlayState* play, Player* this, s32 flags) {
    if (flags & PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE) {
        Player_AnimUpdatePrevTranslRotApplyAgeScale(this);
    } else if ((flags & PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT) || (this->skelAnime.moveFlags != 0)) {
        Player_AnimUpdatePrevTranslRot(this);
    } else {
        this->skelAnime.prevTransl = this->skelAnime.jointTable[0];
        this->skelAnime.prevRot = this->actor.shape.rot.y;
    }

    this->skelAnime.moveFlags = flags;
    Player_StopMovement(this);
    AnimationContext_DisableQueue(play);
}

void Player_PlayAnimOnceWithMovementSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags,
                                             f32 playbackSpeed) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, playbackSpeed);
    Player_SetupAnimMovement(play, this, flags);
}

void Player_PlayAnimOnceWithMovement(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimOnceWithMovementSetSpeed(play, this, anim, flags, 1.0f);
}

void Player_PlayAnimOnceWithMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimOnceWithMovementSetSpeed(play, this, anim, flags, 2.0f / 3.0f);
}

void Player_PlayAnimOnceWithMovementPresetFlagsSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, anim,
                                          PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                              PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                              PLAYER_ANIMMOVEFLAGS_NO_MOVE);
}

void Player_PlayAnimLoopWithMovementSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags,
                                             f32 playbackSpeed) {
    LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, anim, playbackSpeed);
    Player_SetupAnimMovement(play, this, flags);
}

void Player_PlayAnimLoopWithMovement(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimLoopWithMovementSetSpeed(play, this, anim, flags, 1.0f);
}

void Player_PlayAnimLoopWithMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimLoopWithMovementSetSpeed(play, this, anim, flags, 2.0f / 3.0f);
}

void Player_PlayAnimLoopWithMovementPresetFlagsSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_PlayAnimLoopWithMovementSlowed(play, this, anim,
                                          PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                              PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                              PLAYER_ANIMMOVEFLAGS_NO_MOVE);
}

// Stores four consecutive frames of analog stick input data into two buffers, one offset by cam angle and the other not
void Player_StoreAnalogStickInput(PlayState* play, Player* this) {
    s8 scaledStickAngle;
    s8 scaledCamOffsetStickAngle;

    this->analogStickDistance = sAnalogStickDistance;
    this->analogStickAngle = sAnalogStickAngle;

    // Get analog stick dist and angle, stick dist ranges from -60.0f to 60.0f on each axis
    func_80077D10(&sAnalogStickDistance, &sAnalogStickAngle, sControlInput);

    sCameraOffsetAnalogStickAngle = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)) + sAnalogStickAngle;

    // Loops from 0 to 3 over and over
    this->inputFrameCounter = (this->inputFrameCounter + 1) % 4;

    if (sAnalogStickDistance < 55.0f) {
        scaledCamOffsetStickAngle = -1;
        scaledStickAngle = -1;
    } else {
        scaledStickAngle = (u16)(sAnalogStickAngle + DEG_TO_BINANG(45.0f)) >> 9;
        scaledCamOffsetStickAngle =
            (u16)((s16)(sCameraOffsetAnalogStickAngle - this->actor.shape.rot.y) + DEG_TO_BINANG(45.0f)) >> 14;
    }

    this->analogStickInputs[this->inputFrameCounter] = scaledStickAngle;
    this->relativeAnalogStickInputs[this->inputFrameCounter] = scaledCamOffsetStickAngle;
}

// If the player is underwater, sWaterSpeedScale will be 0.5f, making the animation play half as fast
void Player_PlayAnimOnceWithWaterInfluence(PlayState* play, Player* this, LinkAnimationHeader* linkAnim) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, linkAnim, sWaterSpeedScale);
}

s32 Player_IsSwimming(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_SWIMMING) && (this->currentBoots != PLAYER_BOOTS_IRON);
}

s32 Player_IsAimingBoomerang(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_AIMING_BOOMERANG);
}

void Player_SetGetItemDrawIdPlusOne(Player* this, PlayState* play) {
    GetItemEntry* giEntry = &sGetItemTable[this->getItemId - 1];

    this->giDrawIdPlusOne = ABS(giEntry->gi);
}

static LinkAnimationHeader* Player_GetStandingStillAnim(Player* this) {
    return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType);
}

// If idle anim is playing, returns the anim's offset from start of sIdleAnims plus one
s32 Player_IsPlayingIdleAnim(Player* this) {
    LinkAnimationHeader** idleAnim;
    s32 i;

    if (Player_GetStandingStillAnim(this) != this->skelAnime.animation) {
        for (i = 0, idleAnim = &sIdleAnims[0][0]; i < 28; i++, idleAnim++) {
            if (this->skelAnime.animation == *idleAnim) {
                return i + 1;
            }
        }
        return 0;
    }

    return -1;
}

void Player_PlayIdleAnimSfx(Player* this, s32 idleAnimOffset) {
    if (sIdleAnimOffsetToAnimSfx[idleAnimOffset] != 0) {
        Player_PlayAnimSfx(this, sIdleAnimSfx[sIdleAnimOffsetToAnimSfx[idleAnimOffset] - 1]);
    }
}

LinkAnimationHeader* Player_GetRunningAnim(Player* this) {
    if (this->runDamageTimer != 0) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_damage_run, this->modelAnimType);
    } else if (!(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
               (this->currentBoots == PLAYER_BOOTS_IRON)) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_heavy_run, this->modelAnimType);
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_run, this->modelAnimType);
    }
}

s32 Player_IsAimingReadyBoomerang(Player* this) {
    return Player_IsAimingBoomerang(this) && (this->fpsItemTimer != 0);
}

LinkAnimationHeader* Player_GetFightingRightAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_waitR;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitR, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetFightingLeftAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_waitL;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitL, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetEndSidewalkAnim(Player* this) {
    if (Actor_PlayerIsAimingReadyFpsItem(this)) {
        return &gPlayerAnim_link_bow_side_walk;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walk, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetSidewalkRightAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_side_walkR;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkR, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetSidewalkLeftAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_link_boom_throw_side_walkL;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkL, this->modelAnimType);
    }
}

void Player_SetUpperActionFunc(Player* this, PlayerUpperActionFunc arg1) {
    this->upperActionFunc = arg1;
    this->fpsItemShootState = 0;
    this->upperInterpWeight = 0.0f;
    Player_StopInterruptableSfx(this);
}

void Player_SetupChangeItemAnim(PlayState* play, Player* this, s8 itemAction) {
    LinkAnimationHeader* current = this->skelAnime.animation;
    LinkAnimationHeader** iter = sPlayerAnimations + this->modelAnimType;
    u32 i;

    this->stateFlags1 &= ~(PLAYER_STATE1_AIMING_FPS_ITEM | PLAYER_STATE1_AIMING_BOOMERANG);

    for (i = 0; i < PLAYER_ANIMGROUP_MAX; i++) {
        if (current == *iter) {
            break;
        }
        iter += PLAYER_ANIMTYPE_MAX;
    }

    Player_ChangeItem(play, this, itemAction);

    if (i < PLAYER_ANIMGROUP_MAX) {
        this->skelAnime.animation = GET_PLAYER_ANIM(i, this->modelAnimType);
    }
}

s8 Player_ItemToItemAction(s32 item) {
    if (item >= ITEM_NONE_FE) {
        return PLAYER_IA_NONE;
    } else if (item == ITEM_LAST_USED) {
        return PLAYER_IA_LAST_USED;
    } else if (item == ITEM_FISHING_POLE) {
        return PLAYER_IA_FISHING_POLE;
    } else {
        return sItemActions[item];
    }
}

void Player_DoNothing(PlayState* play, Player* this) {
}

void Player_SetupDekuStick(PlayState* play, Player* this) {
    this->dekuStickLength = 1.0f;
}

void Player_DoNothing2(PlayState* play, Player* this) {
}

void Player_SetupBowOrSlingshot(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_AIMING_FPS_ITEM;

    if (this->heldItemAction != PLAYER_IA_SLINGSHOT) {
        this->fpsItemType = PLAYER_FPSITEM_BOW;
    } else {
        this->fpsItemType = PLAYER_FPSITEM_SLINGSHOT;
    }
}

void Player_SetupExplosive(PlayState* play, Player* this) {
    s32 explosiveType;
    ExplosiveInfo* explosiveInfo;
    Actor* spawnedActor;

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        Player_UnequipItem(play, this);
        return;
    }

    explosiveType = Player_GetExplosiveHeld(this);
    explosiveInfo = &sExplosiveInfos[explosiveType];

    spawnedActor =
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, explosiveInfo->actorId, this->actor.world.pos.x,
                           this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, 0);
    if (spawnedActor != NULL) {
        if ((explosiveType != 0) && (play->bombchuBowlingStatus != 0)) {
            play->bombchuBowlingStatus--;
            if (play->bombchuBowlingStatus == 0) {
                play->bombchuBowlingStatus = -1;
            }
        } else {
            Inventory_ChangeAmmo(explosiveInfo->itemId, -1);
        }

        this->interactRangeActor = spawnedActor;
        this->heldActor = spawnedActor;
        this->getItemId = GI_NONE;
        this->leftHandRot.y = spawnedActor->shape.rot.y - this->actor.shape.rot.y;
        this->stateFlags1 |= PLAYER_STATE1_HOLDING_ACTOR;
    }
}

void Player_SetupHookshot(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_AIMING_FPS_ITEM;
    this->fpsItemType = PLAYER_FPSITEM_HOOKSHOT;

    this->heldActor =
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_ARMS_HOOK, this->actor.world.pos.x,
                           this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, 0);
}

void Player_SetupBoomerang(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_AIMING_BOOMERANG;
}

void Player_ChangeItem(PlayState* play, Player* this, s8 itemAction) {
    this->fpsItemType = PLAYER_FPSITEM_NONE;
    this->unk_85C = 0.0f;
    this->spinAttackTimer = 0.0f;

    this->heldItemAction = this->itemAction = itemAction;
    this->modelGroup = this->nextModelGroup;

    this->stateFlags1 &= ~(PLAYER_STATE1_AIMING_FPS_ITEM | PLAYER_STATE1_AIMING_BOOMERANG);

    sItemChangeFuncs[itemAction](play, this);

    Player_SetModelGroup(this, this->modelGroup);
}

void Player_MeleeAttack(Player* this, s32 attackFlag) {
    u16 itemSfx;
    u16 voiceSfx;

    if (this->isMeleeWeaponAttacking == false) {
        if ((this->heldItemAction == PLAYER_IA_SWORD_BGS) && (gSaveContext.swordHealth > 0.0f)) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else {
            itemSfx = NA_SE_IT_SWORD_SWING;
        }

        voiceSfx = NA_SE_VO_LI_SWORD_N;
        if (this->heldItemAction == PLAYER_IA_HAMMER) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else if (this->meleeAttackType >= PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
            itemSfx = 0;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        } else if (this->slashCounter >= 3) {
            itemSfx = NA_SE_IT_SWORD_SWING_HARD;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        }

        if (itemSfx != 0) {
            Player_PlayReactableSfx(this, itemSfx);
        }

        if (!((this->meleeAttackType >= PLAYER_MELEEATKTYPE_FLIPSLASH_START) &&
              (this->meleeAttackType <= PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH))) {
            Player_PlayVoiceSfxForAge(this, voiceSfx);
        }
    }

    this->isMeleeWeaponAttacking = attackFlag;
}

s32 Player_IsFriendlyZTargeting(Player* this) {
    if (this->stateFlags1 & (PLAYER_STATE1_FORCE_STRAFING | PLAYER_STATE1_Z_TARGETING_FRIENDLY | PLAYER_STATE1_30)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_SetupStartUnfriendlyZTargeting(Player* this) {
    if ((this->targetActor != NULL) && CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
        this->stateFlags1 |= PLAYER_STATE1_Z_TARGETING_UNFRIENDLY;
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_Z_TARGETING_UNFRIENDLY) {
        this->stateFlags1 &= ~PLAYER_STATE1_Z_TARGETING_UNFRIENDLY;
        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;
        }
    }

    return 0;
}

s32 Player_IsZTargeting(Player* this) {
    return Player_IsUnfriendlyZTargeting(this) || Player_IsFriendlyZTargeting(this);
}

s32 Player_IsZTargetingSetupStartUnfriendly(Player* this) {
    return Player_SetupStartUnfriendlyZTargeting(this) || Player_IsFriendlyZTargeting(this);
}

void Player_ResetLeftRightBlendWeight(Player* this) {
    this->leftRightBlendWeight = this->leftRightBlendWeightTarget = 0.0f;
}

s32 Player_IsItemValid(Player* this, s32 item) {
    if ((item < ITEM_NONE_FE) && (Player_ItemToItemAction(item) == this->itemAction)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_IsWearableMaskValid(s32 item1, s32 itemAction) {
    if ((item1 < ITEM_NONE_FE) && (Player_ItemToItemAction(item1) == itemAction)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_GetButtonItem(PlayState* play, s32 index) {
    if (index >= 4) {
        return ITEM_NONE;
    } else if (play->bombchuBowlingStatus != 0) {
        return (play->bombchuBowlingStatus > 0) ? ITEM_BOMBCHU : ITEM_NONE;
    } else if (index == 0) {
        return B_BTN_ITEM;
    } else if (index == 1) {
        return C_BTN_ITEM(0);
    } else if (index == 2) {
        return C_BTN_ITEM(1);
    } else {
        return C_BTN_ITEM(2);
    }
}

void Player_SetupUseItem(Player* this, PlayState* play) {
    s32 maskItemAction;
    s32 item;
    s32 i;

    if (this->currentMask != PLAYER_MASK_NONE) {
        maskItemAction = this->currentMask - 1 + PLAYER_IA_MASK_KEATON;
        if (!Player_IsWearableMaskValid(C_BTN_ITEM(0), maskItemAction) &&
            !Player_IsWearableMaskValid(C_BTN_ITEM(1), maskItemAction) &&
            !Player_IsWearableMaskValid(C_BTN_ITEM(2), maskItemAction)) {
            this->currentMask = PLAYER_MASK_NONE;
        }
    }

    if (!(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE)) &&
        !Player_IsShootingHookshot(this)) {
        if (this->itemAction >= PLAYER_IA_FISHING_POLE) {
            if (!Player_IsItemValid(this, B_BTN_ITEM) && !Player_IsItemValid(this, C_BTN_ITEM(0)) &&
                !Player_IsItemValid(this, C_BTN_ITEM(1)) && !Player_IsItemValid(this, C_BTN_ITEM(2))) {
                Player_UseItem(play, this, ITEM_NONE);
                return;
            }
        }

        for (i = 0; i < ARRAY_COUNT(sUseItemButtons); i++) {
            if (CHECK_BTN_ALL(sControlInput->press.button, sUseItemButtons[i])) {
                break;
            }
        }

        item = Player_GetButtonItem(play, i);
        if (item >= ITEM_NONE_FE) {
            for (i = 0; i < ARRAY_COUNT(sUseItemButtons); i++) {
                if (CHECK_BTN_ALL(sControlInput->cur.button, sUseItemButtons[i])) {
                    break;
                }
            }

            item = Player_GetButtonItem(play, i);
            if ((item < ITEM_NONE_FE) && (Player_ItemToItemAction(item) == this->heldItemAction)) {
                sUsingItemAlreadyInHand2 = true;
            }
        } else {
            this->heldItemButton = i;
            Player_UseItem(play, this, item);
        }
    }
}

void Player_SetupStartChangeItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 animLastFrame;
    f32 startFrame;
    f32 endFrame;
    f32 playSpeed;
    s32 itemChangeAnim;
    s8 itemAction;
    s32 nextAnimType;

    itemAction = Player_ItemToItemAction(this->heldItemId);
    Player_SetUpperActionFunc(this, Player_StartChangeItem);

    nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];
    itemChangeAnim =
        sAnimtypeToItemChangeAnims[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType];
    if ((itemAction == PLAYER_IA_BOTTLE) || (itemAction == PLAYER_IA_BOOMERANG) ||
        ((itemAction == PLAYER_IA_NONE) &&
         ((this->heldItemAction == PLAYER_IA_BOTTLE) || (this->heldItemAction == PLAYER_IA_BOOMERANG)))) {
        itemChangeAnim = (itemAction == PLAYER_IA_NONE) ? -PLAYER_ITEM_CHANGE_LEFT_HAND : PLAYER_ITEM_CHANGE_LEFT_HAND;
    }

    this->itemChangeAnim = ABS(itemChangeAnim);

    anim = sItemChangeAnimsInfo[this->itemChangeAnim].anim;
    if ((anim == &gPlayerAnim_link_normal_fighter2free) && (this->currentShield == PLAYER_SHIELD_NONE)) {
        anim = &gPlayerAnim_link_normal_free2fighter_free;
    }

    animLastFrame = Animation_GetLastFrame(anim);
    endFrame = animLastFrame;

    if (itemChangeAnim >= 0) {
        playSpeed = 1.2f;
        startFrame = 0.0f;
    } else {
        endFrame = 0.0f;
        playSpeed = -1.2f;
        startFrame = animLastFrame;
    }

    if (itemAction != PLAYER_IA_NONE) {
        playSpeed *= 2.0f;
    }

    LinkAnimation_Change(play, &this->skelAnimeUpper, anim, playSpeed, startFrame, endFrame, ANIMMODE_ONCE, 0.0f);

    this->stateFlags1 &= ~PLAYER_STATE1_START_CHANGE_ITEM;
}

void Player_SetupItem(Player* this, PlayState* play) {
    if ((this->actor.category == ACTORCAT_PLAYER) && !(this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) &&
        ((this->heldItemAction == this->itemAction) || (this->stateFlags1 & PLAYER_STATE1_22)) &&
        (gSaveContext.health != 0) && (play->csCtx.state == CS_STATE_IDLE) && (this->csMode == PLAYER_CSMODE_NONE) &&
        (play->shootingGalleryStatus == 0) && (play->activeCamId == CAM_ID_MAIN) &&
        (play->transitionTrigger != TRANS_TRIGGER_START) && (gSaveContext.timerState != TIMER_STATE_STOP)) {
        Player_SetupUseItem(this, play);
    }

    if (this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) {
        Player_SetupStartChangeItem(this, play);
    }
}

// Sets FPS item ID and type of ammo, then returns ammo count of the FPS item
s32 Player_GetFpsItemAmmo(PlayState* play, Player* this, s32* itemPtr, s32* typePtr) {
    if (LINK_IS_ADULT) {
        *itemPtr = ITEM_BOW;
        if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
            *typePtr = ARROW_NORMAL_HORSE;
        } else {
            *typePtr = ARROW_NORMAL + (this->heldItemAction - PLAYER_IA_BOW);
        }
    } else {
        *itemPtr = ITEM_SLINGSHOT;
        *typePtr = ARROW_SEED;
    }

    if (gSaveContext.minigameState == 1) {
        return play->interfaceCtx.hbaAmmo;
    } else if (play->shootingGalleryStatus != 0) {
        return play->shootingGalleryStatus;
    } else {
        return AMMO(*itemPtr);
    }
}

s32 Player_SetupReadyFpsItemToShoot(Player* this, PlayState* play) {
    s32 item;
    s32 arrowType;
    s32 magicArrowType;

    if ((this->heldItemAction >= PLAYER_IA_BOW_FIRE) && (this->heldItemAction <= PLAYER_IA_BOW_0E) &&
        (gSaveContext.magicState != MAGIC_STATE_IDLE)) {
        func_80078884(NA_SE_SY_ERROR);
    } else {
        Player_SetUpperActionFunc(this, Player_ReadyFpsItemToShoot);

        this->stateFlags1 |= PLAYER_STATE1_READY_TO_SHOOT;
        this->fpsItemTimer = 14;

        if (this->fpsItemType >= PLAYER_FPSITEM_NONE) {
            func_8002F7DC(&this->actor, sFpsItemReadySfx[ABS(this->fpsItemType) - 1]);

            if (!Player_HoldsHookshot(this) && (Player_GetFpsItemAmmo(play, this, &item, &arrowType) > 0)) {
                magicArrowType = arrowType - ARROW_FIRE;

                if (this->fpsItemType >= PLAYER_FPSITEM_NONE) {
                    if ((magicArrowType >= 0) && (magicArrowType <= 2) &&
                        !Magic_RequestChange(play, sMagicArrowCosts[magicArrowType], MAGIC_CONSUME_NOW)) {
                        arrowType = ARROW_NORMAL;
                    }

                    this->heldActor = Actor_SpawnAsChild(
                        &play->actorCtx, &this->actor, play, ACTOR_EN_ARROW, this->actor.world.pos.x,
                        this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, arrowType);
                }
            }
        }

        return 1;
    }

    return 0;
}

void Player_ChangeItemWithSfx(PlayState* play, Player* this) {
    if (this->heldItemAction != PLAYER_IA_NONE) {
        if (Player_GetSwordItemAction(this, this->heldItemAction) >= 0) {
            Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PUTAWAY);
        } else {
            Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
        }
    }

    Player_UseItem(play, this, this->heldItemId);

    if (Player_GetSwordItemAction(this, this->heldItemAction) >= 0) {
        Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PICKOUT);
    } else if (this->heldItemAction != PLAYER_IA_NONE) {
        Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
    }
}

void Player_SetupHeldItemUpperActionFunc(PlayState* play, Player* this) {
    if (Player_StartChangeItem == this->upperActionFunc) {
        Player_ChangeItemWithSfx(play, this);
    }

    Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemAction]);
    this->fpsItemTimer = 0;
    this->idleCounter = 0;
    Player_DetatchHeldActor(play, this);
    this->stateFlags1 &= ~PLAYER_STATE1_START_CHANGE_ITEM;
}

LinkAnimationHeader* Player_GetStandingDefendAnim(PlayState* play, Player* this) {
    Player_SetUpperActionFunc(this, Player_StandingDefend);
    Player_DetatchHeldActor(play, this);

    if (this->leftRightBlendWeight < 0.5f) {
        return sRightDefendStandingAnims[Player_HoldsTwoHandedWeapon(this)];
    } else {
        return sLeftDefendStandingAnims[Player_HoldsTwoHandedWeapon(this)];
    }
}

s32 Player_StartZTargetDefend(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    f32 frame;

    if (!(this->stateFlags1 & (PLAYER_STATE1_22 | PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_IN_CUTSCENE)) &&
        (play->shootingGalleryStatus == 0) && (this->heldItemAction == this->itemAction) &&
        (this->currentShield != PLAYER_SHIELD_NONE) && !Player_IsChildWithHylianShield(this) &&
        Player_IsZTargeting(this) && CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {

        anim = Player_GetStandingDefendAnim(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->skelAnimeUpper, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
        func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_POSTURE);

        return 1;
    } else {
        return 0;
    }
}

s32 Player_SetupStartZTargetDefend(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    } else {
        return 0;
    }
}

void Player_SetupEndDefend(Player* this) {
    Player_SetUpperActionFunc(this, Player_EndDefend);

    if (this->itemAction < 0) {
        Player_SetHeldItem(this);
    }

    Animation_Reverse(&this->skelAnimeUpper);
    func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_REMOVE);
}

void Player_SetupChangeItem(PlayState* play, Player* this) {
    ItemChangeAnimInfo* itemChangeInfo = &sItemChangeAnimsInfo[this->itemChangeAnim];
    f32 itemChangeFrame;

    itemChangeFrame = itemChangeInfo->itemChangeFrame;
    itemChangeFrame = (this->skelAnimeUpper.playSpeed < 0.0f) ? itemChangeFrame - 1.0f : itemChangeFrame;

    if (LinkAnimation_OnFrame(&this->skelAnimeUpper, itemChangeFrame)) {
        Player_ChangeItemWithSfx(play, this);
    }

    Player_SetupStartUnfriendlyZTargeting(this);
}

s32 func_8083499C(Player* this, PlayState* play) {
    // Never passes due to Player_SetupItem running first, which ends up in PLAYER_STATE1_START_CHANGE_ITEM being unset
    if (this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) {
        Player_SetupStartChangeItem(this, play);
    } else {
        return 0;
    }

    return 1;
}

s32 Player_SetupStartZTargetDefend2(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this) || func_8083499C(this, play)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_StartChangeItem(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnimeUpper) ||
        ((Player_ItemToItemAction(this->heldItemId) == this->heldItemAction) &&
         (sUsingItemAlreadyInHand =
              (sUsingItemAlreadyInHand || ((this->modelAnimType != PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON) &&
                                           (play->shootingGalleryStatus == 0)))))) {
        Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemAction]);
        this->fpsItemTimer = 0;
        this->idleCounter = 0;
        sUsingItemAlreadyInHand2 = sUsingItemAlreadyInHand;
        return this->upperActionFunc(this, play);
    }

    if (Player_IsPlayingIdleAnim(this) != 0) {
        Player_SetupChangeItem(play, this);
        Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
        this->idleCounter = 0;
    } else {
        Player_SetupChangeItem(play, this);
    }

    return 1;
}

s32 Player_StandingDefend(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnimeUpper);

    if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
        Player_SetupEndDefend(this);
        return 1;
    } else {
        this->stateFlags1 |= PLAYER_STATE1_22;
        Player_SetModelsForHoldingShield(this);
        return 1;
    }
}

s32 Player_EndDeflectAttackStanding(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;

    if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        anim = Player_GetStandingDefendAnim(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->skelAnimeUpper, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
    }

    this->stateFlags1 |= PLAYER_STATE1_22;
    Player_SetModelsForHoldingShield(this);

    return 1;
}

s32 Player_EndDefend(Player* this, PlayState* play) {
    sUsingItemAlreadyInHand = sUsingItemAlreadyInHand2;

    if (sUsingItemAlreadyInHand || LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemAction]);
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper,
                               GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType));
        this->idleCounter = 0;
        this->upperActionFunc(this, play);
        return 0;
    }

    return 1;
}

s32 Player_SetupUseFpsItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    if (this->heldItemAction != PLAYER_IA_BOOMERANG) {
        if (!Player_SetupReadyFpsItemToShoot(this, play)) {
            return 0;
        }

        if (!Player_HoldsHookshot(this)) {
            anim = &gPlayerAnim_link_bow_bow_ready;
        } else {
            anim = &gPlayerAnim_link_hook_shot_ready;
        }
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, anim);
    } else {
        Player_SetUpperActionFunc(this, Player_SetupAimBoomerang);
        this->fpsItemTimer = 10;
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_link_boom_throw_wait2waitR);
    }

    if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_uma_anim_walk);
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !Player_SetupStartUnfriendlyZTargeting(this)) {
        Player_PlayAnimLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType));
    }

    return 1;
}

s32 Player_CheckShootingGalleryShootInput(PlayState* play) {
    return (play->shootingGalleryStatus > 0) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B);
}

s32 func_80834E7C(PlayState* play) {
    return (play->shootingGalleryStatus != 0) &&
           ((play->shootingGalleryStatus < 0) ||
            CHECK_BTN_ANY(sControlInput->cur.button, BTN_A | BTN_B | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN));
}

s32 Player_SetupAimAttention(Player* this, PlayState* play) {
    if ((this->attentionMode == PLAYER_ATTENTIONMODE_NONE) || (this->attentionMode == PLAYER_ATTENTIONMODE_AIMING)) {
        if (Player_IsZTargeting(this) ||
            (Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_BOWARROW) == 0)) {
            return 1;
        }
        this->attentionMode = PLAYER_ATTENTIONMODE_AIMING;
    }

    return 0;
}

s32 Player_CanUseFpsItem(Player* this, PlayState* play) {
    if ((this->doorType == PLAYER_DOORTYPE_NONE) && !(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG)) {
        if (sUsingItemAlreadyInHand || Player_CheckShootingGalleryShootInput(play)) {
            if (Player_SetupUseFpsItem(this, play)) {
                return Player_SetupAimAttention(this, play);
            }
        }
    }

    return 0;
}

s32 Player_EndHookshotMove(Player* this) {
    if (this->actor.child != NULL) {
        if (this->heldActor == NULL) {
            this->heldActor = this->actor.child;
            Player_RequestRumble(this, 255, 10, 250, 0);
            func_8002F7DC(&this->actor, NA_SE_IT_HOOKSHOT_RECEIVE);
        }

        return 1;
    }

    return 0;
}

s32 Player_HoldFpsItem(Player* this, PlayState* play) {
    if (this->fpsItemType >= PLAYER_FPSITEM_NONE) {
        this->fpsItemType = -this->fpsItemType;
    }

    if ((!Player_HoldsHookshot(this) || Player_EndHookshotMove(this)) && !Player_StartZTargetDefend(play, this) &&
        !Player_CanUseFpsItem(this, play)) {
        return 0;
    }

    return 1;
}

s32 Player_UpdateShotFpsItem(PlayState* play, Player* this) {
    s32 item;
    s32 arrowType;

    if (this->heldActor != NULL) {
        if (!Player_HoldsHookshot(this)) {
            Player_GetFpsItemAmmo(play, this, &item, &arrowType);

            if (gSaveContext.minigameState == 1) {
                play->interfaceCtx.hbaAmmo--;
            } else if (play->shootingGalleryStatus != 0) {
                play->shootingGalleryStatus--;
            } else {
                Inventory_ChangeAmmo(item, -1);
            }

            if (play->shootingGalleryStatus == 1) {
                play->shootingGalleryStatus = -10;
            }

            Player_RequestRumble(this, 150, 10, 150, 0);
        } else {
            Player_RequestRumble(this, 255, 20, 150, 0);
        }

        this->fpsItemShotTimer = 4;
        this->heldActor->parent = NULL;
        this->actor.child = NULL;
        this->heldActor = NULL;

        return 1;
    }

    return 0;
}

static u16 sFpsItemNoAmmoSfx[] = { NA_SE_IT_BOW_FLICK, NA_SE_IT_SLING_FLICK };

s32 Player_ReadyFpsItemToShoot(Player* this, PlayState* play) {
    s32 holdingHookshot;

    if (!Player_HoldsHookshot(this)) {
        holdingHookshot = false;
    } else {
        holdingHookshot = true;
    }

    Math_ScaledStepToS(&this->upperBodyRot.z, 1200, 400);
    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Z;

    if ((this->fpsItemShootState == 0) && (Player_IsPlayingIdleAnim(this) == 0) &&
        (this->skelAnime.animation == &gPlayerAnim_link_bow_side_walk)) {
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, sReadyFpsItemWhileWalkingAnims[holdingHookshot]);
        this->fpsItemShootState = -1;
    } else if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, sReadyFpsItemAnims[holdingHookshot]);
        this->fpsItemShootState = 1;
    } else if (this->fpsItemShootState == 1) {
        this->fpsItemShootState = 2;
    }

    if (this->fpsItemTimer > 10) {
        this->fpsItemTimer--;
    }

    Player_SetupAimAttention(this, play);

    if ((this->fpsItemShootState > 0) &&
        ((this->fpsItemType < PLAYER_FPSITEM_NONE) || (!sUsingItemAlreadyInHand2 && !func_80834E7C(play)))) {
        Player_SetUpperActionFunc(this, Player_AimFpsItem);
        if (this->fpsItemType >= PLAYER_FPSITEM_NONE) {
            if (holdingHookshot == false) {
                if (!Player_UpdateShotFpsItem(play, this)) {
                    func_8002F7DC(&this->actor, sFpsItemNoAmmoSfx[ABS(this->fpsItemType) - 1]);
                }
            } else if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_UpdateShotFpsItem(play, this);
            }
        }
        this->fpsItemTimer = 10;
        Player_StopMovement(this);
    } else {
        this->stateFlags1 |= PLAYER_STATE1_READY_TO_SHOOT;
    }

    return 1;
}

s32 Player_AimFpsItem(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnimeUpper);

    if (Player_HoldsHookshot(this) && !Player_EndHookshotMove(this)) {
        return 1;
    }

    if (!Player_StartZTargetDefend(play, this) &&
        (sUsingItemAlreadyInHand || ((this->fpsItemType < PLAYER_FPSITEM_NONE) && sUsingItemAlreadyInHand2) ||
         Player_CheckShootingGalleryShootInput(play))) {
        this->fpsItemType = ABS(this->fpsItemType);

        if (Player_SetupReadyFpsItemToShoot(this, play)) {
            if (Player_HoldsHookshot(this)) {
                this->fpsItemShootState = 1;
            } else {
                LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_link_bow_bow_shoot_next);
            }
        }
    } else {
        if (this->fpsItemTimer != 0) {
            this->fpsItemTimer--;
        }

        if (Player_IsZTargeting(this) || (this->attentionMode != PLAYER_ATTENTIONMODE_NONE) ||
            (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
            if (this->fpsItemTimer == 0) {
                this->fpsItemTimer++;
            }
            return 1;
        }

        if (Player_HoldsHookshot(this)) {
            Player_SetUpperActionFunc(this, Player_HoldFpsItem);
        } else {
            Player_SetUpperActionFunc(this, Player_EndAimFpsItem);
            LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_link_bow_bow_shoot_end);
        }

        this->fpsItemTimer = 0;
    }

    return 1;
}

s32 Player_EndAimFpsItem(Player* this, PlayState* play) {
    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        Player_SetUpperActionFunc(this, Player_HoldFpsItem);
    }

    return 1;
}

void Player_SetZTargetFriendlyYaw(Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_Z_TARGETING_FRIENDLY;

    if (!(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_7) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sYawToTouchedWall < DEG_TO_BINANG(45.0f))) {
        this->currentYaw = this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
    }

    this->targetYaw = this->actor.shape.rot.y;
}

s32 Player_InterruptHoldingActor(PlayState* play, Player* this, Actor* heldActor) {
    if (heldActor == NULL) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetupStandingStillType(this, play);
        return 1;
    }

    return 0;
}

void Player_SetupHoldActorUpperAction(Player* this, PlayState* play) {
    if (!Player_InterruptHoldingActor(play, this, this->heldActor)) {
        Player_SetUpperActionFunc(this, Player_HoldActor);
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, &gPlayerAnim_link_normal_carryB_wait);
    }
}

s32 Player_HoldActor(Player* this, PlayState* play) {
    Actor* heldActor = this->heldActor;

    if (heldActor == NULL) {
        Player_SetupHeldItemUpperActionFunc(play, this);
    }

    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
            LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, &gPlayerAnim_link_normal_carryB_wait);
        }

        if ((heldActor->id == ACTOR_EN_NIW) && (this->actor.velocity.y <= 0.0f)) {
            this->actor.minVelocityY = -2.0f;
            this->actor.gravity = -0.5f;
            this->fallStartHeight = this->actor.world.pos.y;
        }

        return 1;
    }

    return Player_SetupStartZTargetDefend(this, play);
}

void Player_SetLeftHandDlists(Player* this, Gfx** dLists) {
    this->leftHandDLists = dLists + gSaveContext.linkAge;
}

s32 Player_HoldBoomerang(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) {
        Player_SetUpperActionFunc(this, Player_WaitForThrownBoomerang);
    } else if (Player_CanUseFpsItem(this, play)) {
        return 1;
    }

    return 0;
}

s32 Player_SetupAimBoomerang(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        Player_SetUpperActionFunc(this, Player_AimBoomerang);
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, &gPlayerAnim_link_boom_throw_waitR);
    }

    Player_SetupAimAttention(this, play);

    return 1;
}

s32 Player_AimBoomerang(Player* this, PlayState* play) {
    LinkAnimationHeader* animSeg = this->skelAnime.animation;

    if ((Player_GetFightingRightAnim(this) == animSeg) || (Player_GetFightingLeftAnim(this) == animSeg) ||
        (Player_GetSidewalkRightAnim(this) == animSeg) || (Player_GetSidewalkLeftAnim(this) == animSeg)) {
        AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnimeUpper.jointTable,
                                    this->skelAnime.jointTable);
    } else {
        LinkAnimation_Update(play, &this->skelAnimeUpper);
    }

    Player_SetupAimAttention(this, play);

    if (!sUsingItemAlreadyInHand2) {
        Player_SetUpperActionFunc(this, Player_ThrowBoomerang);
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper,
                               (this->leftRightBlendWeight < 0.5f) ? &gPlayerAnim_link_boom_throwR
                                                                   : &gPlayerAnim_link_boom_throwL);
    }

    return 1;
}

s32 Player_ThrowBoomerang(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        Player_SetUpperActionFunc(this, Player_WaitForThrownBoomerang);
        this->fpsItemTimer = 0;
    } else if (LinkAnimation_OnFrame(&this->skelAnimeUpper, 6.0f)) {
        f32 posX = (Math_SinS(this->actor.shape.rot.y) * 10.0f) + this->actor.world.pos.x;
        f32 posZ = (Math_CosS(this->actor.shape.rot.y) * 10.0f) + this->actor.world.pos.z;
        s32 yaw = (this->targetActor != NULL) ? this->actor.shape.rot.y + 14000 : this->actor.shape.rot.y;
        EnBoom* boomerang =
            (EnBoom*)Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BOOM, posX, this->actor.world.pos.y + 30.0f, posZ,
                                 this->actor.focus.rot.x, yaw, 0, 0);

        this->boomerangActor = &boomerang->actor;
        if (boomerang != NULL) {
            boomerang->moveTo = this->targetActor;
            boomerang->returnTimer = 20;
            this->stateFlags1 |= PLAYER_STATE1_AWAITING_THROWN_BOOMERANG;
            if (!Player_IsUnfriendlyZTargeting(this)) {
                Player_SetZTargetFriendlyYaw(this);
            }
            this->fpsItemShotTimer = 4;
            func_8002F7DC(&this->actor, NA_SE_IT_BOOMERANG_THROW);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        }
    }

    return 1;
}

s32 Player_WaitForThrownBoomerang(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG)) {
        Player_SetUpperActionFunc(this, Player_CatchBoomerang);
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_link_boom_catch);
        Player_SetLeftHandDlists(this, gPlayerLeftHandBoomerangDLs);
        func_8002F7DC(&this->actor, NA_SE_PL_CATCH_BOOMERANG);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        return 1;
    }

    return 0;
}

s32 Player_CatchBoomerang(Player* this, PlayState* play) {
    if (!Player_HoldBoomerang(this, play) && LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        Player_SetUpperActionFunc(this, Player_HoldBoomerang);
    }

    return 1;
}

s32 Player_SetActionFunc(PlayState* play, Player* this, PlayerActionFunc func, s32 flag) {
    if (func == this->actionFunc) {
        return 0;
    }

    if (Player_PlayOcarina == this->actionFunc) {
        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
        this->stateFlags2 &= ~(PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
    } else if (Player_UpdateMagicSpell == this->actionFunc) {
        Player_ResetSubCam(play, this);
    }

    this->actionFunc = func;

    if ((this->itemAction != this->heldItemAction) && (!(flag & 1) || !(this->stateFlags1 & PLAYER_STATE1_22))) {
        Player_SetHeldItem(this);
    }

    if (!(flag & 1) && !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
        Player_SetupHeldItemUpperActionFunc(play, this);
        this->stateFlags1 &= ~PLAYER_STATE1_22;
    }

    Player_EndAnimMovement(this);
    this->stateFlags1 &= ~(PLAYER_STATE1_END_HOOKSHOT_MOVE | PLAYER_STATE1_TALKING | PLAYER_STATE1_TAKING_DAMAGE |
                           PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE |
                           PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID);
    this->stateFlags2 &=
        ~(PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING | PLAYER_STATE2_PLAYING_OCARINA_GENERAL | PLAYER_STATE2_IDLING);
    this->stateFlags3 &=
        ~(PLAYER_STATE3_MIDAIR | PLAYER_STATE3_ENDING_MELEE_ATTACK | PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH);
    this->genericVar = 0;
    this->genericTimer = 0;
    this->idleCounter = 0;
    Player_StopInterruptableSfx(this);

    return 1;
}

void Player_SetActionFuncPreserveMoveFlags(PlayState* play, Player* this, PlayerActionFunc func, s32 flag) {
    s32 flagsToRestore;

    flagsToRestore = this->skelAnime.moveFlags;
    this->skelAnime.moveFlags = 0;
    Player_SetActionFunc(play, this, func, flag);
    this->skelAnime.moveFlags = flagsToRestore;
}

void Player_SetActionFuncPreserveItemAP(PlayState* play, Player* this, PlayerActionFunc func, s32 flag) {
    s32 itemAPToRestore;

    if (this->itemAction >= 0) {
        itemAPToRestore = this->itemAction;
        this->itemAction = this->heldItemAction;
        Player_SetActionFunc(play, this, func, flag);
        this->itemAction = itemAPToRestore;
        Player_SetModels(this, Player_ActionToModelGroup(this, this->itemAction));
    }
}

void Player_ChangeCameraSetting(PlayState* play, s16 camSetting) {
    if (!Play_CamIsNotFixed(play)) {
        if (camSetting == CAM_SET_SCENE_TRANSITION) {
            Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING_ALT);
        }
    } else {
        Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), camSetting);
    }
}

void Player_SetCameraTurnAround(PlayState* play, s32 arg1) {
    Player_ChangeCameraSetting(play, CAM_SET_TURN_AROUND);
    Camera_SetCameraData(Play_GetCamera(play, CAM_ID_MAIN), 4, NULL, NULL, arg1, 0, 0);
}

void Player_PutAwayHookshot(Player* this) {
    if (Player_HoldsHookshot(this)) {
        Actor* heldActor = this->heldActor;

        if (heldActor != NULL) {
            Actor_Kill(heldActor);
            this->actor.child = NULL;
            this->heldActor = NULL;
        }
    }
}

void Player_UseItem(PlayState* play, Player* this, s32 item) {
    s8 itemAction;
    s32 temp;
    s32 nextAnimType;

    itemAction = Player_ItemToItemAction(item);

    if (((this->heldItemAction == this->itemAction) &&
         (!(this->stateFlags1 & PLAYER_STATE1_22) ||
          (Player_ActionToMeleeWeapon(itemAction) != PLAYER_MELEEWEAPON_NONE) || (itemAction == PLAYER_IA_NONE))) ||
        ((this->itemAction < 0) &&
         ((Player_ActionToMeleeWeapon(itemAction) != PLAYER_MELEEWEAPON_NONE) || (itemAction == PLAYER_IA_NONE)))) {

        if ((itemAction == PLAYER_IA_NONE) || !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
            ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
             ((itemAction == PLAYER_IA_HOOKSHOT) || (itemAction == PLAYER_IA_LONGSHOT)))) {

            if ((play->bombchuBowlingStatus == 0) &&
                (((itemAction == PLAYER_IA_DEKU_STICK) && (AMMO(ITEM_DEKU_STICK) == 0)) ||
                 ((itemAction == PLAYER_IA_MAGIC_BEAN) && (AMMO(ITEM_MAGIC_BEAN) == 0)) ||
                 (temp = Player_ActionToExplosive(this, itemAction),
                  ((temp >= 0) && ((AMMO(sExplosiveInfos[temp].itemId) == 0) ||
                                   (play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].length >= 3)))))) {
                func_80078884(NA_SE_SY_ERROR);
            } else if (itemAction == PLAYER_IA_LENS_OF_TRUTH) {
                if (Magic_RequestChange(play, 0, MAGIC_CONSUME_LENS)) {
                    if (play->actorCtx.lensActive) {
                        Actor_DisableLens(play);
                    } else {
                        play->actorCtx.lensActive = true;
                    }

                    func_80078884((play->actorCtx.lensActive) ? NA_SE_SY_GLASSMODE_ON : NA_SE_SY_GLASSMODE_OFF);
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if (itemAction == PLAYER_IA_DEKU_NUT) {
                if (AMMO(ITEM_DEKU_NUT) != 0) {
                    Player_SetupThrowDekuNut(play, this);
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if ((temp = Player_ActionToMagicSpell(this, itemAction)) >= 0) {
                if (((itemAction == PLAYER_IA_FARORES_WIND) && (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 0)) ||
                    ((gSaveContext.magicCapacity != 0) && (gSaveContext.magicState == MAGIC_STATE_IDLE) &&
                     (gSaveContext.magic >= sMagicSpellCosts[temp]))) {
                    this->itemAction = itemAction;
                    this->attentionMode = PLAYER_ATTENTIONMODE_ITEM_CUTSCENE;
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if (itemAction >= PLAYER_IA_MASK_KEATON) {
                if (this->currentMask != PLAYER_MASK_NONE) {
                    this->currentMask = PLAYER_MASK_NONE;
                } else {
                    this->currentMask = itemAction - PLAYER_IA_MASK_KEATON + 1;
                }

                Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
            } else if (((itemAction >= PLAYER_IA_OCARINA_FAIRY) && (itemAction <= PLAYER_IA_OCARINA_OF_TIME)) ||
                       (itemAction >= PLAYER_IA_BOTTLE_FISH)) {
                if (!Player_IsUnfriendlyZTargeting(this) ||
                    ((itemAction >= PLAYER_IA_BOTTLE_POTION_RED) && (itemAction <= PLAYER_IA_BOTTLE_FAIRY))) {
                    TitleCard_Clear(play, &play->actorCtx.titleCtx);
                    this->attentionMode = PLAYER_ATTENTIONMODE_ITEM_CUTSCENE;
                    this->itemAction = itemAction;
                }
            } else if ((itemAction != this->heldItemAction) ||
                       ((this->heldActor == NULL) && (Player_ActionToExplosive(this, itemAction) >= 0))) {
                this->nextModelGroup = Player_ActionToModelGroup(this, itemAction);
                nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];

                if ((this->heldItemAction >= 0) && (Player_ActionToMagicSpell(this, itemAction) < 0) &&
                    (item != this->heldItemId) &&
                    (sAnimtypeToItemChangeAnims[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]]
                                               [nextAnimType] != PLAYER_ITEM_CHANGE_DEFAULT)) {
                    this->heldItemId = item;
                    this->stateFlags1 |= PLAYER_STATE1_START_CHANGE_ITEM;
                } else {
                    Player_PutAwayHookshot(this);
                    Player_DetatchHeldActor(play, this);
                    Player_SetupChangeItemAnim(play, this, itemAction);
                }
            } else {
                sUsingItemAlreadyInHand = sUsingItemAlreadyInHand2 = true;
            }
        }
    }
}

void Player_SetupDie(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    s32 isSwimming = Player_IsSwimming(this);

    Player_ResetAttributesAndHeldActor(play, this);

    Player_SetActionFunc(play, this, isSwimming ? Player_Drown : Player_Die, 0);

    this->stateFlags1 |= PLAYER_STATE1_IN_DEATH_CUTSCENE;

    Player_PlayAnimOnce(play, this, anim);
    if (anim == &gPlayerAnim_link_derth_rebirth) {
        this->skelAnime.endFrame = 84.0f;
    }

    Player_ClearAttentionModeAndStopMoving(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DOWN);

    if (this->actor.category == ACTORCAT_PLAYER) {
        Audio_SetBgmVolumeOffDuringFanfare();

        if (Inventory_ConsumeFairy(play)) {
            play->gameOverCtx.state = GAMEOVER_REVIVE_START;
            this->fairyReviveFlag = 1;
        } else {
            play->gameOverCtx.state = GAMEOVER_DEATH_START;
            Audio_StopBgmAndFanfare(0);
            Audio_PlayFanfare(NA_BGM_GAME_OVER);
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
        }

        OnePointCutscene_Init(play, 9806, isSwimming ? 120 : 60, &this->actor, CAM_ID_MAIN);
        Letterbox_SetSizeTarget(32);
    }
}

s32 Player_CanUseItem(Player* this) {
    return (!(Player_RunMiniCutsceneFunc == this->actionFunc) ||
            ((this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) &&
             ((this->heldItemId == ITEM_LAST_USED) || (this->heldItemId == ITEM_NONE)))) &&
           (!(Player_StartChangeItem == this->upperActionFunc) ||
            (Player_ItemToItemAction(this->heldItemId) == this->heldItemAction));
}

s32 Player_SetupCurrentUpperAction(Player* this, PlayState* play) {
    if (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->actor.parent != NULL) &&
        Player_HoldsHookshot(this)) {
        Player_SetActionFunc(play, this, Player_MoveAlongHookshotPath, 1);
        this->stateFlags3 |= PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH;
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_hook_fly_start);
        Player_SetupAnimMovement(play, this,
                                 PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                     PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                     PLAYER_ANIMMOVEFLAGS_7);
        Player_ClearAttentionModeAndStopMoving(this);
        this->currentYaw = this->actor.shape.rot.y;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        this->hoverBootsTimer = 0;
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y |
                           PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
        return 1;
    }

    if (Player_CanUseItem(this)) {
        Player_SetupItem(this, play);
        if (Player_ThrowDekuNut == this->actionFunc) {
            return 1;
        }
    }

    if (!this->upperActionFunc(this, play)) {
        return 0;
    }

    if (this->upperInterpWeight != 0.0f) {
        if ((Player_IsPlayingIdleAnim(this) == 0) || (this->linearVelocity != 0.0f)) {
            AnimationContext_SetCopyFalse(play, this->skelAnime.limbCount, this->skelAnimeUpper.jointTable,
                                          this->skelAnime.jointTable, D_80853410);
        }
        Math_StepToF(&this->upperInterpWeight, 0.0f, 0.25f);
        AnimationContext_SetInterp(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                   this->skelAnimeUpper.jointTable, 1.0f - this->upperInterpWeight);
    } else if ((Player_IsPlayingIdleAnim(this) == 0) || (this->linearVelocity != 0.0f)) {
        AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                     this->skelAnimeUpper.jointTable, D_80853410);
    } else {
        AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                    this->skelAnimeUpper.jointTable);
    }

    return 1;
}

s32 Player_SetupMiniCsFunc(PlayState* play, Player* this, PlayerMiniCsFunc func) {
    this->miniCsFunc = func;
    Player_SetActionFunc(play, this, Player_RunMiniCutsceneFunc, 0);
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    return Player_UnequipItem(play, this);
}

void Player_UpdateYaw(Player* this, PlayState* play) {
    s16 previousYaw = this->actor.shape.rot.y;

    if (!(this->stateFlags2 &
          (PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION))) {
        if ((this->targetActor != NULL) &&
            ((play->actorCtx.targetCtx.unk_4B != 0) || (this->actor.category != ACTORCAT_PLAYER))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y,
                               Math_Vec3f_Yaw(&this->actor.world.pos, &this->targetActor->focus.pos), 4000);
        } else if ((this->stateFlags1 & PLAYER_STATE1_Z_TARGETING_FRIENDLY) &&
                   !(this->stateFlags2 & (PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING |
                                          PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, this->targetYaw, 4000);
        }
    } else if (!(this->stateFlags2 & PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION)) {
        Math_ScaledStepToS(&this->actor.shape.rot.y, this->currentYaw, 2000);
    }

    this->unk_87C = this->actor.shape.rot.y - previousYaw;
}

// Steps angle based on offset from referenceAngle, then returns any excess angle difference beyond angleMinMax
s32 Player_StepAngleWithOffset(s16* angle, s16 target, s16 step, s16 angleMinMax, s16 referenceAngle,
                               s16 angleDiffMinMax) {
    s16 angleDiff;
    s16 clampedAngleDiff;
    s16 originalAngle;

    angleDiff = clampedAngleDiff = referenceAngle - *angle;
    clampedAngleDiff = CLAMP(clampedAngleDiff, -angleDiffMinMax, angleDiffMinMax);
    *angle += (s16)(angleDiff - clampedAngleDiff);

    Math_ScaledStepToS(angle, target, step);

    originalAngle = *angle;
    if (*angle < -angleMinMax) {
        *angle = -angleMinMax;
    } else if (*angle > angleMinMax) {
        *angle = angleMinMax;
    }
    return originalAngle - *angle;
}

s32 Player_UpdateLookAngles(Player* this, s32 syncUpperRotToFocusRot) {
    s16 yawDiff;
    s16 lookYaw;

    lookYaw = this->actor.shape.rot.y;
    if (syncUpperRotToFocusRot != false) {
        lookYaw = this->actor.focus.rot.y;
        this->upperBodyRot.x = this->actor.focus.rot.x;
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
    } else {
        Player_StepAngleWithOffset(
            &this->upperBodyRot.x,
            Player_StepAngleWithOffset(&this->headRot.x, this->actor.focus.rot.x, DEG_TO_BINANG(3.3f),
                                       DEG_TO_BINANG(54.932f), this->actor.focus.rot.x, 0),
            DEG_TO_BINANG(1.1f), DEG_TO_BINANG(21.973f), this->headRot.x, DEG_TO_BINANG(54.932f));
        yawDiff = this->actor.focus.rot.y - lookYaw;
        Player_StepAngleWithOffset(&yawDiff, 0, DEG_TO_BINANG(1.1f), DEG_TO_BINANG(131.84f), this->upperBodyRot.y,
                                   DEG_TO_BINANG(43.95f));
        lookYaw = this->actor.focus.rot.y - yawDiff;
        Player_StepAngleWithOffset(&this->headRot.y, yawDiff - this->upperBodyRot.y, DEG_TO_BINANG(1.1f),
                                   DEG_TO_BINANG(43.95f), yawDiff, DEG_TO_BINANG(43.95f));
        Player_StepAngleWithOffset(&this->upperBodyRot.y, yawDiff, DEG_TO_BINANG(1.1f), DEG_TO_BINANG(43.95f),
                                   this->headRot.y, DEG_TO_BINANG(43.95f));
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X |
                           PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_X |
                           PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
    }

    return lookYaw;
}

void Player_SetupZTargeting(Player* this, PlayState* play) {
    s32 isRangeCheckDisabled = false;
    s32 zTrigPressed = CHECK_BTN_ALL(sControlInput->cur.button, BTN_Z);
    Actor* actorToTarget;
    s32 pad;
    s32 holdTarget;
    s32 actorRequestingTalk;

    if (!zTrigPressed) {
        this->stateFlags1 &= ~PLAYER_STATE1_30;
    }

    if ((play->csCtx.state != CS_STATE_IDLE) || (this->csMode != PLAYER_CSMODE_NONE) ||
        (this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_IN_CUTSCENE)) ||
        (this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        this->targetSwitchTimer = 0;
    } else if (zTrigPressed || (this->stateFlags2 & PLAYER_STATE2_USING_SWITCH_Z_TARGETING) ||
               (this->forcedTargetActor != NULL)) {
        if (this->targetSwitchTimer <= 5) {
            this->targetSwitchTimer = 5;
        } else {
            this->targetSwitchTimer--;
        }
    } else if (this->stateFlags1 & PLAYER_STATE1_Z_TARGETING_FRIENDLY) {
        this->targetSwitchTimer = 0;
    } else if (this->targetSwitchTimer != 0) {
        this->targetSwitchTimer--;
    }

    if (this->targetSwitchTimer >= 6) {
        isRangeCheckDisabled = true;
    }

    actorRequestingTalk = Player_CheckActorTalkRequested(play);
    if (actorRequestingTalk || (this->targetSwitchTimer != 0) ||
        (this->stateFlags1 & (PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_AWAITING_THROWN_BOOMERANG))) {
        if (!actorRequestingTalk) {
            if (!(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) &&
                ((this->heldItemAction != PLAYER_IA_FISHING_POLE) || (this->fpsItemType == PLAYER_FPSITEM_NONE)) &&
                CHECK_BTN_ALL(sControlInput->press.button, BTN_Z)) {

                if (this->actor.category == ACTORCAT_PLAYER) {
                    actorToTarget = play->actorCtx.targetCtx.arrowPointedActor;
                } else {
                    actorToTarget = &GET_PLAYER(play)->actor;
                }

                holdTarget = (gSaveContext.zTargetSetting != 0) || (this->actor.category != ACTORCAT_PLAYER);
                this->stateFlags1 |= PLAYER_STATE1_UNUSED_Z_TARGETING_FLAG;

                if ((actorToTarget != NULL) && !(actorToTarget->flags & ACTOR_FLAG_27)) {
                    if ((actorToTarget == this->targetActor) && (this->actor.category == ACTORCAT_PLAYER)) {
                        actorToTarget = play->actorCtx.targetCtx.unk_94;
                    }

                    if (actorToTarget != this->targetActor) {
                        if (!holdTarget) {
                            this->stateFlags2 |= PLAYER_STATE2_USING_SWITCH_Z_TARGETING;
                        }
                        this->targetActor = actorToTarget;
                        this->targetSwitchTimer = 15;
                        this->stateFlags2 &= ~(PLAYER_STATE2_CAN_SPEAK_OR_CHECK | PLAYER_STATE2_NAVI_REQUESTING_TALK);
                    } else {
                        if (!holdTarget) {
                            Player_ForceDisableZTargeting(this);
                        }
                    }

                    this->stateFlags1 &= ~PLAYER_STATE1_30;
                } else {
                    if (!(this->stateFlags1 & (PLAYER_STATE1_Z_TARGETING_FRIENDLY | PLAYER_STATE1_30))) {
                        Player_SetZTargetFriendlyYaw(this);
                    }
                }
            }

            if (this->targetActor != NULL) {
                if ((this->actor.category == ACTORCAT_PLAYER) && (this->targetActor != this->forcedTargetActor) &&
                    Actor_OutsideTargetRange(this->targetActor, this, isRangeCheckDisabled)) {
                    Player_ForceDisableZTargeting(this);
                    this->stateFlags1 |= PLAYER_STATE1_30;
                } else if (this->targetActor != NULL) {
                    this->targetActor->targetPriority = 40;
                }
            } else if (this->forcedTargetActor != NULL) {
                this->targetActor = this->forcedTargetActor;
            }
        }

        if (this->targetActor != NULL) {
            this->stateFlags1 &= ~(PLAYER_STATE1_FORCE_STRAFING | PLAYER_STATE1_Z_TARGETING_FRIENDLY);
            if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                !CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
                this->stateFlags1 |= PLAYER_STATE1_FORCE_STRAFING;
            }
        } else {
            if (this->stateFlags1 & PLAYER_STATE1_Z_TARGETING_FRIENDLY) {
                this->stateFlags2 &= ~PLAYER_STATE2_USING_SWITCH_Z_TARGETING;
            } else {
                func_8008EE08(this);
            }
        }
    } else {
        func_8008EE08(this);
    }
}

s32 Player_CalculateTargetVelocityAndYaw(PlayState* play, Player* this, f32* targetVelocity, s16* targetYaw, f32 arg4) {
    f32 baseSpeedScale;
    f32 slope;
    f32 slopeSpeedScale;
    f32 speedLimit;

    if ((this->attentionMode != PLAYER_ATTENTIONMODE_NONE) || (play->transitionTrigger == TRANS_TRIGGER_START) ||
        (this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE)) {
        *targetVelocity = 0.0f;
        *targetYaw = this->actor.shape.rot.y;
    } else {
        *targetVelocity = sAnalogStickDistance;
        *targetYaw = sAnalogStickAngle;

        if (arg4 != 0.0f) {
            *targetVelocity -= 20.0f;
            if (*targetVelocity < 0.0f) {
                *targetVelocity = 0.0f;
            } else {
                baseSpeedScale = 1.0f - Math_CosS(*targetVelocity * 450.0f);
                *targetVelocity = ((baseSpeedScale * baseSpeedScale) * 30.0f) + 7.0f;
            }
        } else {
            *targetVelocity *= 0.8f;
        }

        if (sAnalogStickDistance != 0.0f) {
            slope = Math_SinS(this->angleToFloorX);
            speedLimit = this->speedLimit;
            slopeSpeedScale = CLAMP(slope, 0.0f, 0.6f);

            if (this->shapeOffsetY != 0.0f) {
                speedLimit = speedLimit - (this->shapeOffsetY * 0.008f);
                if (speedLimit < 2.0f) {
                    speedLimit = 2.0f;
                }
            }

            *targetVelocity = (*targetVelocity * 0.14f) - (8.0f * slopeSpeedScale * slopeSpeedScale);
            *targetVelocity = CLAMP(*targetVelocity, 0.0f, speedLimit);

            return 1;
        }
    }

    return 0;
}

s32 Player_StepLinearVelocityToZero(Player* this) {
    return Math_StepToF(&this->linearVelocity, 0.0f, REG(43) / 100.0f);
}

s32 Player_GetTargetVelocityAndYaw(Player* this, f32* targetVelocity, s16* targetYaw, f32 arg3, PlayState* play) {
    if (!Player_CalculateTargetVelocityAndYaw(play, this, targetVelocity, targetYaw, arg3)) {
        *targetYaw = this->actor.shape.rot.y;

        if (this->targetActor != NULL) {
            if ((play->actorCtx.targetCtx.unk_4B != 0) &&
                !(this->stateFlags2 & PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION)) {
                *targetYaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->targetActor->focus.pos);
                return 0;
            }
        } else if (Player_IsFriendlyZTargeting(this)) {
            *targetYaw = this->targetYaw;
        }

        return 0;
    } else {
        *targetYaw += Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        return 1;
    }
}

// Indices into sSubActions, negative sign signals to stop processing the array
static s8 sTargetEnemyStandStillSubActions[] = { 13, 2, 4, 9, 10, 11, 8, -7 };
static s8 sFriendlyTargetingStandStillSubActions[] = { 13, 1, 2, 5, 3, 4, 9, 10, 11, 7, 8, -6 };
static s8 sEndSidewalkSubActions[] = { 13, 1, 2, 3, 4, 9, 10, 11, 8, 7, -6 };
static s8 sFriendlyBackwalkSubActions[] = { 13, 2, 4, 9, 10, 11, 8, -7 };
static s8 sSidewalkSubActions[] = { 13, 2, 4, 9, 10, 11, 12, 8, -7 };
static s8 sTurnSubActions[] = { -7 };
static s8 sStandStillSubActions[] = { 0, 11, 1, 2, 3, 5, 4, 9, 8, 7, -6 };
static s8 sRunSubActions[] = { 0, 11, 1, 2, 3, 12, 5, 4, 9, 8, 7, -6 };
static s8 sTargetRunSubActions[] = { 13, 1, 2, 3, 12, 5, 4, 9, 10, 11, 8, 7, -6 };
static s8 sEndBackwalkSubActions[] = { 10, 8, -7 };
static s8 sSwimSubActions[] = { 0, 12, 5, -4 };

static s32 (*sSubActions[])(Player* this, PlayState* play) = {
    Player_SetupCUpBehavior,               // 0
    Player_SetupOpenDoor,                  // 1
    Player_SetupGetItemOrHoldBehavior,     // 2
    Player_SetupMountHorse,                // 3
    Player_SetupSpeakOrCheck,              // 4
    Player_SetupSpecialWallInteraction,    // 5
    Player_SetupRollOrPutAway,             // 6
    Player_SetupStartMeleeWeaponAttack,    // 7
    Player_SetupStartChargeSpinAttack,     // 8
    Player_SetupPutDownOrThrowActor,       // 9
    Player_SetupJumpSlashOrRoll,           // 10
    Player_SetupDefend,                    // 11
    Player_SetupWallJumpBehavior,          // 12
    Player_SetupItemCutsceneOrFirstPerson, // 13
};

// Checks if you're allowed to perform a sub-action based on a provided array of indices into sSubActions
s32 Player_SetupSubAction(PlayState* play, Player* this, s8* subActionIndex, s32 flag) {
    s32 i;

    if (!(this->stateFlags1 &
          (PLAYER_STATE1_EXITING_SCENE | PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_IN_CUTSCENE))) {
        if (flag != 0) {
            D_808535E0 = Player_SetupCurrentUpperAction(this, play);
            if (Player_ThrowDekuNut == this->actionFunc) {
                return 1;
            }
        }

        if (Player_IsShootingHookshot(this)) {
            this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
            return 1;
        }

        if (!(this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) &&
            (Player_StartChangeItem != this->upperActionFunc)) {
            while (*subActionIndex >= 0) {
                if (sSubActions[*subActionIndex](this, play)) {
                    return 1;
                }
                subActionIndex++;
            }

            if (sSubActions[-(*subActionIndex)](this, play)) {
                return 1;
            }
        }
    }

    return 0;
}

// Checks if action is interrupted within a certain number of frames from the end of the current animation
// Returns -1 is action is not interrupted at all, 0 if interrupted by a sub-action, 1 if interrupted by the player
// moving
s32 Player_IsActionInterrupted(PlayState* play, Player* this, SkelAnime* skelAnime, f32 framesFromEnd) {
    f32 targetVelocity;
    s16 targetYaw;

    if ((skelAnime->endFrame - framesFromEnd) <= skelAnime->curFrame) {
        if (Player_SetupSubAction(play, this, sStandStillSubActions, 1)) {
            return 0;
        }

        if (Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play)) {
            return 1;
        }
    }

    return -1;
}

void Player_SetupSpinAttackActor(PlayState* play, Player* this, s32 spinAttackParams) {
    if (spinAttackParams != 0) {
        this->spinAttackTimer = 0.0f;
    } else {
        this->spinAttackTimer = 0.5f;
    }

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (this->actor.category == ACTORCAT_PLAYER) {
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_M_THUNDER, this->bodyPartsPos[PLAYER_BODYPART_WAIST].x,
                    this->bodyPartsPos[PLAYER_BODYPART_WAIST].y, this->bodyPartsPos[PLAYER_BODYPART_WAIST].z, 0, 0, 0,
                    Player_GetMeleeWeaponHeld(this) | spinAttackParams);
    }
}

s32 Player_CanQuickspin(Player* this) {
    s8 stickInputsArray[4];
    s8* analogStickInput;
    s8* stickInput;
    s8 inputDiff1;
    s8 inputDiff2;
    s32 i;

    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) || Player_HoldsBrokenKnife(this)) {
        return 0;
    }

    analogStickInput = &this->analogStickInputs[0];
    stickInput = &stickInputsArray[0];
    // Check all four stored frames of input to see if stick distance traveled is ever less than 55.0f from the center
    for (i = 0; i < 4; i++, analogStickInput++, stickInput++) {
        if ((*stickInput = *analogStickInput) < 0) {
            return 0;
        }
        // Multiply each stored stickInput by 2
        *stickInput *= 2;
    }

    // Get diff between first two frames of stick input
    inputDiff1 = stickInputsArray[0] - stickInputsArray[1];
    // Return false if the difference is too small (< ~28 degrees) for the player to be spinning the analog stick
    if (ABS(inputDiff1) < 10) {
        return 0;
    }

    stickInput = &stickInputsArray[1];
    // *stickInput will be the second, then third frame of stick input in this loop
    for (i = 1; i < 3; i++, stickInput++) {
        // Get diff between current input frame and next input frame
        inputDiff2 = *stickInput - *(stickInput + 1);
        // If the difference too small, or stick has changed directions, return false
        if ((ABS(inputDiff2) < 10) || (inputDiff2 * inputDiff1 < 0)) {
            return 0;
        }
    }

    return 1;
}

void Player_SetupSpinAttackAnims(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;

    if ((this->meleeAttackType >= PLAYER_MELEEATKTYPE_LEFT_SLASH_1H) &&
        (this->meleeAttackType <= PLAYER_MELEEATKTYPE_LEFT_COMBO_2H)) {
        anim = sSpinAttackAnims1[Player_HoldsTwoHandedWeapon(this)];
    } else {
        anim = sSpinAttackAnims2[Player_HoldsTwoHandedWeapon(this)];
    }

    Player_InactivateMeleeWeapon(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 8.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -9.0f);
    Player_SetupSpinAttackActor(play, this, 0x200);
}

void Player_StartChargeSpinAttack(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_ChargeSpinAttack, 1);
    Player_SetupSpinAttackAnims(play, this);
}

static s8 sMeleeWeaponAttackDirections[] = {
    PLAYER_MELEEATKTYPE_STAB_1H,
    PLAYER_MELEEATKTYPE_LEFT_SLASH_1H,
    PLAYER_MELEEATKTYPE_LEFT_SLASH_1H,
    PLAYER_MELEEATKTYPE_RIGHT_SLASH_1H,
};
static s8 sHammerAttackDirections[] = {
    PLAYER_MELEEATKTYPE_HAMMER_FORWARD,
    PLAYER_MELEEATKTYPE_HAMMER_SIDE,
    PLAYER_MELEEATKTYPE_HAMMER_FORWARD,
    PLAYER_MELEEATKTYPE_HAMMER_SIDE,
};

s32 Player_GetMeleeAttackAnim(Player* this) {
    s32 relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];
    s32 attackAnim;

    if (this->heldItemAction == PLAYER_IA_HAMMER) {
        if (relativeStickInput < PLAYER_RELATIVESTICKINPUT_FORWARD) {
            relativeStickInput = PLAYER_RELATIVESTICKINPUT_FORWARD;
        }
        attackAnim = sHammerAttackDirections[relativeStickInput];
        this->slashCounter = 0;
    } else {
        if (Player_CanQuickspin(this)) {
            attackAnim = PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H;
        } else {
            if (relativeStickInput < PLAYER_RELATIVESTICKINPUT_FORWARD) {
                if (Player_IsZTargeting(this)) {
                    attackAnim = PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H;
                } else {
                    attackAnim = PLAYER_MELEEATKTYPE_LEFT_SLASH_1H;
                }
            } else {
                attackAnim = sMeleeWeaponAttackDirections[relativeStickInput];
                if (attackAnim == PLAYER_MELEEATKTYPE_STAB_1H) {
                    this->stateFlags2 |= PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
                    if (!Player_IsZTargeting(this)) {
                        attackAnim = PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H;
                    }
                }
            }
            if (this->heldItemAction == PLAYER_IA_DEKU_STICK) {
                attackAnim = PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H;
            }
        }
        if (Player_HoldsTwoHandedWeapon(this)) {
            attackAnim++;
        }
    }

    return attackAnim;
}

void Player_SetupMeleeWeaponToucherFlags(Player* this, s32 quadIndex, u32 dmgFlags) {
    this->meleeWeaponQuads[quadIndex].info.toucher.dmgFlags = dmgFlags;

    if (dmgFlags == DMG_DEKU_STICK) {
        this->meleeWeaponQuads[quadIndex].info.toucherFlags = TOUCH_ON | TOUCH_NEAREST | TOUCH_SFX_WOOD;
    } else {
        this->meleeWeaponQuads[quadIndex].info.toucherFlags = TOUCH_ON | TOUCH_NEAREST;
    }
}

static u32 sMeleeWeaponDmgFlags[][2] = {
    { DMG_SLASH_MASTER, DMG_JUMP_MASTER }, { DMG_SLASH_KOKIRI, DMG_JUMP_KOKIRI }, { DMG_SLASH_GIANT, DMG_JUMP_GIANT },
    { DMG_DEKU_STICK, DMG_JUMP_MASTER },   { DMG_HAMMER_SWING, DMG_HAMMER_JUMP },
};

void Player_StartMeleeWeaponAttack(PlayState* play, Player* this, s32 meleeAttackType) {
    s32 pad;
    u32 dmgFlags;
    s32 meleeWeapon;

    Player_SetActionFunc(play, this, Player_MeleeWeaponAttack, 0);
    this->comboTimer = 8;
    if (!((meleeAttackType >= PLAYER_MELEEATKTYPE_FLIPSLASH_FINISH) &&
          (meleeAttackType <= PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH))) {
        Player_InactivateMeleeWeapon(this);
    }

    if ((meleeAttackType != this->meleeAttackType) || !(this->slashCounter < 3)) {
        this->slashCounter = 0;
    }

    this->slashCounter++;
    if (this->slashCounter >= 3) {
        meleeAttackType += 2;
    }

    this->meleeAttackType = meleeAttackType;

    Player_PlayAnimOnceSlowed(play, this, sMeleeAttackAnims[meleeAttackType].startAnim);
    if ((meleeAttackType != PLAYER_MELEEATKTYPE_FLIPSLASH_START) &&
        (meleeAttackType != PLAYER_MELEEATKTYPE_JUMPSLASH_START)) {
        Player_SetupAnimMovement(play, this,
                                 PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                     PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
    }

    this->currentYaw = this->actor.shape.rot.y;

    if (Player_HoldsBrokenKnife(this)) {
        meleeWeapon = PLAYER_MELEEWEAPON_SWORD_MASTER;
    } else {
        meleeWeapon = Player_GetMeleeWeaponHeld(this) - 1;
    }

    if ((meleeAttackType >= PLAYER_MELEEATKTYPE_FLIPSLASH_START) &&
        (meleeAttackType <= PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH)) {
        dmgFlags = sMeleeWeaponDmgFlags[meleeWeapon][1];
    } else {
        dmgFlags = sMeleeWeaponDmgFlags[meleeWeapon][0];
    }

    Player_SetupMeleeWeaponToucherFlags(this, 0, dmgFlags);
    Player_SetupMeleeWeaponToucherFlags(this, 1, dmgFlags);
}

void Player_SetupInvincibility(Player* this, s32 timer) {
    if (this->invincibilityTimer >= 0) {
        this->invincibilityTimer = timer;
        this->damageFlashTimer = 0;
    }
}

void Player_SetupInvincibilityNoDamageFlash(Player* this, s32 timer) {
    if (this->invincibilityTimer > timer) {
        this->invincibilityTimer = timer;
    }
    this->damageFlashTimer = 0;
}

// Returns false is player is out of health
s32 Player_InflictDamage(PlayState* play, Player* this, s32 damage) {
    if ((this->invincibilityTimer != 0) || (this->actor.category != ACTORCAT_PLAYER)) {
        return 1;
    }

    return Health_ChangeBy(play, damage);
}

void Player_SetLedgeGrabPosition(Player* this) {
    this->skelAnime.prevTransl = this->skelAnime.jointTable[0];
    Player_UpdateAnimMovement(this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y);
}

void Player_SetupFallFromLedge(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UpdateMidair, 0);
    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
    this->genericTimer = 1;
    if (this->attentionMode != PLAYER_ATTENTIONMODE_CUTSCENE) {
        this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
    }
}

static LinkAnimationHeader* sLinkDamageAnims[] = {
    &gPlayerAnim_link_normal_front_shit, &gPlayerAnim_link_normal_front_shitR, &gPlayerAnim_link_normal_back_shit,
    &gPlayerAnim_link_normal_back_shitR, &gPlayerAnim_link_normal_front_hit,   &gPlayerAnim_link_anchor_front_hitR,
    &gPlayerAnim_link_normal_back_hit,   &gPlayerAnim_link_anchor_back_hitR,
};

void Player_SetupDamage(PlayState* play, Player* this, s32 damageReaction, f32 knockbackVelXZ, f32 knockbackVelY,
                        s16 damageYaw, s32 invincibilityTimer) {
    LinkAnimationHeader* anim = NULL;
    LinkAnimationHeader** damageAnims;

    if (this->stateFlags1 & PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP) {
        Player_SetLedgeGrabPosition(this);
    }

    this->runDamageTimer = 0;

    func_8002F7DC(&this->actor, NA_SE_PL_DAMAGE);

    if (!Player_InflictDamage(play, this, 0 - this->actor.colChkInfo.damage)) {
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
            Player_SetupFallFromLedge(this, play);
        }
        return;
    }

    Player_SetupInvincibility(this, invincibilityTimer);

    if (damageReaction == PLAYER_DMGREACTION_FROZEN) {
        Player_SetActionFunc(play, this, Player_FrozenInIce, 0);

        anim = &gPlayerAnim_link_normal_ice_down;

        Player_ClearAttentionModeAndStopMoving(this);
        Player_RequestRumble(this, 255, 10, 40, 0);

        func_8002F7DC(&this->actor, NA_SE_PL_FREEZE_S);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
    } else if (damageReaction == PLAYER_DMGREACTION_ELECTRIC_SHOCKED) {
        Player_SetActionFunc(play, this, Player_SetupElectricShock, 0);

        Player_RequestRumble(this, 255, 80, 150, 0);

        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_normal_electric_shock);
        Player_ClearAttentionModeAndStopMoving(this);

        this->genericTimer = 20;
    } else {
        damageYaw -= this->actor.shape.rot.y;
        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            Player_SetActionFunc(play, this, Player_DamagedSwim, 0);
            Player_RequestRumble(this, 180, 20, 50, 0);

            this->linearVelocity = 4.0f;
            this->actor.velocity.y = 0.0f;

            anim = &gPlayerAnim_link_swimer_swim_hit;

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        } else if ((damageReaction == PLAYER_DMGREACTION_KNOCKBACK) || (damageReaction == PLAYER_DMGREACTION_HOP) ||
                   !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                   (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                         PLAYER_STATE1_CLIMBING))) {
            Player_SetActionFunc(play, this, Player_StartKnockback, 0);

            this->stateFlags3 |= PLAYER_STATE3_MIDAIR;

            Player_RequestRumble(this, 255, 20, 150, 0);
            Player_ClearAttentionModeAndStopMoving(this);

            if (damageReaction == PLAYER_DMGREACTION_HOP) {
                this->genericTimer = 4;

                this->actor.speedXZ = 3.0f;
                this->linearVelocity = 3.0f;
                this->actor.velocity.y = 6.0f;

                Player_ChangeAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_damage_run, this->modelAnimType));
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
            } else {
                this->actor.speedXZ = knockbackVelXZ;
                this->linearVelocity = knockbackVelXZ;
                this->actor.velocity.y = knockbackVelY;

                if (ABS(damageYaw) > DEG_TO_BINANG(90.0f)) {
                    anim = &gPlayerAnim_link_normal_front_downA;
                } else {
                    anim = &gPlayerAnim_link_normal_back_downA;
                }

                if ((this->actor.category != ACTORCAT_PLAYER) && (this->actor.colChkInfo.health == 0)) {
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_BL_DOWN);
                } else {
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
                }
            }

            this->hoverBootsTimer = 0;
            this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        } else {
            if ((this->linearVelocity > 4.0f) && !Player_IsUnfriendlyZTargeting(this)) {
                this->runDamageTimer = 20;
                Player_RequestRumble(this, 120, 20, 10, 0);
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
                return;
            }

            damageAnims = sLinkDamageAnims;

            Player_SetActionFunc(play, this, func_8084370C, 0);
            Player_ResetLeftRightBlendWeight(this);

            if (this->actor.colChkInfo.damage < 5) {
                Player_RequestRumble(this, 120, 20, 10, 0);
            } else {
                Player_RequestRumble(this, 180, 20, 100, 0);
                this->linearVelocity = 23.0f;
                damageAnims += 4;
            }

            if (ABS(damageYaw) <= 0x4000) {
                damageAnims += 2;
            }

            if (Player_IsUnfriendlyZTargeting(this)) {
                damageAnims += 1;
            }

            anim = *damageAnims;

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        }

        this->actor.shape.rot.y += damageYaw;
        this->currentYaw = this->actor.shape.rot.y;
        this->actor.world.rot.y = this->actor.shape.rot.y;
        if (ABS(damageYaw) > 0x4000) {
            this->actor.shape.rot.y += 0x8000;
        }
    }

    Player_ResetAttributesAndHeldActor(play, this);

    this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;

    if (anim != NULL) {
        Player_PlayAnimOnceSlowed(play, this, anim);
    }
}

s32 Player_GetHurtFloorType(s32 floorType) {
    s32 hurtFloorType = floorType - FLOOR_TYPE_HURT_FLOOR;

    if ((hurtFloorType >= FLOOR_TYPE_NONE) && (hurtFloorType <= (FLOOR_TYPE_FIRE_HURT_FLOOR - FLOOR_TYPE_HURT_FLOOR))) {
        return hurtFloorType;
    } else {
        return PLAYER_HURTFLOORTYPE_NONE;
    }
}

s32 Player_IsFloorSinkingSand(s32 floorType) {
    return (floorType == FLOOR_TYPE_SHALLOW_SAND) || (floorType == FLOOR_TYPE_QUICKSAND_NO_HORSE) ||
           (floorType == FLOOR_TYPE_QUICKSAND_HORSE_CAN_CROSS);
}

void Player_BurnDekuShield(Player* this, PlayState* play) {
    if (this->currentShield == PLAYER_SHIELD_DEKU) {
        Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_SHIELD, this->actor.world.pos.x, this->actor.world.pos.y,
                    this->actor.world.pos.z, 0, 0, 0, 1);
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SHIELD);
        Message_StartTextbox(play, 0x305F, NULL);
    }
}

void Player_StartBurning(Player* this) {
    s32 i;

    // clang-format off
    for (i = 0; i < PLAYER_BODYPART_MAX; i++) { this->flameTimers[i] = Rand_S16Offset(0, 200); }
    // clang-format on

    this->isBurning = true;
}

void Player_PlayFallSfxAndCheckBurning(Player* this) {
    if (this->actor.colChkInfo.acHitEffect == PLAYER_HITEFFECTAC_FIRE) {
        Player_StartBurning(this);
    }
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void Player_RoundUpInvincibilityTimer(Player* this) {
    if ((this->invincibilityTimer >= 0) && (this->invincibilityTimer < 20)) {
        this->invincibilityTimer = 20;
    }
}

s32 Player_UpdateDamage(Player* this, PlayState* play) {
    s32 pad;
    s32 sinkingGroundVoidOut = false;
    s32 attackHitShield;

    if (this->voidRespawnCounter != 0) {
        if (!Player_InBlockingCsMode(play, this)) {
            Player_InflictDamageAndCheckForDeath(play, -16);
            this->voidRespawnCounter = 0;
        }
    } else {
        sinkingGroundVoidOut = ((Player_GetHeight(this) - 8.0f) < (this->shapeOffsetY * this->actor.scale.y));

        if (sinkingGroundVoidOut || (this->actor.bgCheckFlags & BGCHECKFLAG_CRUSHED) ||
            (sFloorType == FLOOR_TYPE_VOID_ON_TOUCH) || (this->stateFlags2 & PLAYER_STATE2_FORCE_VOID_OUT)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);

            if (sinkingGroundVoidOut) {
                Play_TriggerRespawn(play);
                Scene_SetTransitionForNextEntrance(play);
            } else {
                // Special case for getting crushed in Forest Temple's Checkboard Ceiling Hall or Shadow Temple's
                // Falling Spike Trap Room, to respawn the player in a specific place
                if (((play->sceneId == SCENE_FOREST_TEMPLE) && (play->roomCtx.curRoom.num == 15)) ||
                    ((play->sceneId == SCENE_SHADOW_TEMPLE) && (play->roomCtx.curRoom.num == 10))) {
                    static SpecialRespawnInfo checkboardCeilingRespawn = { { 1992.0f, 403.0f, -3432.0f }, 0 };
                    static SpecialRespawnInfo fallingSpikeTrapRespawn = { { 1200.0f, -1343.0f, 3850.0f }, 0 };
                    SpecialRespawnInfo* respawnInfo;

                    if (play->sceneId == SCENE_FOREST_TEMPLE) {
                        respawnInfo = &checkboardCeilingRespawn;
                    } else {
                        respawnInfo = &fallingSpikeTrapRespawn;
                    }

                    Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = respawnInfo->pos;
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = respawnInfo->yaw;
                }

                Play_TriggerVoidOut(play);
            }

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_TAKEN_AWAY);
            play->unk_11DE9 = true;
            func_80078884(NA_SE_OC_ABYSS);
        } else if ((this->damageEffect != PLAYER_DMGEFFECT_NONE) &&
                   ((this->damageEffect >= PLAYER_DMGEFFECT_KNOCKBACK) || (this->invincibilityTimer == 0))) {
            u8 damageReactions[] = { PLAYER_DMGREACTION_HOP, PLAYER_DMGREACTION_KNOCKBACK,
                                     PLAYER_DMGREACTION_KNOCKBACK };

            Player_PlayFallSfxAndCheckBurning(this);

            if (this->damageEffect == PLAYER_DMGEFFECT_ELECTRIC_KNOCKBACK) {
                this->shockTimer = 40;
            }

            this->actor.colChkInfo.damage += this->damageAmount;
            Player_SetupDamage(play, this, damageReactions[this->damageEffect - 1], this->knockbackVelXZ,
                               this->knockbackVelY, this->damageYaw, 20);
        } else {
            attackHitShield = (this->shieldQuad.base.acFlags & AC_BOUNCED) != 0;

            //! @bug The second set of conditions here seems intended as a way for Link to "block" hits by rolling.
            // However, `Collider.atFlags` is a byte so the flag check at the end is incorrect and cannot work.
            // Additionally, `Collider.atHit` can never be set while already colliding as AC, so it's also bugged.
            // This behavior was later fixed in MM, most likely by removing both the `atHit` and `atFlags` checks.
            if (attackHitShield ||
                ((this->invincibilityTimer < 0) && (this->cylinder.base.acFlags & AC_HIT) &&
                 (this->cylinder.info.atHit != NULL) && (this->cylinder.info.atHit->atFlags & 0x20000000))) {

                Player_RequestRumble(this, 180, 20, 100, 0);

                if (!Player_IsChildWithHylianShield(this)) {
                    if (this->invincibilityTimer >= 0) {
                        LinkAnimationHeader* anim;
                        s32 isAimingShieldCrouched = (Player_AimShieldCrouched == this->actionFunc);

                        if (!Player_IsSwimming(this)) {
                            Player_SetActionFunc(play, this, Player_DeflectAttackWithShield, 0);
                        }

                        if (!(this->genericVar = isAimingShieldCrouched)) {
                            Player_SetUpperActionFunc(this, Player_EndDeflectAttackStanding);

                            if (this->leftRightBlendWeight < 0.5f) {
                                anim = sRightStandingDeflectWithShieldAnims[Player_HoldsTwoHandedWeapon(this)];
                            } else {
                                anim = sLeftStandingDeflectWithShieldAnims[Player_HoldsTwoHandedWeapon(this)];
                            }
                            LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, anim);
                        } else {
                            Player_PlayAnimOnce(play, this, sDeflectWithShieldAnims[Player_HoldsTwoHandedWeapon(this)]);
                        }
                    }

                    if (!(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP |
                                               PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING))) {
                        this->linearVelocity = -18.0f;
                        this->currentYaw = this->actor.shape.rot.y;
                    }
                }

                if (attackHitShield && (this->shieldQuad.info.acHitInfo->toucher.effect == 1)) {
                    Player_BurnDekuShield(this, play);
                }

                return 0;
            }

            if ((this->deathTimer != 0) || (this->invincibilityTimer > 0) ||
                (this->stateFlags1 & PLAYER_STATE1_TAKING_DAMAGE) || (this->csMode != PLAYER_CSMODE_NONE) ||
                (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) ||
                (this->meleeWeaponQuads[1].base.atFlags & AT_HIT)) {
                return 0;
            }

            if (this->cylinder.base.acFlags & AC_HIT) {
                Actor* ac = this->cylinder.base.ac;
                s32 damageReaction;

                if (ac->flags & ACTOR_FLAG_24) {
                    func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
                }

                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    damageReaction = PLAYER_DMGREACTION_DEFAULT;
                } else if (this->actor.colChkInfo.acHitEffect == PLAYER_HITEFFECTAC_ICE) {
                    damageReaction = PLAYER_DMGREACTION_FROZEN;
                } else if (this->actor.colChkInfo.acHitEffect == PLAYER_HITEFFECTAC_ELECTRIC) {
                    damageReaction = PLAYER_DMGREACTION_ELECTRIC_SHOCKED;
                } else if (this->actor.colChkInfo.acHitEffect == PLAYER_HITEFFECTAC_POWERFUL_HIT) {
                    damageReaction = PLAYER_DMGREACTION_KNOCKBACK;
                } else {
                    Player_PlayFallSfxAndCheckBurning(this);
                    damageReaction = PLAYER_DMGREACTION_DEFAULT;
                }

                Player_SetupDamage(play, this, damageReaction, 4.0f, 5.0f, Actor_WorldYawTowardActor(ac, &this->actor),
                                   20);
            } else if (this->invincibilityTimer != 0) {
                return 0;
            } else {
                static u8 dmgStartFrame[] = { 120, 60 };
                s32 hurtFloorType = Player_GetHurtFloorType(sFloorType);

                if (((this->actor.wallPoly != NULL) &&
                     SurfaceType_IsWallDamage(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) ||
                    ((hurtFloorType >= PLAYER_HURTFLOORTYPE_DEFAULT) &&
                     SurfaceType_IsWallDamage(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) &&
                     (this->hurtFloorTimer >= dmgStartFrame[hurtFloorType])) ||
                    ((hurtFloorType >= PLAYER_HURTFLOORTYPE_DEFAULT) &&
                     ((this->currentTunic != PLAYER_TUNIC_GORON) ||
                      (this->hurtFloorTimer >= dmgStartFrame[hurtFloorType])))) {
                    this->hurtFloorTimer = 0;
                    this->actor.colChkInfo.damage = 4;
                    Player_SetupDamage(play, this, PLAYER_DMGREACTION_DEFAULT, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
                } else {
                    return 0;
                }
            }
        }
    }

    return 1;
}

void Player_SetupJumpWithSfx(Player* this, LinkAnimationHeader* anim, f32 yVel, PlayState* play, u16 sfxId) {
    Player_SetActionFunc(play, this, Player_UpdateMidair, 1);

    if (anim != NULL) {
        Player_PlayAnimOnceSlowed(play, this, anim);
    }

    this->actor.velocity.y = yVel * sWaterSpeedScale;
    this->hoverBootsTimer = 0;
    this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;

    Player_PlayJumpSfx(this);
    Player_PlayVoiceSfxForAge(this, sfxId);

    this->stateFlags1 |= PLAYER_STATE1_JUMPING;
}

void Player_SetupJump(Player* this, LinkAnimationHeader* anim, f32 yVel, PlayState* play) {
    Player_SetupJumpWithSfx(this, anim, yVel, play, NA_SE_VO_LI_SWORD_N);
}

s32 Player_SetupWallJumpBehavior(Player* this, PlayState* play) {
    s32 canJumpToLedge;
    LinkAnimationHeader* anim;
    f32 wallHeight;
    f32 yVel;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;
    f32 wallDist;

    if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) &&
        (this->touchedWallJumpType >= PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP) &&
        (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->ageProperties->unk_14 > this->wallHeight))) {
        canJumpToLedge = false;

        if (Player_IsSwimming(this)) {
            if (this->actor.yDistToWater < 50.0f) {
                if ((this->touchedWallJumpType < PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP) ||
                    (this->wallHeight > this->ageProperties->unk_10)) {
                    return 0;
                }
            } else if ((this->currentBoots != PLAYER_BOOTS_IRON) ||
                       (this->touchedWallJumpType > PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP)) {
                return 0;
            }
        } else if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                   ((this->ageProperties->unk_14 <= this->wallHeight) &&
                    (this->stateFlags1 & PLAYER_STATE1_SWIMMING))) {
            return 0;
        }

        if ((this->actor.wallBgId != BGCHECK_SCENE) && (sInteractWallFlags & WALL_FLAG_6)) {
            if (this->wallTouchTimer >= 6) {
                this->stateFlags2 |= PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL;
                if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                    canJumpToLedge = true;
                }
            }
        } else if ((this->wallTouchTimer >= 6) || CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
            canJumpToLedge = true;
        }

        if (canJumpToLedge != false) {
            Player_SetActionFunc(play, this, Player_JumpUpToLedge, 0);

            this->stateFlags1 |= PLAYER_STATE1_JUMPING;

            wallHeight = this->wallHeight;

            if (this->ageProperties->unk_14 <= wallHeight) {
                anim = &gPlayerAnim_link_normal_250jump_start;
                this->linearVelocity = 1.0f;
            } else {
                wallPolyNormalX = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.x);
                wallPolyNormalZ = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.z);
                wallDist = this->wallDistance + 0.5f;

                this->stateFlags1 |= PLAYER_STATE1_CLIMBING_ONTO_LEDGE;

                if (Player_IsSwimming(this)) {
                    anim = &gPlayerAnim_link_swimer_swim_15step_up;
                    wallHeight -= (60.0f * this->ageProperties->translationScale);
                    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;
                } else if (this->ageProperties->unk_18 <= wallHeight) {
                    anim = &gPlayerAnim_link_normal_150step_up;
                    wallHeight -= (59.0f * this->ageProperties->translationScale);
                } else {
                    anim = &gPlayerAnim_link_normal_100step_up;
                    wallHeight -= (41.0f * this->ageProperties->translationScale);
                }

                this->actor.shape.yOffset -= wallHeight * 100.0f;

                this->actor.world.pos.x -= wallDist * wallPolyNormalX;
                this->actor.world.pos.y += this->wallHeight;
                this->actor.world.pos.z -= wallDist * wallPolyNormalZ;

                Player_ClearAttentionModeAndStopMoving(this);
            }

            this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;

            LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 1.3f);
            AnimationContext_DisableQueue(play);

            this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;

            return 1;
        }
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
               (this->touchedWallJumpType == PLAYER_WALLJUMPTYPE_HOP_UP) && (this->wallTouchTimer >= 3)) {
        yVel = (this->wallHeight * 0.08f) + 5.5f;
        Player_SetupJump(this, &gPlayerAnim_link_normal_jump, yVel, play);
        this->linearVelocity = 2.5f;

        return 1;
    }

    return 0;
}

void Player_SetupMiniCsMovement(PlayState* play, Player* this, f32 xzOffset, s16 yaw) {
    Player_SetActionFunc(play, this, Player_MiniCsMovement, 0);
    Player_ResetAttributes(play, this);

    this->genericVar = 1;
    this->genericTimer = 1;

    this->csStartPos.x = (Math_SinS(yaw) * xzOffset) + this->actor.world.pos.x;
    this->csStartPos.z = (Math_CosS(yaw) * xzOffset) + this->actor.world.pos.z;

    Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
}

void Player_SetupSwimIdle(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_UpdateSwimIdle, 0);
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
}

void Player_SetupEnterGrotto(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_EnterGrotto, 0);

    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID;

    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_FREE0);
}

s32 Player_ShouldEnterGrotto(PlayState* play, Player* this) {
    if ((play->transitionTrigger == TRANS_TRIGGER_OFF) &&
        (this->stateFlags1 & PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID)) {
        Player_SetupEnterGrotto(play, this);
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_S);
        func_800788CC(NA_SE_OC_SECRET_WARP_IN);
        return 1;
    }

    return 0;
}

/**
 * The actual entrances each "return entrance" value can map to.
 * This is used by scenes that are shared between locations, like child/adult Shooting Gallery or Great Fairy Fountains.
 *
 * This 1D array is split into groups of entrances.
 * The start of each group is indexed by `sReturnEntranceGroupIndices` values.
 * The resulting groups are then indexed by the spawn value.
 *
 * The spawn value (`PlayState.spawn`) is set to a different value depending on the entrance used to enter the
 * scene, which allows these dynamic "return entrances" to link back to the previous scene.
 *
 * Note: grottos and normal fairy fountains use `ENTR_RETURN_GROTTO`
 */
s16 sReturnEntranceGroupData[] = {
    // ENTR_RETURN_DAIYOUSEI_IZUMI
    /*  0 */ ENTR_DEATH_MOUNTAIN_TRAIL_4,  // DMT from Magic Fairy Fountain
    /*  1 */ ENTR_DEATH_MOUNTAIN_CRATER_3, // DMC from Double Defense Fairy Fountain
    /*  2 */ ENTR_HYRULE_CASTLE_2,         // Hyrule Castle from Dins Fire Fairy Fountain

    // ENTR_RETURN_2
    /*  3 */ ENTR_KAKARIKO_VILLAGE_9, // Kakariko from Potion Shop
    /*  4 */ ENTR_MARKET_DAY_5,       // Market (child day) from Potion Shop

    // ENTR_RETURN_SHOP1
    /*  5 */ ENTR_KAKARIKO_VILLAGE_3, // Kakariko from Bazaar
    /*  6 */ ENTR_MARKET_DAY_6,       // Market (child day) from Bazaar

    // ENTR_RETURN_4
    /*  7 */ ENTR_KAKARIKO_VILLAGE_11, // Kakariko from House of Skulltulas
    /*  8 */ ENTR_BACK_ALLEY_DAY_2,    // Back Alley (day) from Bombchu Shop

    // ENTR_RETURN_SYATEKIJYOU
    /*  9 */ ENTR_KAKARIKO_VILLAGE_10, // Kakariko from Shooting Gallery
    /* 10 */ ENTR_MARKET_DAY_8,        // Market (child day) from Shooting Gallery

    // ENTR_RETURN_YOUSEI_IZUMI_YOKO
    /* 11 */ ENTR_ZORAS_FOUNTAIN_5,  // Zoras Fountain from Farores Wind Fairy Fountain
    /* 12 */ ENTR_HYRULE_CASTLE_2,   // Hyrule Castle from Dins Fire Fairy Fountain
    /* 13 */ ENTR_DESERT_COLOSSUS_7, // Desert Colossus from Nayrus Love Fairy Fountain
};

/**
 * The values are indices into `sReturnEntranceGroupData` marking the start of each group
 */
u8 sReturnEntranceGroupIndices[] = {
    11, // ENTR_RETURN_YOUSEI_IZUMI_YOKO
    9,  // ENTR_RETURN_SYATEKIJYOU
    3,  // ENTR_RETURN_2
    5,  // ENTR_RETURN_SHOP1
    7,  // ENTR_RETURN_4
    0,  // ENTR_RETURN_DAIYOUSEI_IZUMI
};

s32 Player_SetupExit(PlayState* play, Player* this, CollisionPoly* poly, u32 bgId) {
    s32 exitIndex;
    s32 floorSpecialProperty;
    s32 yDistToExit;
    f32 linearVel;
    s32 yaw;

    if (this->actor.category == ACTORCAT_PLAYER) {
        exitIndex = 0;

        if (!(this->stateFlags1 & PLAYER_STATE1_IN_DEATH_CUTSCENE) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
            (this->csMode == PLAYER_CSMODE_NONE) && !(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
            (((poly != NULL) && (exitIndex = SurfaceType_GetExitIndex(&play->colCtx, poly, bgId), exitIndex != 0)) ||
             (Player_IsFloorSinkingSand(sFloorType) && (this->floorProperty == FLOOR_PROPERTY_VOID_PIT_LARGE)))) {

            yDistToExit = this->sceneExitPosY - (s32)this->actor.world.pos.y;

            if (!(this->stateFlags1 &
                  (PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (yDistToExit < 100) &&
                (sPlayerYDistToFloor > 100.0f)) {
                return 0;
            }

            if (exitIndex == 0) {
                Play_TriggerVoidOut(play);
                Scene_SetTransitionForNextEntrance(play);
            } else {
                play->nextEntranceIndex = play->exitList[exitIndex - 1];

                if (play->nextEntranceIndex == ENTR_RETURN_GROTTO) {
                    gSaveContext.respawnFlag = 2;
                    play->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex;
                    play->transitionType = TRANS_TYPE_FADE_WHITE;
                    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
                } else if (play->nextEntranceIndex >= ENTR_RETURN_YOUSEI_IZUMI_YOKO) {
                    play->nextEntranceIndex =
                        sReturnEntranceGroupData[sReturnEntranceGroupIndices[play->nextEntranceIndex -
                                                                             ENTR_RETURN_YOUSEI_IZUMI_YOKO] +
                                                 play->spawn];
                    Scene_SetTransitionForNextEntrance(play);
                } else {
                    if (SurfaceType_GetFloorEffect(&play->colCtx, poly, bgId) == FLOOR_EFFECT_2) {
                        gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = play->nextEntranceIndex;
                        Play_TriggerVoidOut(play);
                        gSaveContext.respawnFlag = -2;
                    }

                    gSaveContext.retainWeatherMode = true;
                    Scene_SetTransitionForNextEntrance(play);
                }

                play->transitionTrigger = TRANS_TRIGGER_START;
            }

            if (!(this->stateFlags1 & (PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_IN_CUTSCENE)) &&
                !(this->stateFlags2 & PLAYER_STATE2_CRAWLING) && !Player_IsSwimming(this) &&
                (floorSpecialProperty = SurfaceType_GetFloorType(&play->colCtx, poly, bgId),
                 (floorSpecialProperty != FLOOR_TYPE_10)) &&
                ((yDistToExit < 100) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {

                if (floorSpecialProperty == FLOOR_TYPE_LOOK_UP) {
                    func_800788CC(NA_SE_OC_SECRET_HOLE_OUT);
                    func_800F6964(5);
                    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
                    gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
                } else {
                    linearVel = this->linearVelocity;

                    if (linearVel < 0.0f) {
                        this->actor.world.rot.y += 0x8000;
                        linearVel = -linearVel;
                    }

                    if (linearVel > R_RUN_SPEED_LIMIT / 100.0f) {
                        gSaveContext.entranceSpeed = R_RUN_SPEED_LIMIT / 100.0f;
                    } else {
                        gSaveContext.entranceSpeed = linearVel;
                    }

                    if (sConveyorSpeedIndex != CONVEYOR_SPEED_DISABLED) {
                        yaw = sConveyorYaw;
                    } else {
                        yaw = this->actor.world.rot.y;
                    }
                    Player_SetupMiniCsMovement(play, this, 400.0f, yaw);
                }
            } else {
                if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_StopMovement(this);
                }
            }

            this->stateFlags1 |= PLAYER_STATE1_EXITING_SCENE | PLAYER_STATE1_IN_CUTSCENE;

            Player_ChangeCameraSetting(play, CAM_SET_SCENE_TRANSITION);

            return 1;
        } else {
            if (play->transitionTrigger == TRANS_TRIGGER_OFF) {

                if ((this->actor.world.pos.y < -4000.0f) ||
                    (((this->floorProperty == FLOOR_PROPERTY_VOID_PIT) ||
                      (this->floorProperty == FLOOR_PROPERTY_VOID_PIT_LARGE)) &&
                     ((sPlayerYDistToFloor < 100.0f) || (this->fallDistance > 400.0f) ||
                      ((play->sceneId != SCENE_SHADOW_TEMPLE) && (this->fallDistance > 200.0f)))) ||
                    ((play->sceneId == SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR) && (this->fallDistance > 320.0f))) {

                    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                        if (this->floorProperty == FLOOR_PROPERTY_VOID_PIT) {
                            Play_TriggerRespawn(play);
                        } else {
                            Play_TriggerVoidOut(play);
                        }
                        play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                        func_80078884(NA_SE_OC_ABYSS);
                    } else {
                        Player_SetupEnterGrotto(play, this);
                        this->genericTimer = 9999;
                        if (this->floorProperty == FLOOR_PROPERTY_VOID_PIT) {
                            this->genericVar = -1;
                        } else {
                            this->genericVar = 1;
                        }
                    }
                }

                this->sceneExitPosY = this->actor.world.pos.y;
            }
        }
    }

    return 0;
}

void Player_GetWorldPosRelativeToPlayer(Player* this, Vec3f* playerPos, Vec3f* posOffset, Vec3f* worldPos) {
    f32 cos = Math_CosS(this->actor.shape.rot.y);
    f32 sin = Math_SinS(this->actor.shape.rot.y);

    worldPos->x = playerPos->x + ((posOffset->x * cos) + (posOffset->z * sin));
    worldPos->y = playerPos->y + posOffset->y;
    worldPos->z = playerPos->z + ((posOffset->z * cos) - (posOffset->x * sin));
}

Actor* Player_SpawnFairy(PlayState* play, Player* this, Vec3f* playerPos, Vec3f* posOffset, s32 type) {
    Vec3f pos;

    Player_GetWorldPosRelativeToPlayer(this, playerPos, posOffset, &pos);

    return Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ELF, pos.x, pos.y, pos.z, 0, 0, 0, type);
}

// Returns floor Y, or BGCHECK_MIN_Y if no floor detected
f32 Player_RaycastFloorWithOffset(PlayState* play, Player* this, Vec3f* raycastPosOffset, Vec3f* raycastPos,
                                  CollisionPoly** colPoly, s32* bgId) {
    Player_GetWorldPosRelativeToPlayer(this, &this->actor.world.pos, raycastPosOffset, raycastPos);

    return BgCheck_EntityRaycastDown3(&play->colCtx, colPoly, bgId, raycastPos);
}

// Returns floor Y, or BGCHECK_MIN_Y if no floor detected
f32 Player_RaycastFloorWithOffset2(PlayState* play, Player* this, Vec3f* raycastPosOffset, Vec3f* raycastPos) {
    CollisionPoly* colPoly;
    s32 polyBgId;

    return Player_RaycastFloorWithOffset(play, this, raycastPosOffset, raycastPos, &colPoly, &polyBgId);
}

s32 Player_WallLineTestWithOffset(PlayState* play, Player* this, Vec3f* posOffset, CollisionPoly** wallPoly, s32* bgId,
                                  Vec3f* posResult) {
    Vec3f posA;
    Vec3f posB;

    posA.x = this->actor.world.pos.x;
    posA.y = this->actor.world.pos.y + posOffset->y;
    posA.z = this->actor.world.pos.z;

    Player_GetWorldPosRelativeToPlayer(this, &this->actor.world.pos, posOffset, &posB);

    return BgCheck_EntityLineTest1(&play->colCtx, &posA, &posB, posResult, wallPoly, true, false, false, true, bgId);
}

s32 Player_SetupOpenDoor(Player* this, PlayState* play) {
    DoorShutter* doorShutter;
    DoorActorBase* door;
    s32 doorDirection;
    f32 cos;
    f32 sin;
    Actor* doorActor;
    f32 doorOpeningPosOffset;
    s32 pad3;
    s32 frontRoom;
    Actor* attachedActor;
    LinkAnimationHeader* anim;
    CollisionPoly* floorPoly;
    Vec3f raycastPos;

    if ((this->doorType != PLAYER_DOORTYPE_NONE) &&
        (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
         ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_EN_RU1)))) {
        if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) || (Player_SetupOpenDoorFromSpawn == this->actionFunc)) {
            doorActor = this->doorActor;

            if (this->doorType <= PLAYER_DOORTYPE_AJAR) {
                doorActor->textId = 0xD0;
                Player_StartTalkingWithActor(play, doorActor);
                return 0;
            }

            doorDirection = this->doorDirection;
            cos = Math_CosS(doorActor->shape.rot.y);
            sin = Math_SinS(doorActor->shape.rot.y);

            if (this->doorType == PLAYER_DOORTYPE_SLIDING) {
                doorShutter = (DoorShutter*)doorActor;

                this->currentYaw = doorShutter->dyna.actor.home.rot.y;
                if (doorDirection > 0) {
                    this->currentYaw -= 0x8000;
                }
                this->actor.shape.rot.y = this->currentYaw;

                if (this->linearVelocity <= 0.0f) {
                    this->linearVelocity = 0.1f;
                }

                Player_SetupMiniCsMovement(play, this, 50.0f, this->actor.shape.rot.y);

                this->genericVar = 0;
                this->csDoorType = this->doorType;
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;

                this->csStartPos.x = this->actor.world.pos.x + ((doorDirection * 20.0f) * sin);
                this->csStartPos.z = this->actor.world.pos.z + ((doorDirection * 20.0f) * cos);
                this->csEndPos.x = this->actor.world.pos.x + ((doorDirection * -120.0f) * sin);
                this->csEndPos.z = this->actor.world.pos.z + ((doorDirection * -120.0f) * cos);

                doorShutter->unk_164 = 1;
                Player_ClearAttentionModeAndStopMoving(this);

                if (this->doorTimer != 0) {
                    this->genericTimer = 0;
                    Player_ChangeAnimMorphToLastFrame(play, this, Player_GetStandingStillAnim(this));
                    this->skelAnime.endFrame = 0.0f;
                } else {
                    this->linearVelocity = 0.1f;
                }

                if (doorShutter->dyna.actor.category == ACTORCAT_DOOR) {
                    this->doorBgCamIndex =
                        play->transiActorCtx.list[GET_TRANSITION_ACTOR_INDEX(&doorShutter->dyna.actor)]
                            .sides[(doorDirection > 0) ? 0 : 1]
                            .bgCamIndex;

                    Actor_DisableLens(play);
                }
            } else {
                // The door actor can be either EnDoor or DoorKiller.
                door = (DoorActorBase*)doorActor;

                door->openAnim = (doorDirection < 0.0f)
                                     ? (LINK_IS_ADULT ? DOOR_OPEN_ANIM_ADULT_L : DOOR_OPEN_ANIM_CHILD_L)
                                     : (LINK_IS_ADULT ? DOOR_OPEN_ANIM_ADULT_R : DOOR_OPEN_ANIM_CHILD_R);

                if (door->openAnim == DOOR_OPEN_ANIM_ADULT_L) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorA_free, this->modelAnimType);
                } else if (door->openAnim == DOOR_OPEN_ANIM_CHILD_L) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorA, this->modelAnimType);
                } else if (door->openAnim == DOOR_OPEN_ANIM_ADULT_R) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorB_free, this->modelAnimType);
                } else {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_doorB, this->modelAnimType);
                }

                Player_SetActionFunc(play, this, Player_OpenDoor, 0);
                Player_UnequipItem(play, this);

                if (doorDirection < 0) {
                    this->actor.shape.rot.y = doorActor->shape.rot.y;
                } else {
                    this->actor.shape.rot.y = doorActor->shape.rot.y - 0x8000;
                }

                this->currentYaw = this->actor.shape.rot.y;

                doorOpeningPosOffset = (doorDirection * 22.0f);
                this->actor.world.pos.x = doorActor->world.pos.x + doorOpeningPosOffset * sin;
                this->actor.world.pos.z = doorActor->world.pos.z + doorOpeningPosOffset * cos;

                Player_PlayAnimOnceWithWaterInfluence(play, this, anim);

                if (this->doorTimer != 0) {
                    this->skelAnime.endFrame = 0.0f;
                }

                Player_ClearAttentionModeAndStopMoving(this);
                Player_SetupAnimMovement(play, this,
                                         PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                             PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                             PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_7 |
                                             PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);

                // If this door is the second half of a double door (spawned as child)
                if (doorActor->parent != NULL) {
                    doorDirection = -doorDirection;
                }

                door->playerIsOpening = true;

                // If the door actor is not DoorKiller
                if (this->doorType != PLAYER_DOORTYPE_FAKE) {
                    // The door actor is EnDoor

                    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                    Actor_DisableLens(play);

                    if (ENDOOR_GET_TYPE(doorActor) == DOOR_SCENEEXIT) {
                        raycastPos.x = doorActor->world.pos.x - (doorOpeningPosOffset * sin);
                        raycastPos.y = doorActor->world.pos.y + 10.0f;
                        raycastPos.z = doorActor->world.pos.z - (doorOpeningPosOffset * cos);

                        BgCheck_EntityRaycastDown1(&play->colCtx, &floorPoly, &raycastPos);

                        //! @bug floorPoly's bgId is not guaranteed to be BGCHECK_SCENE
                        if (Player_SetupExit(play, this, floorPoly, BGCHECK_SCENE)) {
                            gSaveContext.entranceSpeed = 2.0f;
                            gSaveContext.entranceSound = NA_SE_OC_DOOR_OPEN;
                        }
                    } else {
                        Camera_ChangeDoorCam(Play_GetCamera(play, CAM_ID_MAIN), doorActor,
                                             play->transiActorCtx.list[GET_TRANSITION_ACTOR_INDEX(doorActor)]
                                                 .sides[(doorDirection > 0) ? 0 : 1]
                                                 .bgCamIndex,
                                             0, 38.0f * sInvertedWaterSpeedScale, 26.0f * sInvertedWaterSpeedScale,
                                             10.0f * sInvertedWaterSpeedScale);
                    }
                }
            }

            if ((this->doorType != PLAYER_DOORTYPE_FAKE) && (doorActor->category == ACTORCAT_DOOR)) {
                frontRoom = play->transiActorCtx.list[GET_TRANSITION_ACTOR_INDEX(doorActor)]
                                .sides[(doorDirection > 0) ? 0 : 1]
                                .room;

                if ((frontRoom >= 0) && (frontRoom != play->roomCtx.curRoom.num)) {
                    func_8009728C(play, &play->roomCtx, frontRoom);
                }
            }

            doorActor->room = play->roomCtx.curRoom.num;

            if (((attachedActor = doorActor->child) != NULL) || ((attachedActor = doorActor->parent) != NULL)) {
                attachedActor->room = play->roomCtx.curRoom.num;
            }

            return 1;
        }
    }

    return 0;
}

void Player_SetupUnfriendlyZTargetStandStill(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);

    if (this->leftRightBlendWeight < 0.5f) {
        anim = Player_GetFightingRightAnim(this);
        this->leftRightBlendWeight = 0.0f;
    } else {
        anim = Player_GetFightingLeftAnim(this);
        this->leftRightBlendWeight = 1.0f;
    }

    this->leftRightBlendWeightTarget = this->leftRightBlendWeight;
    Player_PlayAnimLoop(play, this, anim);
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupFriendlyZTargetingStandStill(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_FriendlyZTargetStandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, Player_GetStandingStillAnim(this));
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupStandingStillType(Player* this, PlayState* play) {
    if (Player_IsUnfriendlyZTargeting(this)) {
        Player_SetupUnfriendlyZTargetStandStill(this, play);
    } else if (Player_IsFriendlyZTargeting(this)) {
        Player_SetupFriendlyZTargetingStandStill(this, play);
    } else {
        Player_SetupStandingStillMorph(this, play);
    }
}

void Player_ReturnToStandStill(Player* this, PlayState* play) {
    PlayerActionFunc func;

    if (Player_IsUnfriendlyZTargeting(this)) {
        func = Player_UnfriendlyZTargetStandingStill;
    } else if (Player_IsFriendlyZTargeting(this)) {
        func = Player_FriendlyZTargetStandingStill;
    } else {
        func = Player_StandingStill;
    }

    Player_SetActionFunc(play, this, func, 1);
}

void Player_SetupReturnToStandStill(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    if (Player_IsUnfriendlyZTargeting(this)) {
        this->genericTimer = 1;
    }
}

void Player_SetupReturnToStandStillSetAnim(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetupReturnToStandStill(this, play);
    Player_PlayAnimOnceWithWaterInfluence(play, this, anim);
}

s32 Player_CanHoldActor(Player* this) {
    return (this->interactRangeActor != NULL) && (this->heldActor == NULL);
}

void Player_SetupHoldActor(PlayState* play, Player* this) {
    if (Player_CanHoldActor(this)) {
        Actor* interactRangeActor = this->interactRangeActor;
        s32 interactActorId = interactRangeActor->id;

        if (interactActorId == ACTOR_BG_TOKI_SWD) {
            this->interactRangeActor->parent = &this->actor;
            Player_SetActionFunc(play, this, Player_SetDrawAndStartCutsceneAfterTimer, 0);
            this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        } else {
            LinkAnimationHeader* anim;

            if (interactActorId == ACTOR_BG_HEAVY_BLOCK) {
                Player_SetActionFunc(play, this, Player_ThrowStonePillar, 0);
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                anim = &gPlayerAnim_link_normal_heavy_carry;
            } else if ((interactActorId == ACTOR_EN_ISHI) && ((interactRangeActor->params & 0xF) == 1)) {
                Player_SetActionFunc(play, this, Player_LiftSilverBoulder, 0);
                anim = &gPlayerAnim_link_silver_carry;
            } else if (((interactActorId == ACTOR_EN_BOMBF) || (interactActorId == ACTOR_EN_KUSA)) &&
                       (Player_GetStrength() <= PLAYER_STR_NONE)) {
                Player_SetActionFunc(play, this, Player_FailToLiftActor, 0);
                this->actor.world.pos.x =
                    (Math_SinS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.x;
                this->actor.world.pos.z =
                    (Math_CosS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.z;
                this->currentYaw = this->actor.shape.rot.y = interactRangeActor->yawTowardsPlayer + 0x8000;
                anim = &gPlayerAnim_link_normal_nocarry_free;
            } else {
                Player_SetActionFunc(play, this, Player_LiftActor, 0);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_carryB, this->modelAnimType);
            }

            Player_PlayAnimOnce(play, this, anim);
        }
    } else {
        Player_SetupStandingStillType(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
    }
}

void Player_SetupTalkWithActor(PlayState* play, Player* this) {
    Player_SetActionFuncPreserveMoveFlags(play, this, Player_TalkWithActor, 0);

    this->stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE;

    if (this->actor.textId != 0) {
        Message_StartTextbox(play, this->actor.textId, this->talkActor);
        this->targetActor = this->talkActor;
    }
}

void Player_SetupRideHorse(PlayState* play, Player* this) {
    Player_SetActionFuncPreserveMoveFlags(play, this, Player_RideHorse, 0);
}

void Player_SetupGrabPushPullWall(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_GrabPushPullWall, 0);
}

void Player_SetupClimbingWallOrDownLedge(PlayState* play, Player* this) {
    s32 preservedTimer = this->genericTimer;
    s32 preservedVar = this->genericVar;

    Player_SetActionFuncPreserveMoveFlags(play, this, Player_ClimbingWallOrDownLedge, 0);
    this->actor.velocity.y = 0.0f;

    this->genericTimer = preservedTimer;
    this->genericVar = preservedVar;
}

void Player_SetupInsideCrawlspace(PlayState* play, Player* this) {
    Player_SetActionFuncPreserveMoveFlags(play, this, Player_InsideCrawlspace, 0);
}

void Player_SetupGetItem(PlayState* play, Player* this) {
    Player_SetActionFuncPreserveMoveFlags(play, this, Player_GetItem, 0);

    this->stateFlags1 |= PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_CUTSCENE;

    if (this->getItemId == GI_HEART_CONTAINER_2) {
        this->genericTimer = 20;
    } else if (this->getItemId >= 0) {
        this->genericTimer = 1;
    } else {
        this->getItemId = -this->getItemId;
    }
}

s32 Player_StartJump(Player* this, PlayState* play) {
    s16 yawDiff;
    LinkAnimationHeader* anim;
    f32 yVel;

    yawDiff = this->currentYaw - this->actor.shape.rot.y;

    if ((ABS(yawDiff) < DEG_TO_BINANG(22.5f)) && (this->linearVelocity > 4.0f)) {
        anim = &gPlayerAnim_link_normal_run_jump;
    } else {
        anim = &gPlayerAnim_link_normal_jump;
    }

    if (this->linearVelocity > (IREG(66) / 100.0f)) {
        yVel = IREG(67) / 100.0f;
    } else {
        yVel = (IREG(68) / 100.0f) + ((IREG(69) * this->linearVelocity) / 1000.0f);
    }

    Player_SetupJumpWithSfx(this, anim, yVel, play, NA_SE_VO_LI_AUTO_JUMP);
    this->genericTimer = 1;

    return 1;
}

void Player_SetupGrabLedge(PlayState* play, Player* this, CollisionPoly* colPoly, f32 distToPoly,
                           LinkAnimationHeader* anim) {
    f32 nx = COLPOLY_GET_NORMAL(colPoly->normal.x);
    f32 nz = COLPOLY_GET_NORMAL(colPoly->normal.z);

    Player_SetActionFunc(play, this, Player_GrabLedge, 0);
    Player_ResetAttributesAndHeldActor(play, this);
    Player_PlayAnimOnce(play, this, anim);

    this->actor.world.pos.x -= (distToPoly + 1.0f) * nx;
    this->actor.world.pos.z -= (distToPoly + 1.0f) * nz;
    this->actor.shape.rot.y = this->currentYaw = Math_Atan2S(nz, nx);

    Player_ClearAttentionModeAndStopMoving(this);
    Player_AnimUpdatePrevTranslRot(this);
}

s32 Player_SetupGrabLedgeInsteadOfFalling(Player* this, PlayState* play) {
    CollisionPoly* colPoly;
    s32 polyBgId;
    Vec3f pos;
    Vec3f colPolyPos;
    f32 dist;

    if ((this->actor.yDistToWater < -80.0f) && (ABS(this->angleToFloorX) < DEG_TO_BINANG(15.0f)) &&
        (ABS(this->angleToFloorY) < DEG_TO_BINANG(15.0f))) {
        pos.x = this->actor.prevPos.x - this->actor.world.pos.x;
        pos.z = this->actor.prevPos.z - this->actor.world.pos.z;

        dist = sqrtf(SQ(pos.x) + SQ(pos.z));
        if (dist != 0.0f) {
            dist = 5.0f / dist;
        } else {
            dist = 0.0f;
        }

        pos.x = this->actor.prevPos.x + (pos.x * dist);
        pos.y = this->actor.world.pos.y;
        pos.z = this->actor.prevPos.z + (pos.z * dist);

        if (BgCheck_EntityLineTest1(&play->colCtx, &this->actor.world.pos, &pos, &colPolyPos, &colPoly, true, false,
                                    false, true, &polyBgId) &&
            (ABS(colPoly->normal.y) < 600)) {
            f32 nx = COLPOLY_GET_NORMAL(colPoly->normal.x);
            f32 ny = COLPOLY_GET_NORMAL(colPoly->normal.y);
            f32 nz = COLPOLY_GET_NORMAL(colPoly->normal.z);
            f32 distToPoly;
            s32 shouldClimbDownAdjacentWall;

            distToPoly = Math3D_UDistPlaneToPos(nx, ny, nz, colPoly->dist, &this->actor.world.pos);

            shouldClimbDownAdjacentWall = sFloorProperty == FLOOR_PROPERTY_CLIMB_DOWN_ADJACENT_WALL;
            if (!shouldClimbDownAdjacentWall &&
                (SurfaceType_GetWallFlags(&play->colCtx, colPoly, polyBgId) & WALL_FLAG_3)) {
                shouldClimbDownAdjacentWall = true;
            }

            Player_SetupGrabLedge(play, this, colPoly, distToPoly,
                                  shouldClimbDownAdjacentWall ? &gPlayerAnim_link_normal_Fclimb_startB
                                                              : &gPlayerAnim_link_normal_fall);

            if (shouldClimbDownAdjacentWall) {
                Player_SetupMiniCsFunc(play, this, Player_SetupClimbingWallOrDownLedge);

                this->currentYaw += 0x8000;
                this->actor.shape.rot.y = this->currentYaw;

                this->stateFlags1 |= PLAYER_STATE1_CLIMBING;
                Player_SetupAnimMovement(play, this,
                                         PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                             PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                             PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                             PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);

                this->genericTimer = -1;
                this->genericVar = shouldClimbDownAdjacentWall;
            } else {
                this->stateFlags1 |= PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP;
                this->stateFlags1 &= ~PLAYER_STATE1_Z_TARGETING_FRIENDLY;
            }

            func_8002F7DC(&this->actor, NA_SE_PL_SLIPDOWN);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
            return 1;
        }
    }

    return 0;
}

void Player_SetupClimbOntoLedge(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetActionFunc(play, this, Player_ClimbOntoLedge, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 1.3f);
}

static Vec3f sWaterRaycastOffset = { 0.0f, 0.0f, 100.0f };

void Player_SetupMidairBehavior(Player* this, PlayState* play) {
    s32 yawDiff;
    CollisionPoly* floorPoly;
    s32 floorBgId;
    WaterBox* waterbox;
    Vec3f raycastPos;
    f32 floorPosY;
    f32 waterPosY;

    this->fallDistance = this->fallStartHeight - (s32)this->actor.world.pos.y;

    if (!(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
        !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (!Player_ShouldEnterGrotto(play, this)) {
            if (sFloorProperty == FLOOR_PROPERTY_STOP_XZ_MOMENTUM) {
                this->actor.world.pos.x = this->actor.prevPos.x;
                this->actor.world.pos.z = this->actor.prevPos.z;
                return;
            }

            if (!(this->stateFlags3 & PLAYER_STATE3_MIDAIR) && !(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_7) &&
                (Player_UpdateMidair != this->actionFunc) && (Player_FallingDive != this->actionFunc)) {

                if ((sFloorProperty == FLOOR_PROPERTY_STOP_ALL_MOMENTUM) || (this->isMeleeWeaponAttacking != 0)) {
                    Math_Vec3f_Copy(&this->actor.world.pos, &this->actor.prevPos);
                    Player_StopMovement(this);
                    return;
                }

                if (this->hoverBootsTimer != 0) {
                    this->actor.velocity.y = 1.0f;
                    sFloorProperty = FLOOR_PROPERTY_NO_JUMPING;
                    return;
                }

                yawDiff = (s16)(this->currentYaw - this->actor.shape.rot.y);

                Player_SetActionFunc(play, this, Player_UpdateMidair, 1);
                Player_ResetAttributes(play, this);

                this->floorSfxOffset = this->prevFloorSfxOffset;

                if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE) &&
                    !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) &&
                    (sFloorProperty != FLOOR_PROPERTY_CLIMB_DOWN_ADJACENT_WALL) &&
                    (sFloorProperty != FLOOR_PROPERTY_NO_JUMPING) && (sPlayerYDistToFloor > 20.0f) &&
                    (this->isMeleeWeaponAttacking == 0) && (ABS(yawDiff) < DEG_TO_BINANG(45.0f)) &&
                    (this->linearVelocity > 3.0f)) {

                    if ((sFloorProperty == FLOOR_PROPERTY_FALLING_DIVE) &&
                        !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {

                        floorPosY = Player_RaycastFloorWithOffset(play, this, &sWaterRaycastOffset, &raycastPos,
                                                                  &floorPoly, &floorBgId);
                        waterPosY = this->actor.world.pos.y;

                        if (WaterBox_GetSurface1(play, &play->colCtx, raycastPos.x, raycastPos.z, &waterPosY,
                                                 &waterbox) &&
                            ((waterPosY - floorPosY) > 50.0f)) {
                            Player_SetupJump(this, &gPlayerAnim_link_normal_run_jump_water_fall, 6.0f, play);
                            Player_SetActionFunc(play, this, Player_FallingDive, 0);
                            return;
                        }
                    }

                    Player_StartJump(this, play);
                    return;
                }

                if ((sFloorProperty == FLOOR_PROPERTY_NO_JUMPING) ||
                    (sPlayerYDistToFloor <= this->ageProperties->unk_34) ||
                    !Player_SetupGrabLedgeInsteadOfFalling(this, play)) {
                    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
                    return;
                }
            }
        }
    } else {
        this->fallStartHeight = this->actor.world.pos.y;
    }
}

s32 Player_SetupCameraMode(PlayState* play, Player* this) {
    s32 cameraMode;

    if (this->attentionMode == PLAYER_ATTENTIONMODE_AIMING) {
        if (Actor_PlayerIsAimingFpsItem(this)) {
            if (LINK_IS_ADULT) {
                cameraMode = CAM_MODE_BOWARROW;
            } else {
                cameraMode = CAM_MODE_SLINGSHOT;
            }
        } else {
            cameraMode = CAM_MODE_BOOMERANG;
        }
    } else {
        cameraMode = CAM_MODE_FIRSTPERSON;
    }

    return Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), cameraMode);
}

s32 Player_SetupCutscene(PlayState* play, Player* this) {
    if (this->attentionMode == PLAYER_ATTENTIONMODE_CUTSCENE) {
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        if (this->csFlag != 0) {
            this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        }
        Player_InactivateMeleeWeapon(this);
        return 1;
    } else {
        return 0;
    }
}

void Player_LoadGetItemObject(Player* this, s16 objectId) {
    s32 pad;
    u32 size;

    if (objectId != OBJECT_INVALID) {
        this->giObjectLoading = true;
        osCreateMesgQueue(&this->giObjectLoadQueue, &this->giObjectLoadMsg, 1);

        size = gObjectTable[objectId].vromEnd - gObjectTable[objectId].vromStart;

        LOG_HEX("size", size, "../z_player.c", 9090);
        ASSERT(size <= 1024 * 8, "size <= 1024 * 8", "../z_player.c", 9091);

        DmaMgr_RequestAsync(&this->giObjectDmaRequest, this->giObjectSegment, gObjectTable[objectId].vromStart, size, 0,
                            &this->giObjectLoadQueue, NULL, "../z_player.c", 9099);
    }
}

void Player_SetupMagicSpell(PlayState* play, Player* this, s32 magicSpell) {
    Player_SetActionFuncPreserveItemAP(play, this, Player_UpdateMagicSpell, 0);

    this->magicSpellType = magicSpell - 3;
    Magic_RequestChange(play, sMagicSpellCosts[magicSpell], MAGIC_CONSUME_WAIT_PREVIEW);

    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_link_magic_tame, 0.83f);

    if (magicSpell == 5) {
        this->subCamId = OnePointCutscene_Init(play, 1100, -101, NULL, CAM_ID_MAIN);
    } else {
        Player_SetCameraTurnAround(play, 10);
    }
}

void Player_ResetLookAngles(Player* this) {
    this->actor.focus.rot.x = this->actor.focus.rot.z = this->headRot.x = this->headRot.y = this->headRot.z =
        this->upperBodyRot.x = this->upperBodyRot.y = this->upperBodyRot.z = 0;

    this->actor.focus.rot.y = this->actor.shape.rot.y;
}

static u8 sExchangeGetItemIDs[] = {
    GI_ZELDAS_LETTER,        // EXCH_ITEM_ZELDAS_LETTER
    GI_WEIRD_EGG,            // EXCH_ITEM_WEIRD_EGG
    GI_CHICKEN,              // EXCH_ITEM_CHICKEN
    GI_MAGIC_BEAN,           // EXCH_ITEM_MAGIC_BEAN
    GI_POCKET_EGG,           // EXCH_ITEM_POCKET_EGG
    GI_POCKET_CUCCO,         // EXCH_ITEM_POCKET_CUCCO
    GI_COJIRO,               // EXCH_ITEM_COJIRO
    GI_ODD_MUSHROOM,         // EXCH_ITEM_ODD_MUSHROOM
    GI_ODD_POTION,           // EXCH_ITEM_ODD_POTION
    GI_POACHERS_SAW,         // EXCH_ITEM_POACHERS_SAW
    GI_BROKEN_GORONS_SWORD,  // EXCH_ITEM_BROKEN_GORONS_SWORD
    GI_PRESCRIPTION,         // EXCH_ITEM_PRESCRIPTION
    GI_EYEBALL_FROG,         // EXCH_ITEM_EYEBALL_FROG
    GI_EYE_DROPS,            // EXCH_ITEM_EYE_DROPS
    GI_CLAIM_CHECK,          // EXCH_ITEM_CLAIM_CHECK
    GI_MASK_SKULL,           // EXCH_ITEM_MASK_SKULL
    GI_MASK_SPOOKY,          // EXCH_ITEM_MASK_SPOOKY
    GI_MASK_KEATON,          // EXCH_ITEM_MASK_KEATON
    GI_MASK_BUNNY_HOOD,      // EXCH_ITEM_MASK_BUNNY_HOOD
    GI_MASK_TRUTH,           // EXCH_ITEM_MASK_TRUTH
    GI_MASK_GORON,           // EXCH_ITEM_MASK_GORON
    GI_MASK_ZORA,            // EXCH_ITEM_MASK_ZORA
    GI_MASK_GERUDO,          // EXCH_ITEM_MASK_GERUDO
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_FISH
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_BLUE_FIRE
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_BUG
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_POE
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_BIG_POE
    GI_BOTTLE_RUTOS_LETTER, // EXCH_ITEM_BOTTLE_RUTOS_LETTER
};

static LinkAnimationHeader* sExchangeItemAnims[] = {
    &gPlayerAnim_link_normal_give_other,
    &gPlayerAnim_link_bottle_read,
    &gPlayerAnim_link_normal_take_out,
};

s32 Player_SetupItemCutsceneOrFirstPerson(Player* this, PlayState* play) {
    s32 item;
    s32 sp28;
    GetItemEntry* giEntry;
    Actor* talkActor;

    if ((this->attentionMode != PLAYER_ATTENTIONMODE_NONE) &&
        (Player_IsSwimming(this) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
         (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE))) {

        if (!Player_SetupCutscene(play, this)) {
            if (this->attentionMode == PLAYER_ATTENTIONMODE_ITEM_CUTSCENE) {
                item = Player_ActionToMagicSpell(this, this->itemAction);
                if (item >= PLAYER_MAGICSPELL_UNUSED_15) {
                    if ((item != PLAYER_MAGICSPELL_FARORES_WIND) ||
                        (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        Player_SetupMagicSpell(play, this, item);
                    } else {
                        Player_SetActionFunc(play, this, Player_ChooseFaroresWindOption, 1);
                        this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                        Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
                        Player_SetCameraTurnAround(play, 4);
                    }

                    Player_ClearAttentionModeAndStopMoving(this);
                    return 1;
                }

                item = this->itemAction - PLAYER_IA_ZELDAS_LETTER;
                if ((item >= 0) ||
                    (sp28 = Player_ActionToBottle(this, this->itemAction) - 1,
                     ((sp28 >= 0) && (sp28 < 6) &&
                      ((this->itemAction > PLAYER_IA_BOTTLE_POE) ||
                       ((this->talkActor != NULL) && (((this->itemAction == PLAYER_IA_BOTTLE_POE) &&
                                                       (this->exchangeItemId == EXCH_ITEM_BOTTLE_POE)) ||
                                                      (this->exchangeItemId == EXCH_ITEM_BOTTLE_BLUE_FIRE))))))) {

                    if ((play->actorCtx.titleCtx.delayTimer == 0) && (play->actorCtx.titleCtx.alpha == 0)) {
                        Player_SetActionFuncPreserveItemAP(play, this, Player_PresentExchangeItem, 0);

                        if (item >= 0) {
                            giEntry = &sGetItemTable[sExchangeGetItemIDs[item] - 1];
                            Player_LoadGetItemObject(this, giEntry->objectId);
                        }

                        this->stateFlags1 |=
                            PLAYER_STATE1_TALKING | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;

                        if (item >= 0) {
                            item = item + 1;
                        } else {
                            item = sp28 + 0x18;
                        }

                        talkActor = this->talkActor;

                        if ((talkActor != NULL) &&
                            ((this->exchangeItemId == item) || (this->exchangeItemId == EXCH_ITEM_BOTTLE_BLUE_FIRE) ||
                             ((this->exchangeItemId == EXCH_ITEM_BOTTLE_POE) &&
                              (this->itemAction == PLAYER_IA_BOTTLE_BIG_POE)) ||
                             ((this->exchangeItemId == EXCH_ITEM_MAGIC_BEAN) &&
                              (this->itemAction == PLAYER_IA_BOTTLE_BUG))) &&
                            ((this->exchangeItemId != EXCH_ITEM_MAGIC_BEAN) ||
                             (this->itemAction == PLAYER_IA_MAGIC_BEAN))) {
                            if (this->exchangeItemId == EXCH_ITEM_MAGIC_BEAN) {
                                Inventory_ChangeAmmo(ITEM_MAGIC_BEAN, -1);
                                Player_SetActionFuncPreserveItemAP(play, this, func_8084279C, 0);
                                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                                this->genericTimer = 0x50;
                                this->genericVar = -1;
                            }
                            talkActor->flags |= ACTOR_FLAG_8;
                            this->targetActor = this->talkActor;
                        } else if (item == EXCH_ITEM_BOTTLE_RUTOS_LETTER) {
                            this->genericVar = 1;
                            this->actor.textId = 0x4005;
                            Player_SetCameraTurnAround(play, 1);
                        } else {
                            this->genericVar = 2;
                            this->actor.textId = 0xCF;
                            Player_SetCameraTurnAround(play, 4);
                        }

                        this->actor.flags |= ACTOR_FLAG_8;
                        this->exchangeItemId = item;

                        if (this->genericVar < 0) {
                            Player_ChangeAnimMorphToLastFrame(
                                play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_check, this->modelAnimType));
                        } else {
                            Player_PlayAnimOnce(play, this, sExchangeItemAnims[this->genericVar]);
                        }

                        Player_ClearAttentionModeAndStopMoving(this);
                    }
                    return 1;
                }

                item = Player_ActionToBottle(this, this->itemAction);
                if (item >= PLAYER_BOTTLECONTENTS_NONE) {
                    if (item == PLAYER_BOTTLECONTENTS_FAIRY) {
                        Player_SetActionFuncPreserveItemAP(play, this, Player_HealWithFairy, 0);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_bottle_bug_out);
                        Player_SetCameraTurnAround(play, 3);
                    } else if ((item > PLAYER_BOTTLECONTENTS_NONE) && (item < PLAYER_BOTTLECONTENTS_POE)) {
                        Player_SetActionFuncPreserveItemAP(play, this, Player_DropItemFromBottle, 0);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_bottle_fish_out);
                        Player_SetCameraTurnAround(play, (item == 1) ? 1 : 5);
                    } else {
                        Player_SetActionFuncPreserveItemAP(play, this, Player_DrinkFromBottle, 0);
                        Player_ChangeAnimSlowedMorphToLastFrame(play, this, &gPlayerAnim_link_bottle_drink_demo_start);
                        Player_SetCameraTurnAround(play, 2);
                    }
                } else {
                    Player_SetActionFuncPreserveItemAP(play, this, Player_PlayOcarina, 0);
                    Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_normal_okarina_start);
                    this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_GENERAL;
                    Player_SetCameraTurnAround(play, (this->ocarinaActor != NULL) ? 0x5B : 0x5A);
                    if (this->ocarinaActor != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR;
                        Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->ocarinaActor);
                    }
                }
            } else if (Player_SetupCameraMode(play, this)) {
                if (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
                    Player_SetActionFunc(play, this, Player_FirstPersonAiming, 1);
                    this->genericTimer = 13;
                    Player_ResetLookAngles(this);
                }
                this->stateFlags1 |= PLAYER_STATE1_IN_FIRST_PERSON_MODE;
                func_80078884(NA_SE_SY_CAMERA_ZOOM_UP);
                Player_StopMovement(this);
                return 1;
            } else {
                this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
                func_80078884(NA_SE_SY_ERROR);
                return 0;
            }

            this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
        }

        Player_ClearAttentionModeAndStopMoving(this);
        return 1;
    }

    return 0;
}

s32 Player_SetupSpeakOrCheck(Player* this, PlayState* play) {
    Actor* talkActor = this->talkActor;
    Actor* targetActor = this->targetActor;
    Actor* naviActor = NULL;
    s32 naviHasText = false;
    s32 targetActorHasText;

    targetActorHasText = (targetActor != NULL) && (CHECK_FLAG_ALL(targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) ||
                                                   (targetActor->naviEnemyId != NAVI_ENEMY_NONE));

    if (targetActorHasText || (this->naviTextId != 0)) {
        naviHasText = (this->naviTextId < 0) && ((ABS(this->naviTextId) & 0xFF00) != 0x200);
        if (naviHasText || !targetActorHasText) {
            naviActor = this->naviActor;
            if (naviHasText) {
                targetActor = NULL;
                talkActor = NULL;
            }
        } else {
            naviActor = targetActor;
        }
    }

    if ((talkActor != NULL) || (naviActor != NULL)) {
        if ((targetActor == NULL) || (targetActor == talkActor) || (targetActor == naviActor)) {
            if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                ((this->heldActor != NULL) &&
                 (naviHasText || (talkActor == this->heldActor) || (naviActor == this->heldActor) ||
                  ((talkActor != NULL) && (talkActor->flags & ACTOR_FLAG_16))))) {
                if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                    (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) ||
                    (Player_IsSwimming(this) && !(this->stateFlags2 & PLAYER_STATE2_DIVING))) {

                    if (talkActor != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_CAN_SPEAK_OR_CHECK;
                        if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) || (talkActor->flags & ACTOR_FLAG_16)) {
                            naviActor = NULL;
                        } else if (naviActor == NULL) {
                            return 0;
                        }
                    }

                    if (naviActor != NULL) {
                        if (!naviHasText) {
                            this->stateFlags2 |= PLAYER_STATE2_NAVI_REQUESTING_TALK;
                        }

                        if (!CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) && !naviHasText) {
                            return 0;
                        }

                        talkActor = naviActor;
                        this->talkActor = NULL;

                        if (naviHasText || !targetActorHasText) {
                            if (this->naviTextId >= 0) {
                                naviActor->textId = this->naviTextId;
                            } else {
                                naviActor->textId = -this->naviTextId;
                            }
                        } else {
                            if (naviActor->naviEnemyId != NAVI_ENEMY_NONE) {
                                naviActor->textId = naviActor->naviEnemyId + 0x600;
                            }
                        }
                    }

                    this->currentMask = sCurrentMask;
                    Player_StartTalkingWithActor(play, talkActor);
                    return 1;
                }
            }
        }
    }

    return 0;
}

s32 Player_ForceFirstPerson(Player* this, PlayState* play) {
    if (!(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_RIDING_HORSE)) &&
        Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_FIRSTPERSON)) {
        if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
            (Player_IsSwimming(this) && (this->actor.yDistToWater < this->ageProperties->diveWaterSurface))) {
            this->attentionMode = PLAYER_ATTENTIONMODE_C_UP;
            return 1;
        }
    }

    return 0;
}

s32 Player_SetupCUpBehavior(Player* this, PlayState* play) {
    if (this->attentionMode != PLAYER_ATTENTIONMODE_NONE) {
        Player_SetupItemCutsceneOrFirstPerson(this, play);
        return 1;
    }

    if ((this->targetActor != NULL) && (CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) ||
                                        (this->targetActor->naviEnemyId != NAVI_ENEMY_NONE))) {
        this->stateFlags2 |= PLAYER_STATE2_NAVI_REQUESTING_TALK;
    } else if ((this->naviTextId == 0) && !Player_IsUnfriendlyZTargeting(this) &&
               CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) &&
               (R_SCENE_CAM_TYPE != SCENE_CAM_TYPE_FIXED_SHOP_VIEWPOINT) &&
               (R_SCENE_CAM_TYPE != SCENE_CAM_TYPE_FIXED_TOGGLE_VIEWPOINT) && !Player_ForceFirstPerson(this, play)) {
        func_80078884(NA_SE_SY_ERROR);
    }

    return 0;
}

void Player_SetupJumpSlash(PlayState* play, Player* this, s32 arg2, f32 xzVelocity, f32 yVelocity) {
    Player_StartMeleeWeaponAttack(play, this, arg2);
    Player_SetActionFunc(play, this, Player_JumpSlash, 0);

    this->stateFlags3 |= PLAYER_STATE3_MIDAIR;

    this->currentYaw = this->actor.shape.rot.y;
    this->linearVelocity = xzVelocity;
    this->actor.velocity.y = yVelocity;

    this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    this->hoverBootsTimer = 0;

    Player_PlayJumpSfx(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
}

s32 Player_CanJumpSlash(Player* this) {
    if (!(this->stateFlags1 & PLAYER_STATE1_22) && (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE)) {
        if (sUsingItemAlreadyInHand ||
            ((this->actor.category != ACTORCAT_PLAYER) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B))) {
            return 1;
        }
    }

    return 0;
}

s32 Player_SetupMidairJumpSlash(Player* this, PlayState* play) {
    if (Player_CanJumpSlash(this) && (sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE)) {
        Player_SetupJumpSlash(play, this, PLAYER_MELEEATKTYPE_JUMPSLASH_START, 3.0f, 4.5f);
        return 1;
    }

    return 0;
}

void Player_SetupRolling(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_Rolling, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime,
                                   GET_PLAYER_ANIM(PLAYER_ANIMGROUP_landing_roll, this->modelAnimType),
                                   1.25f * sWaterSpeedScale);
}

s32 Player_CanRoll(Player* this, PlayState* play) {
    if ((this->relativeAnalogStickInputs[this->inputFrameCounter] == PLAYER_RELATIVESTICKINPUT_FORWARD) &&
        (sFloorType != 7)) {
        Player_SetupRolling(this, play);
        return 1;
    }

    return 0;
}

void Player_SetupBackflipOrSidehop(Player* this, PlayState* play, s32 relativeStickInput) {
    Player_SetupJumpWithSfx(this, sManualJumpAnims[relativeStickInput][0],
                            !(relativeStickInput & PLAYER_RELATIVESTICKINPUT_LEFT) ? 5.8f : 3.5f, play,
                            NA_SE_VO_LI_SWORD_N);

    if (relativeStickInput) {}

    this->genericTimer = 1;
    this->relativeStickInput = relativeStickInput;

    this->currentYaw = this->actor.shape.rot.y + (relativeStickInput << 0xE);
    this->linearVelocity = !(relativeStickInput & PLAYER_RELATIVESTICKINPUT_LEFT) ? 6.0f : 8.5f;

    this->stateFlags2 |= PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING;

    func_8002F7DC(&this->actor, ((relativeStickInput << 0xE) == 0x8000) ? NA_SE_PL_ROLL : NA_SE_PL_SKIP);
}

s32 Player_SetupJumpSlashOrRoll(Player* this, PlayState* play) {
    s32 relativeStickInput;

    if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
        (sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE) &&
        (SurfaceType_GetFloorEffect(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) != FLOOR_EFFECT_1)) {
        relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];

        if (relativeStickInput <= PLAYER_RELATIVESTICKINPUT_FORWARD) {
            if (Player_IsZTargeting(this)) {
                if (this->actor.category != ACTORCAT_PLAYER) {
                    if (relativeStickInput < 0) {
                        Player_SetupJump(this, &gPlayerAnim_link_normal_jump, REG(69) / 100.0f, play);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                } else {
                    if ((Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) && Player_CanUseItem(this)) {
                        Player_SetupJumpSlash(play, this, PLAYER_MELEEATKTYPE_JUMPSLASH_START, 5.0f, 5.0f);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                }
                return 1;
            }
        } else {
            Player_SetupBackflipOrSidehop(this, play, relativeStickInput);
            return 1;
        }
    }

    return 0;
}

void Player_EndRun(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;

    frame = this->walkFrame - 3.0f;
    if (frame < 0.0f) {
        frame += 29.0f;
    }

    if (frame < 14.0f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk_endL, this->modelAnimType);
        frame = 11.0f - frame;
        if (frame < 0.0f) {
            frame = 1.375f * -frame;
        }
        frame /= 11.0f;
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk_endR, this->modelAnimType);
        frame = 26.0f - frame;
        if (frame < 0.0f) {
            frame = 2 * -frame;
        }
        frame /= 12.0f;
    }

    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE,
                         4.0f * frame);
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupEndRun(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    Player_EndRun(this, play);
}

void Player_SetupStandingStillNoMorph(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_StandingStill, 1);
    Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_ClearLookAndAttention(Player* this, PlayState* play) {
    if (!(this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        Player_ResetLookAngles(this);
        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            Player_SetupSwimIdle(play, this);
        } else {
            Player_SetupStandingStillType(this, play);
        }
        if (this->attentionMode < PLAYER_ATTENTIONMODE_ITEM_CUTSCENE) {
            this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
        }
    }

    this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                           PLAYER_STATE1_IN_FIRST_PERSON_MODE);
}

s32 Player_SetupRollOrPutAway(Player* this, PlayState* play) {
    if (!Player_SetupStartUnfriendlyZTargeting(this) && (D_808535E0 == 0) &&
        !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        if (Player_CanRoll(this, play)) {
            return 1;
        }
        if ((this->putAwayTimer == 0) && (this->heldItemAction >= PLAYER_IA_SWORD_MASTER)) {
            Player_UseItem(play, this, ITEM_NONE);
        } else {
            this->stateFlags2 ^= PLAYER_STATE2_NAVI_IS_ACTIVE;
        }
    }

    return 0;
}

s32 Player_SetupDefend(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;

    if ((play->shootingGalleryStatus == 0) && (this->currentShield != PLAYER_SHIELD_NONE) &&
        CHECK_BTN_ALL(sControlInput->cur.button, BTN_R) &&
        (Player_IsChildWithHylianShield(this) || (!Player_IsFriendlyZTargeting(this) && (this->targetActor == NULL)))) {

        Player_InactivateMeleeWeapon(this);
        Player_DetatchHeldActor(play, this);

        if (Player_SetActionFunc(play, this, Player_AimShieldCrouched, 0)) {
            this->stateFlags1 |= PLAYER_STATE1_22;

            if (!Player_IsChildWithHylianShield(this)) {
                Player_SetModelsForHoldingShield(this);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense, this->modelAnimType);
            } else {
                anim = &gPlayerAnim_clink_normal_defense_ALL;
            }

            if (anim != this->skelAnime.animation) {
                if (Player_IsUnfriendlyZTargeting(this)) {
                    this->unk_86C = 1.0f;
                } else {
                    this->unk_86C = 0.0f;
                    Player_ResetLeftRightBlendWeight(this);
                }
                this->upperBodyRot.x = this->upperBodyRot.y = this->upperBodyRot.z = 0;
            }

            frame = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);

            if (Player_IsChildWithHylianShield(this)) {
                Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE);
            }

            func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_POSTURE);
        }

        return 1;
    }

    return 0;
}

s32 Player_SetupTurnAroundRunning(Player* this, f32* targetVelocity, s16* targetYaw) {
    s16 yawDiff = this->currentYaw - *targetYaw;

    if (ABS(yawDiff) > DEG_TO_BINANG(135.0f)) {
        if (Player_StepLinearVelocityToZero(this)) {
            *targetVelocity = 0.0f;
            *targetYaw = this->currentYaw;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_SetupDeactivateComboTimer(Player* this) {
    if ((this->comboTimer > 0) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        this->comboTimer = -this->comboTimer;
    }
}

s32 Player_SetupStartChargeSpinAttack(Player* this, PlayState* play) {
    if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_22) && (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) &&
            (this->comboTimer == 1) && (this->heldItemAction != PLAYER_IA_DEKU_STICK)) {
            if ((this->heldItemAction != PLAYER_IA_SWORD_BGS) || (gSaveContext.swordHealth > 0.0f)) {
                Player_StartChargeSpinAttack(play, this);
                return 1;
            }
        }
    } else {
        Player_SetupDeactivateComboTimer(this);
    }

    return 0;
}

s32 Player_SetupThrowDekuNut(PlayState* play, Player* this) {
    if ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (AMMO(ITEM_DEKU_NUT) != 0)) {
        Player_SetActionFunc(play, this, Player_ThrowDekuNut, 0);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_light_bom);
        this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
        return 1;
    }

    return 0;
}

static BottleSwingAnimInfo sBottleSwingAnims[] = {
    { &gPlayerAnim_link_bottle_bug_miss, &gPlayerAnim_link_bottle_bug_in, 2, 3 },
    { &gPlayerAnim_link_bottle_fish_miss, &gPlayerAnim_link_bottle_fish_in, 5, 3 },
};

s32 Player_CanSwingBottleOrCastFishingRod(PlayState* play, Player* this) {
    Vec3f checkPos;

    if (sUsingItemAlreadyInHand) {
        if (Player_GetBottleHeld(this) >= 0) {
            Player_SetActionFunc(play, this, Player_SwingBottle, 0);

            if (this->actor.yDistToWater > 12.0f) {
                this->genericTimer = 1;
            }

            Player_PlayAnimOnceSlowed(play, this, sBottleSwingAnims[this->genericTimer].bottleSwingAnim);

            func_8002F7DC(&this->actor, NA_SE_IT_SWORD_SWING);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
            return 1;
        }

        if (this->heldItemAction == PLAYER_IA_FISHING_POLE) {
            checkPos = this->actor.world.pos;
            checkPos.y += 50.0f;

            if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->actor.world.pos.z > 1300.0f) ||
                BgCheck_SphVsFirstPoly(&play->colCtx, &checkPos, 20.0f)) {
                func_80078884(NA_SE_SY_ERROR);
                return 0;
            }

            Player_SetActionFunc(play, this, Player_CastFishingRod, 0);
            this->fishingState = 1;
            Player_StopMovement(this);
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_fishing_throw);
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

void Player_SetupRun(Player* this, PlayState* play) {
    PlayerActionFunc func;

    if (Player_IsZTargeting(this)) {
        func = Player_ZTargetingRun;
    } else {
        func = Player_Run;
    }

    Player_SetActionFunc(play, this, func, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_run, this->modelAnimType));

    this->walkAngleToFloorX = 0;
    this->unk_864 = this->walkFrame = 0.0f;
}

void Player_SetupZTargetRunning(Player* this, PlayState* play, s16 arg2) {
    this->actor.shape.rot.y = this->currentYaw = arg2;
    Player_SetupRun(this, play);
}

s32 Player_SetupDefaultSpawnBehavior(PlayState* play, Player* this, f32 arg2) {
    WaterBox* waterbox;
    f32 posY;

    posY = this->actor.world.pos.y;
    if (WaterBox_GetSurface1(play, &play->colCtx, this->actor.world.pos.x, this->actor.world.pos.z, &posY, &waterbox) !=
        0) {
        posY -= this->actor.world.pos.y;
        if (this->ageProperties->unk_24 <= posY) {
            Player_SetActionFunc(play, this, Player_SpawnSwimming, 0);
            Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
            this->stateFlags1 |= PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE;
            this->genericTimer = 20;
            this->linearVelocity = 2.0f;
            Player_SetBootData(play, this);
            return 0;
        }
    }

    Player_SetupMiniCsMovement(play, this, arg2, this->actor.shape.rot.y);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    return 1;
}

void Player_SpawnNoMomentum(PlayState* play, Player* this) {
    if (Player_SetupDefaultSpawnBehavior(play, this, 180.0f)) {
        this->genericTimer = -20;
    }
}

void Player_SpawnWalkingSlow(PlayState* play, Player* this) {
    this->linearVelocity = 2.0f;
    gSaveContext.entranceSpeed = 2.0f;
    if (Player_SetupDefaultSpawnBehavior(play, this, 120.0f)) {
        this->genericTimer = -15;
    }
}

void Player_SpawnWalkingPreserveMomentum(PlayState* play, Player* this) {
    if (gSaveContext.entranceSpeed < 0.1f) {
        gSaveContext.entranceSpeed = 0.1f;
    }

    this->linearVelocity = gSaveContext.entranceSpeed;

    if (Player_SetupDefaultSpawnBehavior(play, this, 800.0f)) {
        this->genericTimer = -80 / this->linearVelocity;
        if (this->genericTimer < -20) {
            this->genericTimer = -20;
        }
    }
}

void Player_SetupFriendlyBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetActionFunc(play, this, Player_FriendlyBackwalk, 1);
    LinkAnimation_CopyJointToMorph(play, &this->skelAnime);
    this->unk_864 = this->walkFrame = 0.0f;
    this->currentYaw = yaw;
}

void Player_SetupFriendlySidewalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndSidewalk, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType));
}

void Player_SetupUnfriendlyBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UnfriendlyBackwalk, 1);
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_anchor_back_walk, 2.2f, 0.0f,
                         Animation_GetLastFrame(&gPlayerAnim_link_anchor_back_walk), ANIMMODE_ONCE, -6.0f);
    this->linearVelocity = 8.0f;
    this->currentYaw = yaw;
}

void Player_SetupSidewalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_Sidewalk, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkR, this->modelAnimType));
    this->walkFrame = 0.0f;
}

void Player_SetupEndUnfriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndUnfriendlyBackwalk, 1);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_link_anchor_back_brake, 2.0f);
}

void Player_SetupTurn(PlayState* play, Player* this, s16 yaw) {
    this->currentYaw = yaw;
    Player_SetActionFunc(play, this, Player_Turn, 1);
    this->unk_87E = 1200;
    this->unk_87E *= sWaterSpeedScale;
    LinkAnimation_Change(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_45_turn, this->modelAnimType), 1.0f,
                         0.0f, 0.0f, ANIMMODE_LOOP, -6.0f);
}

void Player_EndUnfriendlyZTarget(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    Player_SetActionFunc(play, this, Player_StandingStill, 1);

    if (this->leftRightBlendWeight < 0.5f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitR2wait, this->modelAnimType);
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_waitL2wait, this->modelAnimType);
    }
    Player_PlayAnimOnce(play, this, anim);

    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupUnfriendlyZTarget(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait2waitR, this->modelAnimType));
    this->genericTimer = 1;
}

void Player_SetupEndUnfriendlyZTarget(Player* this, PlayState* play) {
    if (this->linearVelocity != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_EndUnfriendlyZTarget(this, play);
    }
}

void Player_EndMiniCsMovement(Player* this, PlayState* play) {
    if (this->linearVelocity != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_SetupStandingStillType(this, play);
    }
}

s32 Player_SetupSpawnSplash(PlayState* play, Player* this, f32 yVelocity, s32 splashScale) {
    f32 yVelEnteringWater = fabsf(yVelocity);
    WaterBox* waterbox;
    f32 waterSurfaceY;
    Vec3f splashPos;
    s32 splashType;

    if (yVelEnteringWater > 2.0f) {
        splashPos.x = this->bodyPartsPos[PLAYER_BODYPART_WAIST].x;
        splashPos.z = this->bodyPartsPos[PLAYER_BODYPART_WAIST].z;
        waterSurfaceY = this->actor.world.pos.y;
        if (WaterBox_GetSurface1(play, &play->colCtx, splashPos.x, splashPos.z, &waterSurfaceY, &waterbox)) {
            if ((waterSurfaceY - this->actor.world.pos.y) < 100.0f) {
                splashType = (yVelEnteringWater <= 10.0f) ? 0 : 1;
                splashPos.y = waterSurfaceY;
                EffectSsGSplash_Spawn(play, &splashPos, NULL, NULL, splashType, splashScale);
                return 1;
            }
        }
    }

    return 0;
}

void Player_StartJumpOutOfWater(PlayState* play, Player* this, f32 yVelocity) {
    this->stateFlags1 |= PLAYER_STATE1_JUMPING;
    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;

    Player_ResetSubCam(play, this);
    if (Player_SetupSpawnSplash(play, this, yVelocity, 500)) {
        func_8002F7DC(&this->actor, NA_SE_EV_JUMP_OUT_WATER);
    }

    Player_SetBootData(play, this);
}

s32 Player_SetupDive(PlayState* play, Player* this, Input* input) {
    if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
        if ((input == NULL) ||
            (CHECK_BTN_ALL(input->press.button, BTN_A) && (ABS(this->shapePitchOffset) < DEG_TO_BINANG(65.918f)) &&
             (this->currentBoots != PLAYER_BOOTS_IRON))) {

            Player_SetActionFunc(play, this, Player_Dive, 0);
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_swimer_swim_deep_start);

            this->shapePitchOffset = 0;
            this->stateFlags2 |= PLAYER_STATE2_DIVING;
            this->actor.velocity.y = 0.0f;

            if (input != NULL) {
                this->stateFlags2 |= PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER;
                func_8002F7DC(&this->actor, NA_SE_PL_DIVE_BUBBLE);
            }

            return 1;
        }
    }

    if ((this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) || (this->stateFlags2 & PLAYER_STATE2_DIVING)) {
        if (this->actor.velocity.y > 0.0f) {
            if (this->actor.yDistToWater < this->ageProperties->unk_30) {

                this->stateFlags2 &= ~PLAYER_STATE2_DIVING;

                if (input != NULL) {
                    Player_SetActionFunc(play, this, Player_GetItemInWater, 1);

                    if (this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) {
                        this->stateFlags1 |=
                            PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                    }

                    this->genericTimer = 2;
                }

                Player_ResetSubCam(play, this);
                Player_ChangeAnimMorphToLastFrame(play, this,
                                                  (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)
                                                      ? &gPlayerAnim_link_swimer_swim_get
                                                      : &gPlayerAnim_link_swimer_swim_deep_end);

                if (Player_SetupSpawnSplash(play, this, this->actor.velocity.y, 500)) {
                    func_8002F7DC(&this->actor, NA_SE_PL_FACE_UP);
                }

                return 1;
            }
        }
    }

    return 0;
}

void Player_RiseFromDive(PlayState* play, Player* this) {
    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_swimer_swim);
    this->shapePitchOffset = DEG_TO_BINANG(87.891f);
    this->genericTimer = 1;
}

void func_8083D36C(PlayState* play, Player* this) {
    if ((this->currentBoots != PLAYER_BOOTS_IRON) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_ResetAttributesAndHeldActor(play, this);

        if ((this->currentBoots != PLAYER_BOOTS_IRON) && (this->stateFlags2 & PLAYER_STATE2_DIVING)) {
            this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
            Player_SetupDive(play, this, 0);
            this->genericVar = 1;
        } else if (Player_FallingDive == this->actionFunc) {
            Player_SetActionFunc(play, this, Player_Dive, 0);
            Player_RiseFromDive(play, this);
        } else {
            Player_SetActionFunc(play, this, Player_UpdateSwimIdle, 1);
            Player_ChangeAnimMorphToLastFrame(play, this,
                                              (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
                                                  ? &gPlayerAnim_link_swimer_wait2swim_wait
                                                  : &gPlayerAnim_link_swimer_land2swim_wait);
        }
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
        (this->actor.yDistToWater < this->ageProperties->diveWaterSurface)) {
        if (Player_SetupSpawnSplash(play, this, this->actor.velocity.y, 500)) {
            func_8002F7DC(&this->actor, NA_SE_EV_DIVE_INTO_WATER);

            if (this->fallDistance > 800.0f) {
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
            }
        }
    }

    this->stateFlags1 |= PLAYER_STATE1_SWIMMING;
    this->stateFlags2 |= PLAYER_STATE2_DIVING;
    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);
    this->rippleTimer = 0.0f;

    Player_SetBootData(play, this);
}

void func_8083D53C(PlayState* play, Player* this) {
    if (this->actor.yDistToWater < this->ageProperties->diveWaterSurface) {
        Audio_SetBaseFilter(0);
        this->underwaterTimer = 0;
    } else {
        Audio_SetBaseFilter(0x20);
        if (this->underwaterTimer < 300) {
            this->underwaterTimer++;
        }
    }

    if ((Player_JumpUpToLedge != this->actionFunc) && (Player_ClimbOntoLedge != this->actionFunc)) {
        if (this->ageProperties->diveWaterSurface < this->actor.yDistToWater) {
            if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
                (!((this->currentBoots == PLAYER_BOOTS_IRON) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) &&
                 (Player_DamagedSwim != this->actionFunc) && (Player_Drown != this->actionFunc) &&
                 (Player_UpdateSwimIdle != this->actionFunc) && (Player_Swim != this->actionFunc) &&
                 (Player_ZTargetSwimming != this->actionFunc) && (Player_Dive != this->actionFunc) &&
                 (Player_GetItemInWater != this->actionFunc) && (Player_SpawnSwimming != this->actionFunc))) {
                func_8083D36C(play, this);
                return;
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_SWIMMING) &&
                   (this->actor.yDistToWater < this->ageProperties->unk_24)) {
            if ((this->skelAnime.moveFlags == 0) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
                Player_SetupTurn(play, this, this->actor.shape.rot.y);
            }
            Player_StartJumpOutOfWater(play, this, this->actor.velocity.y);
        }
    }
}

void func_8083D6EC(PlayState* play, Player* this) {
    Vec3f ripplePos;
    f32 unsinkSpeed;
    f32 maxSinkSpeed;
    f32 sinkSpeed;
    f32 posDiffMag;

    this->actor.minVelocityY = -20.0f;
    this->actor.gravity = REG(68) / 100.0f;

    if (Player_IsFloorSinkingSand(sFloorType)) {
        unsinkSpeed = fabsf(this->linearVelocity) * 20.0f;
        sinkSpeed = 0.0f;

        if (sFloorType == FLOOR_TYPE_SHALLOW_SAND) {
            if (this->shapeOffsetY > 1300.0f) {
                maxSinkSpeed = this->shapeOffsetY;
            } else {
                maxSinkSpeed = 1300.0f;
            }
            if (this->currentBoots == PLAYER_BOOTS_HOVER) {
                unsinkSpeed += unsinkSpeed;
            } else if (this->currentBoots == PLAYER_BOOTS_IRON) {
                unsinkSpeed *= 0.3f;
            }
        } else {
            maxSinkSpeed = 20000.0f;
            if (this->currentBoots != PLAYER_BOOTS_HOVER) {
                unsinkSpeed += unsinkSpeed;
            } else if ((sFloorType == FLOOR_TYPE_QUICKSAND_NO_HORSE) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                unsinkSpeed = 0;
            }
        }

        if (this->currentBoots != PLAYER_BOOTS_HOVER) {
            sinkSpeed = (maxSinkSpeed - this->shapeOffsetY) * 0.02f;
            sinkSpeed = CLAMP(sinkSpeed, 0.0f, 300.0f);
            if (this->currentBoots == PLAYER_BOOTS_IRON) {
                sinkSpeed += sinkSpeed;
            }
        }

        this->shapeOffsetY += sinkSpeed - unsinkSpeed;
        this->shapeOffsetY = CLAMP(this->shapeOffsetY, 0.0f, maxSinkSpeed);

        this->actor.gravity -= this->shapeOffsetY * 0.004f;
    } else {
        this->shapeOffsetY = 0.0f;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
        if (this->actor.yDistToWater < 50.0f) {
            posDiffMag = fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].x - this->prevWaistPos.x) +
                         fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].y - this->prevWaistPos.y) +
                         fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].z - this->prevWaistPos.z);
            if (posDiffMag > 4.0f) {
                posDiffMag = 4.0f;
            }
            this->rippleTimer += posDiffMag;

            if (this->rippleTimer > 15.0f) {
                this->rippleTimer = 0.0f;

                ripplePos.x = (Rand_ZeroOne() * 10.0f) + this->actor.world.pos.x;
                ripplePos.y = this->actor.world.pos.y + this->actor.yDistToWater;
                ripplePos.z = (Rand_ZeroOne() * 10.0f) + this->actor.world.pos.z;
                EffectSsGRipple_Spawn(play, &ripplePos, 100, 500, 0);

                if ((this->linearVelocity > 4.0f) && !Player_IsSwimming(this) &&
                    ((this->actor.world.pos.y + this->actor.yDistToWater) <
                     this->bodyPartsPos[PLAYER_BODYPART_WAIST].y)) {
                    Player_SetupSpawnSplash(play, this, 20.0f,
                                            (fabsf(this->linearVelocity) * 50.0f) + (this->actor.yDistToWater * 5.0f));
                }
            }
        }

        if (this->actor.yDistToWater > 40.0f) {
            s32 numBubbles = 0;
            s32 i;

            if ((this->actor.velocity.y > -1.0f) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                if (Rand_ZeroOne() < 0.2f) {
                    numBubbles = 1;
                }
            } else {
                numBubbles = this->actor.velocity.y * -2.0f;
            }

            for (i = 0; i < numBubbles; i++) {
                EffectSsBubble_Spawn(play, &this->actor.world.pos, 20.0f, 10.0f, 20.0f, 0.13f);
            }
        }
    }
}

s32 Player_LookAtTargetActor(Player* this, s32 flag) {
    Actor* targetActor = this->targetActor;
    Vec3f sp30;
    s16 sp2E;
    s16 sp2C;

    sp30.x = this->actor.world.pos.x;
    sp30.y = this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 3.0f;
    sp30.z = this->actor.world.pos.z;
    sp2E = Math_Vec3f_Pitch(&sp30, &targetActor->focus.pos);
    sp2C = Math_Vec3f_Yaw(&sp30, &targetActor->focus.pos);
    Math_SmoothStepToS(&this->actor.focus.rot.y, sp2C, 4, 10000, 0);
    Math_SmoothStepToS(&this->actor.focus.rot.x, sp2E, 4, 10000, 0);
    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y;

    return Player_UpdateLookAngles(this, flag);
}

static Vec3f D_8085456C = { 0.0f, 100.0f, 40.0f };

void Player_SetLookAngle(Player* this, PlayState* play) {
    s16 focusPitchTarget;
    s16 focusPitchAngle;
    f32 floorPosY;
    Vec3f raycastPos;

    if (this->targetActor != NULL) {
        if (Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) {
            Player_LookAtTargetActor(this, 1);
        } else {
            Player_LookAtTargetActor(this, 0);
        }
        return;
    }

    if (sFloorType == FLOOR_TYPE_LOOK_UP) {
        Math_SmoothStepToS(&this->actor.focus.rot.x, -20000, 10, 4000, 800);
    } else {
        focusPitchTarget = 0;
        floorPosY = Player_RaycastFloorWithOffset2(play, this, &D_8085456C, &raycastPos);
        if (floorPosY > BGCHECK_Y_MIN) {
            focusPitchAngle = Math_Atan2S(40.0f, this->actor.world.pos.y - floorPosY);
            focusPitchTarget = CLAMP(focusPitchAngle, -4000, 4000);
        }
        this->actor.focus.rot.y = this->actor.shape.rot.y;
        Math_SmoothStepToS(&this->actor.focus.rot.x, focusPitchTarget, 14, 4000, 30);
    }

    Player_UpdateLookAngles(this, Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this));
}

void func_8083DDC8(Player* this, PlayState* play) {
    s16 targetPitch;
    s16 targetRoll;

    if (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this) &&
        (this->linearVelocity > 5.0f)) {
        targetPitch = this->linearVelocity * 200.0f;
        targetRoll = (s16)(this->currentYaw - this->actor.shape.rot.y) * this->linearVelocity * 0.1f;
        targetPitch = CLAMP(targetPitch, -4000, 4000);
        targetRoll = CLAMP(-targetRoll, -4000, 4000);
        Math_ScaledStepToS(&this->upperBodyRot.x, targetPitch, 900);
        this->headRot.x = -(f32)this->upperBodyRot.x * 0.5f;
        Math_ScaledStepToS(&this->headRot.z, targetRoll, 300);
        Math_ScaledStepToS(&this->upperBodyRot.z, targetRoll, 200);
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Z | PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X |
                           PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Z | PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_X;
    } else {
        Player_SetLookAngle(this, play);
    }
}

void Player_SetRunVelocityAndYaw(Player* this, f32 targetVelocity, s16 targetYaw) {
    Math_AsymStepToF(&this->linearVelocity, targetVelocity, REG(19) / 100.0f, 1.5f);
    Math_ScaledStepToS(&this->currentYaw, targetYaw, REG(27));
}

void func_8083DFE0(Player* this, f32* targetVelocity, s16* targetYaw) {
    s16 yawDiff = this->currentYaw - *targetYaw;

    if (this->isMeleeWeaponAttacking == false) {
        this->linearVelocity = CLAMP(this->linearVelocity, -(R_RUN_SPEED_LIMIT / 100.0f), (R_RUN_SPEED_LIMIT / 100.0f));
    }

    if (ABS(yawDiff) > DEG_TO_BINANG(135.0f)) {
        if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
            this->currentYaw = *targetYaw;
        }
    } else {
        Math_AsymStepToF(&this->linearVelocity, *targetVelocity, 0.05f, 0.1f);
        Math_ScaledStepToS(&this->currentYaw, *targetYaw, 200);
    }
}

static HorseMountAnimInfo sMountHorseAnims[] = {
    { &gPlayerAnim_link_uma_left_up, 35.17f, 6.6099997f },
    { &gPlayerAnim_link_uma_right_up, -34.16f, 7.91f },
};

s32 Player_SetupMountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 riderOffsetX;
    f32 riderOffsetZ;
    f32 cosYaw;
    f32 cosSin;
    s32 mountedLeftOfHorse;

    if ((rideActor != NULL) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        cosYaw = Math_CosS(rideActor->actor.shape.rot.y);
        cosSin = Math_SinS(rideActor->actor.shape.rot.y);

        Player_SetupMiniCsFunc(play, this, Player_SetupRideHorse);

        this->stateFlags1 |= PLAYER_STATE1_RIDING_HORSE;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_WATER;

        if (this->mountSide < 0) {
            mountedLeftOfHorse = false;
        } else {
            mountedLeftOfHorse = true;
        }

        riderOffsetX = sMountHorseAnims[mountedLeftOfHorse].riderOffsetX;
        riderOffsetZ = sMountHorseAnims[mountedLeftOfHorse].riderOffsetZ;
        this->actor.world.pos.x =
            rideActor->actor.world.pos.x + rideActor->riderPos.x + ((riderOffsetX * cosYaw) + (riderOffsetZ * cosSin));
        this->actor.world.pos.z =
            rideActor->actor.world.pos.z + rideActor->riderPos.z + ((riderOffsetZ * cosYaw) - (riderOffsetX * cosSin));

        this->rideOffsetY = rideActor->actor.world.pos.y - this->actor.world.pos.y;
        this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

        Actor_MountHorse(play, this, &rideActor->actor);
        Player_PlayAnimOnce(play, this, sMountHorseAnims[mountedLeftOfHorse].anim);
        Player_SetupAnimMovement(play, this,
                                 PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                     PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                     PLAYER_ANIMMOVEFLAGS_7);
        this->actor.parent = this->rideActor;
        Player_ClearAttentionModeAndStopMoving(this);
        Actor_DisableLens(play);
        return 1;
    }

    return 0;
}

void Player_GetSlopeDirection(CollisionPoly* floorPoly, Vec3f* slopeNormal, s16* downwardSlopeYaw) {
    slopeNormal->x = COLPOLY_GET_NORMAL(floorPoly->normal.x);
    slopeNormal->y = COLPOLY_GET_NORMAL(floorPoly->normal.y);
    slopeNormal->z = COLPOLY_GET_NORMAL(floorPoly->normal.z);

    *downwardSlopeYaw = Math_Atan2S(slopeNormal->z, slopeNormal->x);
}

static LinkAnimationHeader* sSlopeSlipAnims[] = {
    &gPlayerAnim_link_normal_down_slope_slip,
    &gPlayerAnim_link_normal_up_slope_slip,
};

s32 Player_WalkOnSlope(PlayState* play, Player* this, CollisionPoly* floorPoly) {
    s32 pad;
    s16 playerVelYaw;
    Vec3f slopeNormal;
    s16 downwardSlopeYaw;
    f32 slopeSlowdownSpeed;
    f32 slopeSlowdownSpeedStep;
    s16 velYawToDownwardSlope;

    if (!Player_InBlockingCsMode(play, this) && (Player_SlipOnSlope != this->actionFunc) &&
        (SurfaceType_GetFloorEffect(&play->colCtx, floorPoly, this->actor.floorBgId) == FLOOR_EFFECT_1)) {

        // Get direction of movement relative to the downward direction of the slope
        playerVelYaw = Math_Atan2S(this->actor.velocity.z, this->actor.velocity.x);
        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);
        velYawToDownwardSlope = downwardSlopeYaw - playerVelYaw;

        if (ABS(velYawToDownwardSlope) > 0x3E80) { // 87.9 degrees
            // moving parallel or upwards on the slope, player does not slip but does slow down
            slopeSlowdownSpeed = (1.0f - slopeNormal.y) * 40.0f;
            slopeSlowdownSpeedStep = SQ(slopeSlowdownSpeed) * 0.015f;
            if (slopeSlowdownSpeedStep < 1.2f) {
                slopeSlowdownSpeedStep = 1.2f;
            }

            // slows down speed as player is climbing a slope
            this->pushedYaw = downwardSlopeYaw;
            Math_StepToF(&this->pushedSpeed, slopeSlowdownSpeed, slopeSlowdownSpeedStep);
        } else {
            // moving downward on the slope, causing player to slip
            Player_SetActionFunc(play, this, Player_SlipOnSlope, 0);
            Player_ResetAttributesAndHeldActor(play, this);
            if (sAngleToFloorX >= 0) {
                this->genericVar = 1;
            }
            Player_ChangeAnimShortMorphLoop(play, this, sSlopeSlipAnims[this->genericVar]);
            this->linearVelocity = sqrtf(SQ(this->actor.velocity.x) + SQ(this->actor.velocity.z));
            this->currentYaw = playerVelYaw;
            return true;
        }
    }

    return false;
}

// unknown data (unused), possibly four individual arrays separated by padding
static s16 D_80854598[] = {
    0xFFDB, 0x0871, 0xF831,         // Vec3s?
    0x0000, 0x0094, 0x0470, 0xF398, // Vec3s?
    0x0000, 0xFFB5, 0x04A9, 0x0C9F, // Vec3s?
    0x0000, 0x0801, 0x0402,
};

void Player_PickupItemDrop(PlayState* play, Player* this, GetItemEntry* giEntry) {
    s32 dropType = giEntry->field & 0x1F;

    if (!(giEntry->field & 0x80)) {
        Item_DropCollectible(play, &this->actor.world.pos, dropType | 0x8000);
        if ((dropType != ITEM00_BOMBS_A) && (dropType != ITEM00_ARROWS_SMALL) && (dropType != ITEM00_ARROWS_MEDIUM) &&
            (dropType != ITEM00_ARROWS_LARGE) && (dropType != ITEM00_RUPEE_GREEN) && (dropType != ITEM00_RUPEE_BLUE) &&
            (dropType != ITEM00_RUPEE_RED) && (dropType != ITEM00_RUPEE_PURPLE) && (dropType != ITEM00_RUPEE_ORANGE)) {
            Item_Give(play, giEntry->itemId);
        }
    } else {
        Item_Give(play, giEntry->itemId);
    }

    func_80078884((this->getItemId < 0) ? NA_SE_SY_GET_BOXITEM : NA_SE_SY_GET_ITEM);
}

s32 Player_SetupGetItemOrHoldBehavior(Player* this, PlayState* play) {
    Actor* interactedActor;

    if (iREG(67) ||
        (((interactedActor = this->interactRangeActor) != NULL) && TitleCard_Clear(play, &play->actorCtx.titleCtx))) {
        if (iREG(67) || (this->getItemId > GI_NONE)) {
            if (iREG(67)) {
                this->getItemId = iREG(68);
            }

            if (this->getItemId < GI_MAX) {
                GetItemEntry* giEntry = &sGetItemTable[this->getItemId - 1];

                if ((interactedActor != &this->actor) && !iREG(67)) {
                    interactedActor->parent = &this->actor;
                }

                iREG(67) = false;

                if ((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) ||
                    (play->sceneId == SCENE_BOMBCHU_BOWLING_ALLEY)) {
                    Player_DetatchHeldActor(play, this);
                    Player_LoadGetItemObject(this, giEntry->objectId);

                    if (!(this->stateFlags2 & PLAYER_STATE2_DIVING) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                        Player_SetupMiniCsFunc(play, this, Player_SetupGetItem);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_get_itemB);
                        Player_SetCameraTurnAround(play, 9);
                    }

                    this->stateFlags1 |=
                        PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                    Player_ClearAttentionModeAndStopMoving(this);
                    return 1;
                }

                Player_PickupItemDrop(play, this, giEntry);
                this->getItemId = GI_NONE;
            }
        } else if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
                   !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
            if (this->getItemId != GI_NONE) {
                GetItemEntry* giEntry = &sGetItemTable[-this->getItemId - 1];
                EnBox* chest = (EnBox*)interactedActor;

                if (giEntry->itemId != ITEM_NONE) {
                    if (((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) && (giEntry->field & 0x40)) ||
                        ((Item_CheckObtainability(giEntry->itemId) != ITEM_NONE) && (giEntry->field & 0x20))) {
                        this->getItemId = -GI_RUPEE_BLUE;
                        giEntry = &sGetItemTable[GI_RUPEE_BLUE - 1];
                    }
                }

                Player_SetupMiniCsFunc(play, this, Player_SetupGetItem);
                this->stateFlags1 |=
                    PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                Player_LoadGetItemObject(this, giEntry->objectId);
                this->actor.world.pos.x =
                    chest->dyna.actor.world.pos.x - (Math_SinS(chest->dyna.actor.shape.rot.y) * 29.4343f);
                this->actor.world.pos.z =
                    chest->dyna.actor.world.pos.z - (Math_CosS(chest->dyna.actor.shape.rot.y) * 29.4343f);
                this->currentYaw = this->actor.shape.rot.y = chest->dyna.actor.shape.rot.y;
                Player_ClearAttentionModeAndStopMoving(this);

                if ((giEntry->itemId != ITEM_NONE) && (giEntry->gi >= 0) &&
                    (Item_CheckObtainability(giEntry->itemId) == ITEM_NONE)) {
                    Player_PlayAnimOnceSlowed(play, this, this->ageProperties->chestOpenAnim);
                    Player_SetupAnimMovement(play, this,
                                             PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                                 PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                                 PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_7 |
                                                 PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
                    chest->unk_1F4 = 1;
                    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_SLOW_CHEST_CS);
                } else {
                    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_box_kick);
                    chest->unk_1F4 = -1;
                }

                return 1;
            }

            if ((this->heldActor == NULL) || Player_HoldsHookshot(this)) {
                if ((interactedActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                    s32 itemAction = this->itemAction;

                    this->itemAction = PLAYER_IA_NONE;
                    this->modelAnimType = PLAYER_ANIMTYPE_DEFAULT;
                    this->heldItemAction = this->itemAction;
                    Player_SetupMiniCsFunc(play, this, Player_SetupHoldActor);

                    if (itemAction == PLAYER_IA_SWORD_MASTER) {
                        this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_IA_LAST_USED);
                        Player_ChangeItem(play, this, PLAYER_IA_LAST_USED);
                    } else {
                        Player_UseItem(play, this, ITEM_LAST_USED);
                    }
                } else {
                    s32 strength = Player_GetStrength();

                    if ((interactedActor->id == ACTOR_EN_ISHI) && ((interactedActor->params & 0xF) == 1) &&
                        (strength < PLAYER_STR_SILVER_G)) {
                        return 0;
                    }

                    Player_SetupMiniCsFunc(play, this, Player_SetupHoldActor);
                }

                Player_ClearAttentionModeAndStopMoving(this);
                this->stateFlags1 |= PLAYER_STATE1_HOLDING_ACTOR;
                return 1;
            }
        }
    }

    return 0;
}

void Player_SetupStartThrowActor(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_StartThrowActor, 1);
    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_throw, this->modelAnimType));
}

s32 Player_CanThrowActor(Player* this, Actor* actor) {
    if ((actor != NULL) && !(actor->flags & ACTOR_FLAG_23) &&
        ((this->linearVelocity < 1.1f) || (actor->id == ACTOR_EN_BOM_CHU))) {
        return false;
    }

    return true;
}

s32 Player_SetupPutDownOrThrowActor(Player* this, PlayState* play) {
    if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor != NULL) &&
        CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
        if (!Player_InterruptHoldingActor(play, this, this->heldActor)) {
            if (!Player_CanThrowActor(this, this->heldActor)) {
                Player_SetActionFunc(play, this, Player_SetupPutDownActor, 1);
                Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_put, this->modelAnimType));
            } else {
                Player_SetupStartThrowActor(this, play);
            }
        }
        return 1;
    }

    return 0;
}

s32 Player_SetupClimbWallOrLadder(Player* this, PlayState* play, u32 wallFlags) {
    if (this->wallHeight >= 79.0f) {
        if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->currentBoots == PLAYER_BOOTS_IRON) ||
            (this->actor.yDistToWater < this->ageProperties->diveWaterSurface)) {
            s32 isClimbableWall = (wallFlags & 8) ? 2 : 0;

            if ((isClimbableWall != 0) || (wallFlags & 2) ||
                SurfaceType_CheckWallFlag2(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) {
                f32 yOffset;
                CollisionPoly* wallPoly = this->actor.wallPoly;
                f32 xOffset;
                f32 zOffset;
                f32 xOffset2;
                f32 zOffset2;

                yOffset = xOffset2 = 0.0f;

                if (isClimbableWall != 0) {
                    xOffset = this->actor.world.pos.x;
                    zOffset = this->actor.world.pos.z;
                } else {
                    Vec3f wallVertices[3];
                    s32 i;
                    f32 yOffsetDiff;
                    Vec3f* testVtx = &wallVertices[0];
                    s32 pad;

                    CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, wallVertices);

                    xOffset = xOffset2 = testVtx->x;
                    zOffset = zOffset2 = testVtx->z;
                    yOffset = testVtx->y;
                    for (i = 1; i < 3; i++) {
                        testVtx++;
                        if (xOffset > testVtx->x) {
                            xOffset = testVtx->x;
                        } else if (xOffset2 < testVtx->x) {
                            xOffset2 = testVtx->x;
                        }

                        if (zOffset > testVtx->z) {
                            zOffset = testVtx->z;
                        } else if (zOffset2 < testVtx->z) {
                            zOffset2 = testVtx->z;
                        }

                        if (yOffset > testVtx->y) {
                            yOffset = testVtx->y;
                        }
                    }

                    xOffset = (xOffset + xOffset2) * 0.5f;
                    zOffset = (zOffset + zOffset2) * 0.5f;

                    xOffset2 = ((this->actor.world.pos.x - xOffset) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
                               ((this->actor.world.pos.z - zOffset) * COLPOLY_GET_NORMAL(wallPoly->normal.x));
                    yOffsetDiff = this->actor.world.pos.y - yOffset;

                    yOffset = ((f32)(s32)((yOffsetDiff / 15.000000223517418) + 0.5) * 15.000000223517418) - yOffsetDiff;
                    xOffset2 = fabsf(xOffset2);
                }

                if (xOffset2 < 8.0f) {
                    f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                    f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                    f32 dist = this->wallDistance;
                    LinkAnimationHeader* climbAnim;

                    Player_SetupMiniCsFunc(play, this, Player_SetupClimbingWallOrDownLedge);
                    this->stateFlags1 |= PLAYER_STATE1_CLIMBING;
                    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;

                    if ((isClimbableWall != 0) || (wallFlags & 2)) {
                        if ((this->genericVar = isClimbableWall) != 0) {
                            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                                climbAnim = &gPlayerAnim_link_normal_Fclimb_startA;
                            } else {
                                climbAnim = &gPlayerAnim_link_normal_Fclimb_hold2upL;
                            }
                            dist = (this->ageProperties->wallCheckRadius - 1.0f) - dist;
                        } else {
                            climbAnim = this->ageProperties->startClimb1Anim;
                            dist = dist - 1.0f;
                        }
                        this->genericTimer = -2;
                        this->actor.world.pos.y += yOffset;
                        this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
                    } else {
                        climbAnim = this->ageProperties->startClimb2Anim;
                        this->genericTimer = -4;
                        this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw;
                    }

                    this->actor.world.pos.x = (dist * wallPolyNormalX) + xOffset;
                    this->actor.world.pos.z = (dist * wallPolyNormalZ) + zOffset;
                    Player_ClearAttentionModeAndStopMoving(this);
                    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.world.pos);
                    Player_PlayAnimOnce(play, this, climbAnim);
                    Player_SetupAnimMovement(play, this,
                                             PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                                 PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                                 PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                                 PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);

                    return true;
                }
            }
        }
    }

    return false;
}

void Player_SetupEndClimb(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetActionFuncPreserveMoveFlags(play, this, Player_EndClimb, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, (4.0f / 3.0f));
}

/**
 * @return true if Player chooses to enter crawlspace
 */
s32 Player_TryEnteringCrawlspace(Player* this, PlayState* play, u32 interactWallFlags) {
    CollisionPoly* wallPoly;
    Vec3f wallVertices[3];
    f32 xVertex1;
    f32 xVertex2;
    f32 zVertex1;
    f32 zVertex2;
    s32 i;

    if (!LINK_IS_ADULT && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) && (interactWallFlags & WALL_FLAG_CRAWLSPACE)) {
        wallPoly = this->actor.wallPoly;
        CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, wallVertices);

        // Determines min and max vertices for x & z (edges of the crawlspace hole)
        xVertex1 = xVertex2 = wallVertices[0].x;
        zVertex1 = zVertex2 = wallVertices[0].z;
        for (i = 1; i < 3; i++) {
            if (xVertex1 > wallVertices[i].x) {
                // Update x min
                xVertex1 = wallVertices[i].x;
            } else if (xVertex2 < wallVertices[i].x) {
                // Update x max
                xVertex2 = wallVertices[i].x;
            }
            if (zVertex1 > wallVertices[i].z) {
                // Update z min
                zVertex1 = wallVertices[i].z;
            } else if (zVertex2 < wallVertices[i].z) {
                // Update z max
                zVertex2 = wallVertices[i].z;
            }
        }

        // XZ Center of the crawlspace hole
        xVertex1 = (xVertex1 + xVertex2) * 0.5f;
        zVertex1 = (zVertex1 + zVertex2) * 0.5f;

        // Perpendicular (sideways) XZ-Distance from player pos to crawlspace line
        // Uses y-component of crossproduct formula for the distance from a point to a line
        xVertex2 = ((this->actor.world.pos.x - xVertex1) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
                   ((this->actor.world.pos.z - zVertex1) * COLPOLY_GET_NORMAL(wallPoly->normal.x));

        if (fabsf(xVertex2) < 8.0f) {
            // Give do-action prompt to "Enter on A" for the crawlspace
            this->stateFlags2 |= PLAYER_STATE2_DO_ACTION_ENTER;

            if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                // Enter Crawlspace
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 wallDistance = this->wallDistance;

                Player_SetupMiniCsFunc(play, this, Player_SetupInsideCrawlspace);
                this->stateFlags2 |= PLAYER_STATE2_CRAWLING;
                this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
                this->actor.world.pos.x = xVertex1 + (wallDistance * wallPolyNormalX);
                this->actor.world.pos.z = zVertex1 + (wallDistance * wallPolyNormalZ);
                Player_ClearAttentionModeAndStopMoving(this);
                this->actor.prevPos = this->actor.world.pos;
                Player_PlayAnimOnce(play, this, &gPlayerAnim_link_child_tunnel_start);
                Player_SetupAnimMovement(play, this,
                                         PLAYER_ANIMMOVEFLAGS_UPDATE_XZ |
                                             PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                             PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                             PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);

                return true;
            }
        }
    }

    return false;
}

s32 Player_SetPositionAndYawOnClimbWall(PlayState* play, Player* this, f32 yOffset, f32 xzDistToWall, f32 xzCheckBScale,
                                        f32 xzCheckAScale) {
    CollisionPoly* wallPoly;
    s32 wallBgId;
    Vec3f checkPosA;
    Vec3f checkPosB;
    Vec3f posResult;
    f32 yawCos;
    f32 yawSin;
    s32 yawTarget;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;

    yawCos = Math_CosS(this->actor.shape.rot.y);
    yawSin = Math_SinS(this->actor.shape.rot.y);

    checkPosA.x = this->actor.world.pos.x + (xzCheckAScale * yawSin);
    checkPosA.z = this->actor.world.pos.z + (xzCheckAScale * yawCos);
    checkPosB.x = this->actor.world.pos.x + (xzCheckBScale * yawSin);
    checkPosB.z = this->actor.world.pos.z + (xzCheckBScale * yawCos);
    checkPosB.y = checkPosA.y = this->actor.world.pos.y + yOffset;

    if (BgCheck_EntityLineTest1(&play->colCtx, &checkPosA, &checkPosB, &posResult, &this->actor.wallPoly, true, false,
                                false, true, &wallBgId)) {

        wallPoly = this->actor.wallPoly;

        this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
        this->actor.wallBgId = wallBgId;

        sInteractWallFlags = SurfaceType_GetWallFlags(&play->colCtx, wallPoly, wallBgId);

        wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
        wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
        yawTarget = Math_Atan2S(-wallPolyNormalZ, -wallPolyNormalX);
        Math_ScaledStepToS(&this->actor.shape.rot.y, yawTarget, 800);

        this->currentYaw = this->actor.shape.rot.y;
        this->actor.world.pos.x = posResult.x - (Math_SinS(this->actor.shape.rot.y) * xzDistToWall);
        this->actor.world.pos.z = posResult.z - (Math_CosS(this->actor.shape.rot.y) * xzDistToWall);

        return 1;
    }

    this->actor.bgCheckFlags &= ~BGCHECKFLAG_PLAYER_WALL_INTERACT;

    return 0;
}

s32 Player_PushPullSetPositionAndYaw(PlayState* play, Player* this) {
    return Player_SetPositionAndYawOnClimbWall(play, this, 26.0f, this->ageProperties->wallCheckRadius + 5.0f, 30.0f,
                                               0.0f);
}

/**
 * Two exit walls are placed at each end of the crawlspace, separate to the two entrance walls used to enter the
 * crawlspace. These front and back exit walls are futher into the crawlspace than the front and
 * back entrance walls. When player interacts with either of these two interior exit walls, start the
 * leaving-crawlspace cutscene and return true. Else, return false
 */
s32 Player_TryLeavingCrawlspace(Player* this, PlayState* play) {
    s16 yawToWall;

    if ((this->linearVelocity != 0.0f) && (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) &&
        (sInteractWallFlags & WALL_FLAG_CRAWLSPACE)) {

        // The exit wallYaws will always point inward on the crawlline
        // Interacting with the exit wall in front will have a yaw diff of 0x8000
        // Interacting with the exit wall behind will have a yaw diff of 0
        yawToWall = this->actor.shape.rot.y - this->actor.wallYaw;
        if (this->linearVelocity < 0.0f) {
            yawToWall += 0x8000;
        }

        if (ABS(yawToWall) > 0x4000) {
            Player_SetActionFunc(play, this, Player_ExitCrawlspace, 0);

            if (this->linearVelocity > 0.0f) {
                // Leaving a crawlspace forwards
                this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
                Player_PlayAnimOnce(play, this, &gPlayerAnim_link_child_tunnel_end);
                Player_SetupAnimMovement(play, this,
                                         PLAYER_ANIMMOVEFLAGS_UPDATE_XZ |
                                             PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                             PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                             PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
                OnePointCutscene_Init(play, 9601, 999, NULL, CAM_ID_MAIN);
            } else {
                // Leaving a crawlspace backwards
                this->actor.shape.rot.y = this->actor.wallYaw;
                LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_child_tunnel_start, -1.0f,
                                     Animation_GetLastFrame(&gPlayerAnim_link_child_tunnel_start), 0.0f, ANIMMODE_ONCE,
                                     0.0f);
                Player_SetupAnimMovement(play, this,
                                         PLAYER_ANIMMOVEFLAGS_UPDATE_XZ |
                                             PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                             PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                             PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
                OnePointCutscene_Init(play, 9602, 999, NULL, CAM_ID_MAIN);
            }

            this->currentYaw = this->actor.shape.rot.y;
            Player_StopMovement(this);

            return true;
        }
    }

    return false;
}

void Player_StartGrabPushPullWall(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    if (!Player_SetupMiniCsFunc(play, this, Player_SetupGrabPushPullWall)) {
        Player_SetActionFunc(play, this, Player_GrabPushPullWall, 0);
    }

    Player_PlayAnimOnce(play, this, anim);
    Player_ClearAttentionModeAndStopMoving(this);

    this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
}

// Used for ladders, crawlspaces, and push/pull walls
s32 Player_SetupSpecialWallInteraction(Player* this, PlayState* play) {
    DynaPolyActor* wallPolyActor;

    if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sYawToTouchedWall < DEG_TO_BINANG(67.5f))) {

        if (((this->linearVelocity > 0.0f) && Player_SetupClimbWallOrLadder(this, play, sInteractWallFlags)) ||
            Player_TryEnteringCrawlspace(this, play, sInteractWallFlags)) {
            return 1;
        }

        if (!Player_IsSwimming(this) &&
            ((this->linearVelocity == 0.0f) || !(this->stateFlags2 & PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL)) &&
            (sInteractWallFlags & 0x40) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
            (this->wallHeight >= 39.0f)) {

            this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL;

            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {

                if ((this->actor.wallBgId != BGCHECK_SCENE) &&
                    ((wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId)) != NULL)) {

                    if (wallPolyActor->actor.id == ACTOR_BG_HEAVY_BLOCK) {
                        if (Player_GetStrength() < PLAYER_STR_GOLD_G) {
                            return 0;
                        }

                        Player_SetupMiniCsFunc(play, this, Player_SetupHoldActor);
                        this->stateFlags1 |= PLAYER_STATE1_HOLDING_ACTOR;
                        this->interactRangeActor = &wallPolyActor->actor;
                        this->getItemId = GI_NONE;
                        this->currentYaw = this->actor.wallYaw + 0x8000;
                        Player_ClearAttentionModeAndStopMoving(this);

                        return 1;
                    }

                    this->pushPullActor = &wallPolyActor->actor;
                } else {
                    this->pushPullActor = NULL;
                }

                Player_StartGrabPushPullWall(this, &gPlayerAnim_link_normal_push_wait, play);

                return 1;
            }
        }
    }

    return 0;
}

s32 Player_SetupPushPullWallIdle(PlayState* play, Player* this) {
    if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) ||
         CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))) {
        DynaPolyActor* wallPolyActor = NULL;

        if (this->actor.wallBgId != BGCHECK_SCENE) {
            wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
        }

        if (&wallPolyActor->actor == this->pushPullActor) {
            if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    Player_ReturnToStandStill(this, play);
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_push_wait_end);
    this->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    return 1;
}

void Player_SetupPushWall(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_PushWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_push_start);
}

void Player_SetupPullWall(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_PullWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_pull_start, this->modelAnimType));
}

void Player_ClimbingLetGo(Player* this, PlayState* play) {
    this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING | PLAYER_STATE1_SWIMMING);
    Player_SetupFallFromLedge(this, play);
    this->linearVelocity = -0.4f;
}

s32 Player_SetupClimbingLetGo(Player* this, PlayState* play) {
    if (!CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((sInteractWallFlags & 8) || (sInteractWallFlags & 2) ||
         SurfaceType_CheckWallFlag2(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId))) {
        return 0;
    }

    Player_ClimbingLetGo(this, play);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
    return 1;
}

s32 Player_GetUnfriendlyZTargetMoveDirection(Player* this, f32 targetVelocity, s16 targetYaw) {
    f32 targetYawDiff = (s16)(targetYaw - this->actor.shape.rot.y);
    f32 yawRatio;

    if (this->targetActor != NULL) {
        Player_LookAtTargetActor(this, Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this));
    }

    // Divide by 180 degrees
    yawRatio = fabsf(targetYawDiff) / 32768.0f;

    if (targetVelocity > (((yawRatio * yawRatio) * 50.0f) + 6.0f)) {
        return 1;
    } else if (targetVelocity > (((1.0f - yawRatio) * 10.0f) + 6.8f)) {
        return -1;
    }

    return 0;
}

// Updates focus and look angles, then returns direction to move in?
s32 Player_GetFriendlyZTargetingMoveDirection(Player* this, f32* targetVelocity, s16* targetYaw, PlayState* play) {
    s16 sp2E = *targetYaw - this->targetYaw;
    u16 sp2C = ABS(sp2E);

    if ((Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) &&
        (this->targetActor == NULL)) {
        *targetVelocity *= Math_SinS(sp2C);

        if (*targetVelocity != 0.0f) {
            *targetYaw = (((sp2E >= 0) ? 1 : -1) << 0xE) + this->actor.shape.rot.y;
        } else {
            *targetYaw = this->actor.shape.rot.y;
        }

        if (this->targetActor != NULL) {
            Player_LookAtTargetActor(this, 1);
        } else {
            Math_SmoothStepToS(&this->actor.focus.rot.x, sControlInput->rel.stick_y * 240.0f, 14, 4000, 30);
            Player_UpdateLookAngles(this, true);
        }
    } else {
        if (this->targetActor != NULL) {
            return Player_GetUnfriendlyZTargetMoveDirection(this, *targetVelocity, *targetYaw);
        } else {
            Player_SetLookAngle(this, play);
            if ((*targetVelocity != 0.0f) && (sp2C < 6000)) {
                return 1;
            } else if (*targetVelocity > Math_SinS((0x4000 - (sp2C >> 1))) * 200.0f) {
                return -1;
            }
        }
    }

    return 0;
}

s32 Player_GetPushPullDirection(Player* this, f32* targetVelocity, s16* targetYaw) {
    s16 yawDiff = *targetYaw - this->actor.shape.rot.y;
    u16 absYawDiff = ABS(yawDiff);
    f32 cos = Math_CosS(absYawDiff);

    *targetVelocity *= cos;

    if (*targetVelocity != 0.0f) {
        if (cos > 0) {
            return 1;
        } else {
            return -1;
        }
    }

    return 0;
}

s32 Player_GetSpinAttackMoveDirection(Player* this, f32* targetVelocity, s16* targetYaw, PlayState* play) {
    Player_SetLookAngle(this, play);

    if ((*targetVelocity != 0.0f) || (ABS(this->unk_87C) > 400)) {
        s16 yawDiff = *targetYaw - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        u16 checkYawDiff = (ABS(yawDiff) - DEG_TO_BINANG(45.0f)) & 0xFFFF;

        if ((checkYawDiff < 0x4000) || (this->unk_87C != 0)) {
            return -1;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_SetLeftRightBlendWeight(Player* this, f32 targetVelocity, s16 targetYaw) {
    s16 yawDiff = targetYaw - this->actor.shape.rot.y;

    if (targetVelocity > 0.0f) {
        if (yawDiff < 0) {
            this->leftRightBlendWeightTarget = 0.0f;
        } else {
            this->leftRightBlendWeightTarget = 1.0f;
        }
    }

    Math_StepToF(&this->leftRightBlendWeight, this->leftRightBlendWeightTarget, 0.3f);
}

void func_808401B0(PlayState* play, Player* this) {
    LinkAnimation_BlendToJoint(play, &this->skelAnime, Player_GetFightingRightAnim(this), this->walkFrame,
                               Player_GetFightingLeftAnim(this), this->walkFrame, this->leftRightBlendWeight,
                               this->blendTable);
}

s32 Player_CheckWalkFrame(f32 curFrame, f32 frameStep, f32 maxFrame, f32 targetFrame) {
    f32 frameDiff;

    if ((targetFrame == 0.0f) && (frameStep > 0.0f)) {
        targetFrame = maxFrame;
    }

    frameDiff = (curFrame + frameStep) - targetFrame;

    if (((frameDiff * frameStep) >= 0.0f) && (((frameDiff - frameStep) * frameStep) < 0.0f)) {
        return 1;
    }

    return 0;
}

void Player_SetupWalkSfx(Player* this, f32 frameStep) {
    f32 updateScale = R_UPDATE_RATE * 0.5f;

    frameStep *= updateScale;
    if (frameStep < -7.25) {
        frameStep = -7.25;
    } else if (frameStep > 7.25f) {
        frameStep = 7.25f;
    }

    if (1) {}

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
        (this->hoverBootsTimer != 0)) {
        func_8002F8F0(&this->actor, NA_SE_PL_HOBBERBOOTS_LV - SFX_FLAG);
    } else if (Player_CheckWalkFrame(this->walkFrame, frameStep, 29.0f, 10.0f) ||
               Player_CheckWalkFrame(this->walkFrame, frameStep, 29.0f, 24.0f)) {
        Player_PlayWalkSfx(this, this->linearVelocity);
        if (this->linearVelocity > 4.0f) {
            this->stateFlags2 |= PLAYER_STATE2_MAKING_REACTABLE_NOISE;
        }
    }

    this->walkFrame += frameStep;

    if (this->walkFrame < 0.0f) {
        this->walkFrame += 29.0f;
    } else if (this->walkFrame >= 29.0f) {
        this->walkFrame -= 29.0f;
    }
}

void Player_UnfriendlyZTargetStandingStill(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    u32 walkFrame;
    s16 targetYawDiff;
    s32 absTargetYawDiff;

    if (this->stateFlags3 & PLAYER_STATE3_ENDING_MELEE_ATTACK) {
        if (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) {
            this->stateFlags2 |=
                PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
        } else {
            this->stateFlags3 &= ~PLAYER_STATE3_ENDING_MELEE_ATTACK;
        }
    }

    if (this->genericTimer != 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_EndAnimMovement(this);
            Player_PlayAnimLoop(play, this, Player_GetFightingRightAnim(this));
            this->genericTimer = 0;
            this->stateFlags3 &= ~PLAYER_STATE3_ENDING_MELEE_ATTACK;
        }
        Player_ResetLeftRightBlendWeight(this);
    } else {
        func_808401B0(play, this);
    }

    Player_StepLinearVelocityToZero(this);

    if (!Player_SetupSubAction(play, this, sTargetEnemyStandStillSubActions, 1)) {
        if (!Player_SetupStartUnfriendlyZTargeting(this) &&
            (!Player_IsFriendlyZTargeting(this) || (Player_StandingDefend != this->upperActionFunc))) {
            Player_SetupEndUnfriendlyZTarget(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        moveDir = Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw);

        if (moveDir > 0) {
            Player_SetupZTargetRunning(this, play, targetYaw);
            return;
        }

        if (moveDir < 0) {
            Player_SetupUnfriendlyBackwalk(this, targetYaw, play);
            return;
        }

        if (targetVelocity > 4.0f) {
            Player_SetupSidewalk(this, play);
            return;
        }

        Player_SetupWalkSfx(this, (this->linearVelocity * 0.3f) + 1.0f);
        Player_SetLeftRightBlendWeight(this, targetVelocity, targetYaw);

        walkFrame = this->walkFrame;
        if ((walkFrame < 6) || ((walkFrame - 0xE) < 6)) {
            Math_StepToF(&this->linearVelocity, 0.0f, 1.5f);
            return;
        }

        targetYawDiff = targetYaw - this->currentYaw;
        absTargetYawDiff = ABS(targetYawDiff);

        if (absTargetYawDiff > DEG_TO_BINANG(90.0f)) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.5f)) {
                this->currentYaw = targetYaw;
            }
            return;
        }

        Math_AsymStepToF(&this->linearVelocity, targetVelocity * 0.3f, 2.0f, 1.5f);

        if (!(this->stateFlags3 & PLAYER_STATE3_ENDING_MELEE_ATTACK)) {
            Math_ScaledStepToS(&this->currentYaw, targetYaw, absTargetYawDiff * 0.1f);
        }
    }
}

void Player_FriendlyZTargetStandingStill(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    s16 targetYawDiff;
    s32 absTargetYawDiff;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
    }

    Player_StepLinearVelocityToZero(this);

    if (!Player_SetupSubAction(play, this, sFriendlyTargetingStandStillSubActions, 1)) {
        if (Player_SetupStartUnfriendlyZTargeting(this)) {
            Player_SetupUnfriendlyZTarget(this, play);
            return;
        }

        if (!Player_IsFriendlyZTargeting(this)) {
            Player_SetActionFuncPreserveMoveFlags(play, this, Player_StandingStill, 1);
            this->currentYaw = this->actor.shape.rot.y;
            return;
        }

        if (Player_StandingDefend == this->upperActionFunc) {
            Player_SetupUnfriendlyZTarget(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        moveDir = Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play);

        if (moveDir > 0) {
            Player_SetupZTargetRunning(this, play, targetYaw);
            return;
        }

        if (moveDir < 0) {
            Player_SetupFriendlyBackwalk(this, targetYaw, play);
            return;
        }

        if (targetVelocity > 4.9f) {
            Player_SetupSidewalk(this, play);
            Player_ResetLeftRightBlendWeight(this);
            return;
        }
        if (targetVelocity != 0.0f) {
            Player_SetupFriendlySidewalk(this, play);
            return;
        }

        targetYawDiff = targetYaw - this->actor.shape.rot.y;
        absTargetYawDiff = ABS(targetYawDiff);

        if (absTargetYawDiff > 800) {
            Player_SetupTurn(play, this, targetYaw);
        }
    }
}

void Player_SetupIdleAnim(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    LinkAnimationHeader** animPtr;
    s32 healthIsCritical;
    s32 idleAnimType;
    s32 rand;

    if ((this->targetActor != NULL) ||
        (!(healthIsCritical = Health_IsCritical()) && ((this->idleCounter = (this->idleCounter + 1) & 1) != 0))) {
        this->stateFlags2 &= ~PLAYER_STATE2_IDLING;
        anim = Player_GetStandingStillAnim(this);
    } else {
        this->stateFlags2 |= PLAYER_STATE2_IDLING;
        if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
            anim = Player_GetStandingStillAnim(this);
        } else {
            idleAnimType = play->roomCtx.curRoom.behaviorType2;
            if (healthIsCritical) {
                if (this->idleCounter >= 0) {
                    idleAnimType = 7;
                    this->idleCounter = -1;
                } else {
                    idleAnimType = 8;
                }
            } else {
                rand = Rand_ZeroOne() * 5.0f;
                if (rand < 4) {
                    if (((rand != 0) && (rand != 3)) ||
                        ((this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) &&
                         ((rand == 3) || (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE)))) {
                        if ((rand == 0) && Player_HoldsTwoHandedWeapon(this)) {
                            rand = 4;
                        }
                        idleAnimType = rand + 9;
                    }
                }
            }
            animPtr = &sIdleAnims[idleAnimType][0];
            if (this->modelAnimType != PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON) {
                animPtr = &sIdleAnims[idleAnimType][1];
            }
            anim = *animPtr;
        }
    }

    LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f) * sWaterSpeedScale, 0.0f,
                         Animation_GetLastFrame(anim), ANIMMODE_ONCE, -6.0f);
}

void Player_StandingStill(Player* this, PlayState* play) {
    s32 idleAnimOffset;
    s32 animDone;
    f32 targetVelocity;
    s16 targetYaw;
    s16 targetYawDiff;

    idleAnimOffset = Player_IsPlayingIdleAnim(this);
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (idleAnimOffset > 0) {
        Player_PlayIdleAnimSfx(this, idleAnimOffset - 1);
    }

    if (animDone != false) {
        if (this->genericTimer != 0) {
            if (DECR(this->genericTimer) == 0) {
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
            this->skelAnime.jointTable[0].y = (this->skelAnime.jointTable[0].y + ((this->genericTimer & 1) * 80)) - 40;
        } else {
            Player_EndAnimMovement(this);
            Player_SetupIdleAnim(play, this);
        }
    }

    Player_StepLinearVelocityToZero(this);

    if (this->genericTimer == 0) {
        if (!Player_SetupSubAction(play, this, sStandStillSubActions, 1)) {
            if (Player_SetupStartUnfriendlyZTargeting(this)) {
                Player_SetupUnfriendlyZTarget(this, play);
                return;
            }

            if (Player_IsFriendlyZTargeting(this)) {
                Player_SetupFriendlyZTargetingStandStill(this, play);
                return;
            }

            Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

            if (targetVelocity != 0.0f) {
                Player_SetupZTargetRunning(this, play, targetYaw);
                return;
            }

            targetYawDiff = targetYaw - this->actor.shape.rot.y;
            if (ABS(targetYawDiff) > 800) {
                Player_SetupTurn(play, this, targetYaw);
                return;
            }

            Math_ScaledStepToS(&this->actor.shape.rot.y, targetYaw, 1200);
            this->currentYaw = this->actor.shape.rot.y;
            if (Player_GetStandingStillAnim(this) == this->skelAnime.animation) {
                Player_SetLookAngle(this, play);
            }
        }
    }
}

void Player_EndSidewalk(Player* this, PlayState* play) {
    f32 frames;
    f32 coeff;
    f32 targetVelocity;
    s16 targetYaw;
    s32 temp1;
    s16 targetYawDiff;
    s32 absTargetYawDiff;
    s32 direction;

    this->skelAnime.mode = 0;
    LinkAnimation_SetUpdateFunction(&this->skelAnime);

    this->skelAnime.animation = Player_GetEndSidewalkAnim(this);

    if (this->skelAnime.animation == &gPlayerAnim_link_bow_side_walk) {
        frames = 24.0f;
        coeff = -(MREG(95) / 100.0f);
    } else {
        frames = 29.0f;
        coeff = MREG(95) / 100.0f;
    }

    this->skelAnime.animLength = frames;
    this->skelAnime.endFrame = frames - 1.0f;

    if ((s16)(this->currentYaw - this->actor.shape.rot.y) >= 0) {
        direction = 1;
    } else {
        direction = -1;
    }

    this->skelAnime.playSpeed = direction * (this->linearVelocity * coeff);

    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 0.0f) || LinkAnimation_OnFrame(&this->skelAnime, frames * 0.5f)) {
        Player_PlayWalkSfx(this, this->linearVelocity);
    }

    if (!Player_SetupSubAction(play, this, sEndSidewalkSubActions, 1)) {
        if (Player_SetupStartUnfriendlyZTargeting(this)) {
            Player_SetupUnfriendlyZTarget(this, play);
            return;
        }

        if (!Player_IsFriendlyZTargeting(this)) {
            Player_SetupStandingStillMorph(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        temp1 = Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play);

        if (temp1 > 0) {
            Player_SetupZTargetRunning(this, play, targetYaw);
            return;
        }

        if (temp1 < 0) {
            Player_SetupFriendlyBackwalk(this, targetYaw, play);
            return;
        }

        if (targetVelocity > 4.9f) {
            Player_SetupSidewalk(this, play);
            Player_ResetLeftRightBlendWeight(this);
            return;
        }

        if ((targetVelocity == 0.0f) && (this->linearVelocity == 0.0f)) {
            Player_SetupFriendlyZTargetingStandStill(this, play);
            return;
        }

        targetYawDiff = targetYaw - this->currentYaw;
        absTargetYawDiff = ABS(targetYawDiff);

        if (absTargetYawDiff > DEG_TO_BINANG(90.0f)) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.5f)) {
                this->currentYaw = targetYaw;
            }
            return;
        }

        Math_AsymStepToF(&this->linearVelocity, targetVelocity * 0.4f, 1.5f, 1.5f);
        Math_ScaledStepToS(&this->currentYaw, targetYaw, absTargetYawDiff * 0.1f);
    }
}

void Player_UpdateBackwalk(Player* this, PlayState* play) {
    f32 morphWeight;
    f32 velocity;

    if (this->unk_864 < 1.0f) {
        morphWeight = R_UPDATE_RATE * 0.5f;
        Player_SetupWalkSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(play, &this->skelAnime,
                                  GET_PLAYER_ANIM(PLAYER_ANIMGROUP_back_walk, this->modelAnimType), this->walkFrame);
        this->unk_864 += 1 * morphWeight;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }
        morphWeight = this->unk_864;
    } else {
        velocity = this->linearVelocity - (REG(48) / 100.0f);
        if (velocity < 0.0f) {
            morphWeight = 1.0f;
            Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            LinkAnimation_LoadToJoint(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_back_walk, this->modelAnimType),
                                      this->walkFrame);
        } else {
            morphWeight = (REG(37) / 1000.0f) * velocity;
            if (morphWeight < 1.0f) {
                Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            } else {
                morphWeight = 1.0f;
                Player_SetupWalkSfx(this, 1.2f + ((REG(38) / 1000.0f) * velocity));
            }
            LinkAnimation_LoadToMorph(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_back_walk, this->modelAnimType),
                                      this->walkFrame);
            LinkAnimation_LoadToJoint(play, &this->skelAnime, &gPlayerAnim_link_normal_back_run,
                                      this->walkFrame * (16.0f / 29.0f));
        }
    }

    if (morphWeight < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - morphWeight);
    }
}

void Player_SetupHaltFriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_HaltFriendlyBackwalk, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, &gPlayerAnim_link_normal_back_brake);
}

s32 Player_SetupEndFriendlyBackwalk(Player* this, f32* targetVelocity, s16* targetYaw, PlayState* play) {
    if (this->linearVelocity > 6.0f) {
        Player_SetupHaltFriendlyBackwalk(this, play);
        return 1;
    }

    if (*targetVelocity != 0.0f) {
        if (Player_StepLinearVelocityToZero(this)) {
            *targetVelocity = 0.0f;
            *targetYaw = this->currentYaw;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_FriendlyBackwalk(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    s16 targetYawDiff;

    Player_UpdateBackwalk(this, play);

    if (!Player_SetupSubAction(play, this, sFriendlyBackwalkSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupZTargetRunning(this, play, this->currentYaw);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        moveDir = Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play);

        if (moveDir >= 0) {
            if (!Player_SetupEndFriendlyBackwalk(this, &targetVelocity, &targetYaw, play)) {
                if (moveDir != 0) {
                    Player_SetupRun(this, play);
                } else if (targetVelocity > 4.9f) {
                    Player_SetupSidewalk(this, play);
                } else {
                    Player_SetupFriendlySidewalk(this, play);
                }
            }
        } else {
            targetYawDiff = targetYaw - this->currentYaw;

            Math_AsymStepToF(&this->linearVelocity, targetVelocity * 1.5f, 1.5f, 2.0f);
            Math_ScaledStepToS(&this->currentYaw, targetYaw, targetYawDiff * 0.1f);

            if ((targetVelocity == 0.0f) && (this->linearVelocity == 0.0f)) {
                Player_SetupFriendlyZTargetingStandStill(this, play);
            }
        }
    }
}

void Player_SetupEndHaltFriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndHaltFriendlyBackwalk, 1);
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_back_brake_end);
}

void Player_HaltFriendlyBackwalk(Player* this, PlayState* play) {
    s32 animDone;
    f32 targetVelocity;
    s16 targetYaw;

    animDone = LinkAnimation_Update(play, &this->skelAnime);
    Player_StepLinearVelocityToZero(this);

    if (!Player_SetupSubAction(play, this, sFriendlyBackwalkSubActions, 1)) {
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;

            if (Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play) > 0) {
                Player_SetupRun(this, play);
            } else if ((targetVelocity != 0.0f) || (animDone != false)) {
                Player_SetupEndHaltFriendlyBackwalk(this, play);
            }
        }
    }
}

void Player_EndHaltFriendlyBackwalk(Player* this, PlayState* play) {
    s32 animDone;

    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (!Player_SetupSubAction(play, this, sFriendlyBackwalkSubActions, 1)) {
        if (animDone != 0) {
            Player_SetupFriendlyZTargetingStandStill(this, play);
        }
    }
}

void Player_SetupSidewalkAnims(PlayState* play, Player* this) {
    f32 frame;
    LinkAnimationHeader* sidewalkLeftAnim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkL, this->modelAnimType);
    LinkAnimationHeader* sidewalkRightAnim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_side_walkR, this->modelAnimType);

    this->skelAnime.animation = sidewalkLeftAnim;

    Player_SetupWalkSfx(this, (REG(30) / 1000.0f) + ((REG(32) / 1000.0f) * this->linearVelocity));

    frame = this->walkFrame * (16.0f / 29.0f);
    LinkAnimation_BlendToJoint(play, &this->skelAnime, sidewalkRightAnim, frame, sidewalkLeftAnim, frame,
                               this->leftRightBlendWeight, this->blendTable);
}

void Player_Sidewalk(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    s16 targetYawDiff;
    s32 absTargetYawDiff;

    Player_SetupSidewalkAnims(play, this);

    if (!Player_SetupSubAction(play, this, sSidewalkSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if (Player_IsFriendlyZTargeting(this)) {
            moveDir = Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play);
        } else {
            moveDir = Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw);
        }

        if (moveDir > 0) {
            Player_SetupRun(this, play);
            return;
        }

        if (moveDir < 0) {
            if (Player_IsFriendlyZTargeting(this)) {
                Player_SetupFriendlyBackwalk(this, targetYaw, play);
            } else {
                Player_SetupUnfriendlyBackwalk(this, targetYaw, play);
            }
            return;
        }

        if ((this->linearVelocity < 3.6f) && (targetVelocity < 4.0f)) {
            if (!Player_IsUnfriendlyZTargeting(this) && Player_IsFriendlyZTargeting(this)) {
                Player_SetupFriendlySidewalk(this, play);
            } else {
                Player_SetupStandingStillType(this, play);
            }
            return;
        }

        Player_SetLeftRightBlendWeight(this, targetVelocity, targetYaw);

        targetYawDiff = targetYaw - this->currentYaw;
        absTargetYawDiff = ABS(targetYawDiff);

        if (absTargetYawDiff > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 3.0f) != 0) {
                this->currentYaw = targetYaw;
            }
            return;
        }

        targetVelocity *= 0.9f;
        Math_AsymStepToF(&this->linearVelocity, targetVelocity, 2.0f, 3.0f);
        Math_ScaledStepToS(&this->currentYaw, targetYaw, absTargetYawDiff * 0.1f);
    }
}

void Player_Turn(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    LinkAnimation_Update(play, &this->skelAnime);

    if (Player_HoldsTwoHandedWeapon(this)) {
        AnimationContext_SetLoadFrame(play, Player_GetStandingStillAnim(this), 0, this->skelAnime.limbCount,
                                      this->skelAnime.morphTable);
        AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                     this->skelAnime.morphTable, D_80853410);
    }

    Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

    if (!Player_SetupSubAction(play, this, sTurnSubActions, 1)) {
        if (targetVelocity != 0.0f) {
            this->actor.shape.rot.y = targetYaw;
            Player_SetupRun(this, play);
        } else if (Math_ScaledStepToS(&this->actor.shape.rot.y, targetYaw, this->unk_87E)) {
            Player_SetupStandingStillNoMorph(this, play);
        }

        this->currentYaw = this->actor.shape.rot.y;
    }
}

void Player_BlendWalkAnims(Player* this, s32 blendToMorph, PlayState* play) {
    LinkAnimationHeader* anim;
    s16 targetPitch;
    f32 blendWeight;

    if (ABS(sAngleToFloorX) < DEG_TO_BINANG(20.0f)) {
        targetPitch = 0;
    } else {
        targetPitch = CLAMP(sAngleToFloorX, -DEG_TO_BINANG(60.0f), DEG_TO_BINANG(60.0f));
    }

    Math_ScaledStepToS(&this->walkAngleToFloorX, targetPitch, DEG_TO_BINANG(2.1973f));

    if ((this->modelAnimType == PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON) ||
        ((this->walkAngleToFloorX == 0) && (this->shapeOffsetY <= 0.0f))) {
        if (blendToMorph == false) {
            LinkAnimation_LoadToJoint(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType), this->walkFrame);
        } else {
            LinkAnimation_LoadToMorph(play, &this->skelAnime,
                                      GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType), this->walkFrame);
        }
        return;
    }

    if (this->walkAngleToFloorX != 0) {
        blendWeight = this->walkAngleToFloorX / 10922.0f;
    } else {
        blendWeight = this->shapeOffsetY * 0.0006f;
    }

    blendWeight *= fabsf(this->linearVelocity) * 0.5f;

    if (blendWeight > 1.0f) {
        blendWeight = 1.0f;
    }

    if (blendWeight < 0.0f) {
        anim = &gPlayerAnim_link_normal_climb_down;
        blendWeight = -blendWeight;
    } else {
        anim = &gPlayerAnim_link_normal_climb_up;
    }

    if (blendToMorph == false) {
        LinkAnimation_BlendToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType),
                                   this->walkFrame, anim, this->walkFrame, blendWeight, this->blendTable);
    } else {
        LinkAnimation_BlendToMorph(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType),
                                   this->walkFrame, anim, this->walkFrame, blendWeight, this->blendTable);
    }
}

void Player_SetupWalkAnims(Player* this, PlayState* play) {
    f32 morphWeight;
    f32 walkFrameStep;

    if (this->unk_864 < 1.0f) {
        morphWeight = R_UPDATE_RATE * 0.5f;

        Player_SetupWalkSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk, this->modelAnimType),
                                  this->walkFrame);

        this->unk_864 += 1 * morphWeight;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }

        morphWeight = this->unk_864;
    } else {
        walkFrameStep = this->linearVelocity - (REG(48) / 100.0f);

        if (walkFrameStep < 0.0f) {
            morphWeight = 1.0f;
            Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));

            Player_BlendWalkAnims(this, false, play);
        } else {
            morphWeight = (REG(37) / 1000.0f) * walkFrameStep;
            if (morphWeight < 1.0f) {
                Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            } else {
                morphWeight = 1.0f;
                Player_SetupWalkSfx(this, 1.2f + ((REG(38) / 1000.0f) * walkFrameStep));
            }

            Player_BlendWalkAnims(this, true, play);

            LinkAnimation_LoadToJoint(play, &this->skelAnime, Player_GetRunningAnim(this),
                                      this->walkFrame * (20.0f / 29.0f));
        }
    }

    if (morphWeight < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - morphWeight);
    }
}

void Player_Run(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupWalkAnims(this, play);

    if (!Player_SetupSubAction(play, this, sRunSubActions, 1)) {
        if (Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

        if (!Player_SetupTurnAroundRunning(this, &targetVelocity, &targetYaw)) {
            Player_SetRunVelocityAndYaw(this, targetVelocity, targetYaw);
            func_8083DDC8(this, play);

            if ((this->linearVelocity == 0.0f) && (targetVelocity == 0.0f)) {
                Player_SetupEndRun(this, play);
            }
        }
    }
}

void Player_ZTargetingRun(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupWalkAnims(this, play);

    if (!Player_SetupSubAction(play, this, sTargetRunSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if (!Player_SetupTurnAroundRunning(this, &targetVelocity, &targetYaw)) {
            if ((Player_IsFriendlyZTargeting(this) && (targetVelocity != 0.0f) &&
                 (Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play) <= 0)) ||
                (!Player_IsFriendlyZTargeting(this) &&
                 (Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw) <= 0))) {
                Player_SetupStandingStillType(this, play);
                return;
            }

            Player_SetRunVelocityAndYaw(this, targetVelocity, targetYaw);
            func_8083DDC8(this, play);

            if ((this->linearVelocity == 0) && (targetVelocity == 0)) {
                Player_SetupStandingStillType(this, play);
            }
        }
    }
}

void Player_UnfriendlyBackwalk(Player* this, PlayState* play) {
    s32 animDone;
    f32 targetVelocity;
    s16 targetYaw;

    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (!Player_SetupSubAction(play, this, sSidewalkSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if ((this->skelAnime.morphWeight == 0.0f) && (this->skelAnime.curFrame > 5.0f)) {
            Player_StepLinearVelocityToZero(this);

            if ((this->skelAnime.curFrame > 10.0f) &&
                (Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw) < 0)) {
                Player_SetupUnfriendlyBackwalk(this, targetYaw, play);
                return;
            }

            if (animDone != 0) {
                Player_SetupEndUnfriendlyBackwalk(this, play);
            }
        }
    }
}

void Player_EndUnfriendlyBackwalk(Player* this, PlayState* play) {
    s32 animDone;
    f32 targetVelocity;
    s16 targetYaw;

    animDone = LinkAnimation_Update(play, &this->skelAnime);

    Player_StepLinearVelocityToZero(this);

    if (!Player_SetupSubAction(play, this, sEndBackwalkSubActions, 1)) {
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;

            if (Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw) > 0) {
                Player_SetupRun(this, play);
                return;
            }

            if ((targetVelocity != 0.0f) || (animDone != 0)) {
                Player_SetupStandingStillType(this, play);
            }
        }
    }
}

void Player_GetDustPos(Vec3f* footPos, Vec3f* dustPos, f32 floorOffset, f32 scaleXZ, f32 scaleY) {
    dustPos->x = (Rand_ZeroOne() * scaleXZ) + footPos->x;
    dustPos->y = (Rand_ZeroOne() * scaleY) + (footPos->y + floorOffset);
    dustPos->z = (Rand_ZeroOne() * scaleXZ) + footPos->z;
}

static Vec3f D_808545B4 = { 0.0f, 0.0f, 0.0f };
static Vec3f D_808545C0 = { 0.0f, 0.0f, 0.0f };

s32 Player_SetupSpawnDustAtFeet(PlayState* play, Player* this) {
    Vec3f dustPos;

    if ((this->floorSfxOffset == SURFACE_SFX_OFFSET_DIRT) || (this->floorSfxOffset == SURFACE_SFX_OFFSET_SAND)) {
        Player_GetDustPos(&this->actor.shape.feetPos[FOOT_LEFT], &dustPos,
                          this->actor.floorHeight - this->actor.shape.feetPos[FOOT_LEFT].y, 7.0f, 5.0f);
        func_800286CC(play, &dustPos, &D_808545B4, &D_808545C0, 50, 30);

        Player_GetDustPos(&this->actor.shape.feetPos[FOOT_RIGHT], &dustPos,
                          this->actor.floorHeight - this->actor.shape.feetPos[FOOT_RIGHT].y, 7.0f, 5.0f);
        //! @bug This dust is spawned on the right foot itself instead of at the dust position calculated above
        func_800286CC(play, &this->actor.shape.feetPos[FOOT_RIGHT], &D_808545B4, &D_808545C0, 50, 30);
        return 1;
    }

    return 0;
}

void func_8084279C(Player* this, PlayState* play) {
    Player_LoopAnimContinuously(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_check_wait, this->modelAnimType));

    if (DECR(this->genericTimer) == 0) {
        if (!Player_SetupItemCutsceneOrFirstPerson(this, play)) {
            Player_SetupReturnToStandStillSetAnim(
                this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_check_end, this->modelAnimType), play);
        }

        this->actor.flags &= ~ACTOR_FLAG_8;
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    }
}

s32 Player_SetupMeleeAttack(Player* this, f32 startFrame, f32 attackFrame, f32 endFrame) {
    if ((startFrame <= this->skelAnime.curFrame) && (this->skelAnime.curFrame <= endFrame)) {
        Player_MeleeAttack(this, (attackFrame <= this->skelAnime.curFrame) ? 1 : -1);
        return 1;
    }

    Player_InactivateMeleeWeapon(this);
    return 0;
}

s32 Player_AttackWhileDefending(Player* this, PlayState* play) {
    if (!Player_IsChildWithHylianShield(this) && (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) &&
        sUsingItemAlreadyInHand) {
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_defense_kiru);
        this->genericVar = 1;
        this->meleeAttackType = PLAYER_MELEEATKTYPE_STAB_1H;
        this->currentYaw = this->actor.shape.rot.y + this->upperBodyRot.y;
        return 1;
    }

    return 0;
}

// Probably want to rename this one...
s32 Player_IsBusy(Player* this, PlayState* play) {
    return Player_SetupItemCutsceneOrFirstPerson(this, play) || Player_SetupSpeakOrCheck(this, play) ||
           Player_SetupGetItemOrHoldBehavior(this, play);
}

void Player_RequestQuake(PlayState* play, s32 speed, s32 y, s32 duration) {
    s32 quakeIndex = Quake_Request(Play_GetCamera(play, CAM_ID_MAIN), QUAKE_TYPE_3);

    Quake_SetSpeed(quakeIndex, speed);
    Quake_SetPerturbations(quakeIndex, y, 0, 0, 0);
    Quake_SetDuration(quakeIndex, duration);
}

void Player_SetupHammerHit(PlayState* play, Player* this) {
    Player_RequestQuake(play, 27767, 7, 20);
    play->actorCtx.unk_02 = 4;
    Player_RequestRumble(this, 255, 20, 150, 0);
    func_8002F7DC(&this->actor, NA_SE_IT_HAMMER_HIT);
}

void func_80842A88(PlayState* play, Player* this) {
    Inventory_ChangeAmmo(ITEM_DEKU_STICK, -1);
    Player_UseItem(play, this, ITEM_NONE);
}

s32 func_80842AC4(PlayState* play, Player* this) {
    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) && (this->dekuStickLength > 0.5f)) {
        if (AMMO(ITEM_DEKU_STICK) != 0) {
            EffectSsStick_Spawn(play, &this->bodyPartsPos[PLAYER_BODYPART_R_HAND], this->actor.shape.rot.y + 0x8000);
            this->dekuStickLength = 0.5f;
            func_80842A88(play, this);
            func_8002F7DC(&this->actor, NA_SE_IT_WOODSTICK_BROKEN);
        }

        return 1;
    }

    return 0;
}

s32 func_80842B7C(PlayState* play, Player* this) {
    if (this->heldItemAction == PLAYER_IA_SWORD_BGS) {
        if (!gSaveContext.bgsFlag && (gSaveContext.swordHealth > 0.0f)) {
            if ((gSaveContext.swordHealth -= 1.0f) <= 0.0f) {
                EffectSsStick_Spawn(play, &this->bodyPartsPos[PLAYER_BODYPART_R_HAND],
                                    this->actor.shape.rot.y + 0x8000);
                func_800849EC(play);
                func_8002F7DC(&this->actor, NA_SE_IT_MAJIN_SWORD_BROKEN);
            }
        }

        return 1;
    }

    return 0;
}

void func_80842CF0(PlayState* play, Player* this) {
    func_80842AC4(play, this);
    func_80842B7C(play, this);
}

static LinkAnimationHeader* D_808545CC[] = {
    &gPlayerAnim_link_fighter_rebound,
    &gPlayerAnim_link_fighter_rebound_long,
    &gPlayerAnim_link_fighter_reboundR,
    &gPlayerAnim_link_fighter_rebound_longR,
};

void Player_SetupMeleeWeaponRebound(PlayState* play, Player* this) {
    s32 pad;
    s32 sp28;

    if (Player_AimShieldCrouched != this->actionFunc) {
        Player_ResetAttributes(play, this);
        Player_SetActionFunc(play, this, Player_MeleeWeaponRebound, 0);

        if (Player_IsUnfriendlyZTargeting(this)) {
            sp28 = 2;
        } else {
            sp28 = 0;
        }

        Player_PlayAnimOnceSlowed(play, this, D_808545CC[Player_HoldsTwoHandedWeapon(this) + sp28]);
    }

    Player_RequestRumble(this, 180, 20, 100, 0);
    this->linearVelocity = -18.0f;
    func_80842CF0(play, this);
}

s32 func_80842DF4(PlayState* play, Player* this) {
    f32 dist;
    CollisionPoly* colPoly;
    s32 colPolyBgId;
    Vec3f checkPos;
    Vec3f weaponHitPos;
    Vec3f tipBaseDiff;
    s32 temp1;
    s32 surfaceMaterial;

    if (this->isMeleeWeaponAttacking > 0) {
        if (this->meleeAttackType < PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
            if (!(this->meleeWeaponQuads[0].base.atFlags & AT_BOUNCED) &&
                !(this->meleeWeaponQuads[1].base.atFlags & AT_BOUNCED)) {
                if (this->skelAnime.curFrame >= 2.0f) {

                    dist = Math_Vec3f_DistXYZAndStoreDiff(&this->meleeWeaponInfo[0].tip, &this->meleeWeaponInfo[0].base,
                                                          &tipBaseDiff);
                    if (dist != 0.0f) {
                        dist = (dist + 10.0f) / dist;
                    }

                    checkPos.x = this->meleeWeaponInfo[0].tip.x + (tipBaseDiff.x * dist);
                    checkPos.y = this->meleeWeaponInfo[0].tip.y + (tipBaseDiff.y * dist);
                    checkPos.z = this->meleeWeaponInfo[0].tip.z + (tipBaseDiff.z * dist);

                    if (BgCheck_EntityLineTest1(&play->colCtx, &checkPos, &this->meleeWeaponInfo[0].tip, &weaponHitPos,
                                                &colPoly, true, false, false, true, &colPolyBgId) &&
                        !SurfaceType_IsIgnoredByEntities(&play->colCtx, colPoly, colPolyBgId) &&
                        (SurfaceType_GetFloorType(&play->colCtx, colPoly, colPolyBgId) != FLOOR_TYPE_NO_FALL_DAMAGE) &&
                        (func_8002F9EC(play, &this->actor, colPoly, colPolyBgId, &weaponHitPos) == 0)) {

                        if (this->heldItemAction == PLAYER_IA_HAMMER) {
                            Player_SetFreezeFlashTimer(play);
                            Player_SetupHammerHit(play, this);
                            Player_SetupMeleeWeaponRebound(play, this);
                            return 1;
                        }

                        if (this->linearVelocity >= 0.0f) {
                            surfaceMaterial = SurfaceType_GetMaterial(&play->colCtx, colPoly, colPolyBgId);

                            if (surfaceMaterial == SURFACE_MATERIAL_WOOD) {
                                CollisionCheck_SpawnShieldParticlesWood(play, &weaponHitPos, &this->actor.projectedPos);
                            } else {
                                CollisionCheck_SpawnShieldParticles(play, &weaponHitPos);
                                if (surfaceMaterial == SURFACE_MATERIAL_DIRT_SOFT) {
                                    func_8002F7DC(&this->actor, NA_SE_IT_WALL_HIT_SOFT);
                                } else {
                                    func_8002F7DC(&this->actor, NA_SE_IT_WALL_HIT_HARD);
                                }
                            }

                            func_80842CF0(play, this);
                            this->linearVelocity = -14.0f;
                            Player_RequestRumble(this, 180, 20, 100, 0);
                        }
                    }
                }
            } else {
                Player_SetupMeleeWeaponRebound(play, this);
                Player_SetFreezeFlashTimer(play);
                return 1;
            }
        }

        temp1 = (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) || (this->meleeWeaponQuads[1].base.atFlags & AT_HIT);

        if (temp1) {
            if (this->meleeAttackType < PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
                Actor* at = this->meleeWeaponQuads[temp1 ? 1 : 0].base.at;

                if ((at != NULL) && (at->id != ACTOR_EN_KANBAN)) {
                    Player_SetFreezeFlashTimer(play);
                }
            }

            if ((func_80842AC4(play, this) == 0) && (this->heldItemAction != PLAYER_IA_HAMMER)) {
                func_80842B7C(play, this);

                if (this->actor.colChkInfo.atHitEffect == PLAYER_HITEFFECTAT_ELECTRIC) {
                    this->actor.colChkInfo.damage = 8;
                    Player_SetupDamage(play, this, PLAYER_DMGREACTION_ELECTRIC_SHOCKED, 0.0f, 0.0f,
                                       this->actor.shape.rot.y, 20);
                    return 1;
                }
            }
        }
    }

    return 0;
}

void Player_AimShieldCrouched(Player* this, PlayState* play) {
    f32 stickInputY;
    f32 stickInputX;
    s16 yaw;
    s16 shieldPitchTarget;
    s16 shieldYawTarget;
    s16 shieldPitchStep;
    s16 shieldYawStep;
    f32 cosYaw;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_IsChildWithHylianShield(this)) {
            Player_PlayAnimLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense_wait, this->modelAnimType));
        }
        this->genericTimer = 1;
        this->genericVar = 0;
    }

    if (!Player_IsChildWithHylianShield(this)) {
        this->stateFlags1 |= PLAYER_STATE1_22;
        Player_SetupCurrentUpperAction(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_22;
    }

    Player_StepLinearVelocityToZero(this);

    if (this->genericTimer != 0) {
        stickInputY = sControlInput->rel.stick_y * 100;
        stickInputX = sControlInput->rel.stick_x * -120;
        yaw = this->actor.shape.rot.y - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

        cosYaw = Math_CosS(yaw);
        shieldPitchTarget = (Math_SinS(yaw) * stickInputX) + (stickInputY * cosYaw);
        cosYaw = Math_CosS(yaw);
        shieldYawTarget = (stickInputX * cosYaw) - (Math_SinS(yaw) * stickInputY);

        if (shieldPitchTarget > 3500) {
            shieldPitchTarget = 3500;
        }

        shieldPitchStep = ABS(shieldPitchTarget - this->actor.focus.rot.x) * 0.25f;
        if (shieldPitchStep < 100) {
            shieldPitchStep = 100;
        }

        shieldYawStep = ABS(shieldYawTarget - this->upperBodyRot.y) * 0.25f;
        if (shieldYawStep < 50) {
            shieldYawStep = 50;
        }

        Math_ScaledStepToS(&this->actor.focus.rot.x, shieldPitchTarget, shieldPitchStep);
        this->upperBodyRot.x = this->actor.focus.rot.x;
        Math_ScaledStepToS(&this->upperBodyRot.y, shieldYawTarget, shieldYawStep);

        if (this->genericVar != 0) {
            if (!func_80842DF4(play, this)) {
                if (this->skelAnime.curFrame < 2.0f) {
                    Player_MeleeAttack(this, 1);
                }
            } else {
                this->genericTimer = 1;
                this->genericVar = 0;
            }
        } else if (!Player_IsBusy(this, play)) {
            if (Player_SetupDefend(this, play)) {
                Player_AttackWhileDefending(this, play);
            } else {
                this->stateFlags1 &= ~PLAYER_STATE1_22;
                Player_InactivateMeleeWeapon(this);

                if (Player_IsChildWithHylianShield(this)) {
                    Player_SetupReturnToStandStill(this, play);
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_clink_normal_defense_ALL, 1.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_clink_normal_defense_ALL), 0.0f,
                                         ANIMMODE_ONCE, 0.0f);
                    Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE);
                } else {
                    if (this->itemAction < 0) {
                        Player_SetHeldItem(this);
                    }
                    Player_SetupReturnToStandStillSetAnim(
                        this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense_end, this->modelAnimType), play);
                }

                func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_REMOVE);
                return;
            }
        } else {
            return;
        }
    }

    this->stateFlags1 |= PLAYER_STATE1_22;
    Player_SetModelsForHoldingShield(this);

    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X |
                       PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
}

void Player_DeflectAttackWithShield(Player* this, PlayState* play) {
    s32 actionInterruptResult;
    LinkAnimationHeader* anim;
    f32 frames;

    Player_StepLinearVelocityToZero(this);

    if (this->genericVar == 0) {
        D_808535E0 = Player_SetupCurrentUpperAction(this, play);
        if ((Player_StandingDefend == this->upperActionFunc) ||
            (Player_IsActionInterrupted(play, this, &this->skelAnimeUpper, 4.0f) > 0)) {
            Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
        }
    } else {
        actionInterruptResult = Player_IsActionInterrupted(play, this, &this->skelAnime, 4.0f);
        if ((actionInterruptResult != PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) &&
            ((actionInterruptResult > PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) ||
             LinkAnimation_Update(play, &this->skelAnime))) {
            Player_SetActionFunc(play, this, Player_AimShieldCrouched, 1);
            this->stateFlags1 |= PLAYER_STATE1_22;
            Player_SetModelsForHoldingShield(this);
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_defense, this->modelAnimType);
            frames = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frames, frames, ANIMMODE_ONCE, 0.0f);
        }
    }
}

void func_8084370C(Player* this, PlayState* play) {
    s32 sp1C;

    Player_StepLinearVelocityToZero(this);

    sp1C = Player_IsActionInterrupted(play, this, &this->skelAnime, 16.0f);
    if ((sp1C != 0) && (LinkAnimation_Update(play, &this->skelAnime) || (sp1C > 0))) {
        Player_SetupStandingStillType(this, play);
    }
}

void Player_StartKnockback(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    Player_RoundUpInvincibilityTimer(this);

    if (!(this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) && (this->genericTimer == 0) && (this->damageEffect != 0)) {
        s16 temp = this->actor.shape.rot.y - this->damageYaw;

        this->currentYaw = this->actor.shape.rot.y = this->damageYaw;
        this->linearVelocity = this->knockbackVelXZ;

        if (ABS(temp) > 0x4000) {
            this->actor.shape.rot.y = this->damageYaw + 0x8000;
        }

        if (this->actor.velocity.y < 0.0f) {
            this->actor.gravity = 0.0f;
            this->actor.velocity.y = 0.0f;
        }
    }

    if (LinkAnimation_Update(play, &this->skelAnime) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (this->genericTimer != 0) {
            this->genericTimer--;
            if (this->genericTimer == 0) {
                Player_SetupStandingStillMorph(this, play);
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) ||
                   (!(this->cylinder.base.acFlags & AC_HIT) && (this->damageEffect == PLAYER_DMGEFFECT_NONE))) {
            if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
                this->genericTimer++;
            } else {
                Player_SetActionFunc(play, this, Player_DownFromKnockback, 0);
                this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;
            }

            Player_PlayAnimOnce(play, this,
                                (this->currentYaw != this->actor.shape.rot.y) ? &gPlayerAnim_link_normal_front_downB
                                                                              : &gPlayerAnim_link_normal_back_downB);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
        }
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
    }
}

void Player_DownFromKnockback(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    Player_RoundUpInvincibilityTimer(this);

    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime) && (this->linearVelocity == 0.0f)) {
        if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
            this->genericTimer++;
        } else {
            Player_SetActionFunc(play, this, Player_GetUpFromKnockback, 0);
            this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;
        }

        Player_PlayAnimOnceSlowed(play, this,
                                  (this->currentYaw != this->actor.shape.rot.y)
                                      ? &gPlayerAnim_link_normal_front_down_wake
                                      : &gPlayerAnim_link_normal_back_down_wake);
        this->currentYaw = this->actor.shape.rot.y;
    }
}

static PlayerAnimSfxEntry sKnockbackGetUpAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4014 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x401E },
};

void Player_GetUpFromKnockback(Player* this, PlayState* play) {
    s32 actionInterruptResult;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_RoundUpInvincibilityTimer(this);

    if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
        LinkAnimation_Update(play, &this->skelAnime);
    } else {
        actionInterruptResult = Player_IsActionInterrupted(play, this, &this->skelAnime, 16.0f);
        if ((actionInterruptResult != PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) &&
            (LinkAnimation_Update(play, &this->skelAnime) ||
             (actionInterruptResult > PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION))) {
            Player_SetupStandingStillType(this, play);
        }
    }

    Player_PlayAnimSfx(this, sKnockbackGetUpAnimSfx);
}

static Vec3f sDeathReviveFairyPosOffset = { 0.0f, 0.0f, 5.0f };

void Player_EndDie(PlayState* play, Player* this) {
    if (this->genericTimer != 0) {
        if (this->genericTimer > 0) {
            this->genericTimer--;
            if (this->genericTimer == 0) {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_swimer_swim_wait, 1.0f, 0.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_link_swimer_swim_wait), ANIMMODE_ONCE,
                                         -16.0f);
                } else {
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_derth_rebirth, 1.0f, 99.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_link_derth_rebirth), ANIMMODE_ONCE, 0.0f);
                }
                gSaveContext.healthAccumulator = 0x140;
                this->genericTimer = -1;
            }
        } else if (gSaveContext.healthAccumulator == 0) {
            this->stateFlags1 &= ~PLAYER_STATE1_IN_DEATH_CUTSCENE;
            if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                Player_SetupSwimIdle(play, this);
            } else {
                Player_SetupStandingStillMorph(this, play);
            }
            this->deathTimer = 20;
            Player_SetupInvincibilityNoDamageFlash(this, -20);
            Audio_SetBgmVolumeOnDuringFanfare();
        }
    } else if (this->fairyReviveFlag != 0) {
        this->genericTimer = 60;
        Player_SpawnFairy(play, this, &this->actor.world.pos, &sDeathReviveFairyPosOffset, FAIRY_REVIVE_DEATH);
        func_8002F7DC(&this->actor, NA_SE_EV_FIATY_HEAL - SFX_FLAG);
        OnePointCutscene_Init(play, 9908, 125, &this->actor, CAM_ID_MAIN);
    } else if (play->gameOverCtx.state == GAMEOVER_DEATH_WAIT_GROUND) {
        play->gameOverCtx.state = GAMEOVER_DEATH_DELAY_MENU;
    }
}

static PlayerAnimSfxEntry sDyingOrRevivingAnimSfx[] = {
    { NA_SE_PL_BOUND, 0x103C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x408C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x40A4 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x40AA },
};

void Player_Die(Player* this, PlayState* play) {
    if (this->currentTunic != PLAYER_TUNIC_GORON) {
        if ((play->roomCtx.curRoom.behaviorType2 == ROOM_BEHAVIOR_TYPE2_3) ||
            (sFloorType == FLOOR_TYPE_VOID_ON_TOUCH) ||
            ((Player_GetHurtFloorType(sFloorType) >= PLAYER_HURTFLOORTYPE_DEFAULT) &&
             !SurfaceType_IsWallDamage(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId))) {
            Player_StartBurning(this);
        }
    }

    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->actor.category == ACTORCAT_PLAYER) {
            Player_EndDie(play, this);
        }
        return;
    }

    if (this->skelAnime.animation == &gPlayerAnim_link_derth_rebirth) {
        Player_PlayAnimSfx(this, sDyingOrRevivingAnimSfx);
    } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_electric_shock_end) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 88.0f)) {
            Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
        }
    }
}

void Player_PlayFallingVoiceSfx(Player* this, u16 sfxId) {
    Player_PlayVoiceSfxForAge(this, sfxId);

    if ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_EN_RU1)) {
        Audio_PlayActorSfx2(this->heldActor, NA_SE_VO_RT_FALL);
    }
}

static FallImpactInfo sFallImpactInfo[] = {
    { -8, 180, 40, 100, NA_SE_VO_LI_LAND_DAMAGE_S },
    { -16, 255, 140, 150, NA_SE_VO_LI_LAND_DAMAGE_S },
};

// Returns 0 if landed safely, -1 if dead from fall, impactType + 1 if hurt from fall
s32 Player_SetupFallLanding(PlayState* play, Player* this) {
    s32 fallDistance;

    if ((sFloorType == FLOOR_TYPE_NO_FALL_DAMAGE) || (sFloorType == FLOOR_TYPE_VOID_ON_TOUCH)) {
        fallDistance = 0;
    } else {
        fallDistance = this->fallDistance;
    }

    Math_StepToF(&this->linearVelocity, 0.0f, 1.0f);

    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);

    if (fallDistance >= 400) {
        s32 impactType;
        FallImpactInfo* impactInfo;

        if (this->fallDistance < 800) {
            impactType = 0;
        } else {
            impactType = 1;
        }

        impactInfo = &sFallImpactInfo[impactType];

        if (Player_InflictDamageAndCheckForDeath(play, impactInfo->damage)) {
            return -1;
        }

        Player_SetupInvincibility(this, 40);
        Player_RequestQuake(play, 32967, 2, 30);
        Player_RequestRumble(this, impactInfo->rumbleStrength, impactInfo->rumbleDuration,
                             impactInfo->rumbleDecreaseRate, 0);
        func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
        Player_PlayVoiceSfxForAge(this, impactInfo->sfxId);

        return impactType + 1;
    }

    if (fallDistance > 200) {
        fallDistance *= 2;

        if (fallDistance > 255) {
            fallDistance = 255;
        }

        Player_RequestRumble(this, (u8)fallDistance, (u8)(fallDistance * 0.1f), (u8)fallDistance, 0);

        if (sFloorType == FLOOR_TYPE_NO_FALL_DAMAGE) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }
    }

    Player_PlayLandingSfx(this);

    return 0;
}

void Player_ThrowActor(PlayState* play, Player* this, f32 speedXZ, f32 velocityY) {
    Actor* heldActor = this->heldActor;

    if (!Player_InterruptHoldingActor(play, this, heldActor)) {
        heldActor->world.rot.y = this->actor.shape.rot.y;
        heldActor->speedXZ = speedXZ;
        heldActor->velocity.y = velocityY;
        Player_SetupHeldItemUpperActionFunc(play, this);
        func_8002F7DC(&this->actor, NA_SE_PL_THROW);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
}

void Player_UpdateMidair(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    if (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 40) {
        this->actor.gravity = 0.0f;
    } else if (Player_IsUnfriendlyZTargeting(this)) {
        this->actor.gravity = -1.2f;
    }

    Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
            Actor* heldActor = this->heldActor;

            if (!Player_InterruptHoldingActor(play, this, heldActor) && (heldActor->id == ACTOR_EN_NIW) &&
                CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
                Player_ThrowActor(play, this, this->linearVelocity + 2.0f, this->actor.velocity.y + 2.0f);
            }
        }

        LinkAnimation_Update(play, &this->skelAnime);

        if (!(this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING)) {
            func_8083DFE0(this, &targetVelocity, &targetYaw);
        }

        Player_SetupCurrentUpperAction(this, play);

        if (((this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) && (this->genericVar == 2)) ||
            !Player_SetupMidairJumpSlash(this, play)) {
            if (this->actor.velocity.y < 0.0f) {
                if (this->genericTimer >= 0) {
                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_WALL) || (this->genericTimer == 0) ||
                        (this->fallDistance > 0)) {
                        if ((sPlayerYDistToFloor > 800.0f) || (this->stateFlags1 & PLAYER_STATE1_END_HOOKSHOT_MOVE)) {
                            Player_PlayFallingVoiceSfx(this, NA_SE_VO_LI_FALL_S);
                            this->stateFlags1 &= ~PLAYER_STATE1_END_HOOKSHOT_MOVE;
                        }

                        LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_normal_landing, 1.0f, 0.0f, 0.0f,
                                             ANIMMODE_ONCE, 8.0f);
                        this->genericTimer = -1;
                    }
                } else {
                    if ((this->genericTimer == -1) && (this->fallDistance > 120.0f) && (sPlayerYDistToFloor > 280.0f)) {
                        this->genericTimer = -2;
                        Player_PlayFallingVoiceSfx(this, NA_SE_VO_LI_FALL_L);
                    }

                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
                        !(this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) &&
                        !(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_SWIMMING)) &&
                        (this->linearVelocity > 0.0f)) {
                        if ((this->wallHeight >= 150.0f) &&
                            (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0)) {
                            Player_SetupClimbWallOrLadder(this, play, sInteractWallFlags);
                        } else if ((this->touchedWallJumpType >= PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP) &&
                                   (this->wallHeight < 150.0f) &&
                                   (((this->actor.world.pos.y - this->actor.floorHeight) + this->wallHeight) >
                                    (70.0f * this->ageProperties->translationScale))) {
                            AnimationContext_DisableQueue(play);
                            if (this->stateFlags1 & PLAYER_STATE1_END_HOOKSHOT_MOVE) {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HOOKSHOT_HANG);
                            } else {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
                            }
                            this->actor.world.pos.y += this->wallHeight;
                            Player_SetupGrabLedge(
                                play, this, this->actor.wallPoly, this->wallDistance,
                                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_jump_climb_hold, this->modelAnimType));
                            this->actor.shape.rot.y = this->currentYaw += 0x8000;
                            this->stateFlags1 |= PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP;
                        }
                    }
                }
            }
        }
    } else {
        LinkAnimationHeader* anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_landing, this->modelAnimType);
        s32 fallLandingResult;

        if (this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) {
            if (Player_IsUnfriendlyZTargeting(this)) {
                anim = sManualJumpAnims[this->relativeStickInput][2];
            } else {
                anim = sManualJumpAnims[this->relativeStickInput][1];
            }
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_run_jump) {
            anim = &gPlayerAnim_link_normal_run_jump_end;
        } else if (Player_IsUnfriendlyZTargeting(this)) {
            anim = &gPlayerAnim_link_anchor_landingR;
            Player_ResetLeftRightBlendWeight(this);
        } else if (this->fallDistance <= 80) {
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_short_landing, this->modelAnimType);
        } else if ((this->fallDistance < 800) && (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0) &&
                   !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            Player_SetupRolling(this, play);
            return;
        }

        fallLandingResult = Player_SetupFallLanding(play, this);

        if (fallLandingResult > 0) {
            Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_landing, this->modelAnimType),
                                                  play);
            this->skelAnime.endFrame = 8.0f;
            if (fallLandingResult == 1) {
                this->genericTimer = 10;
            } else {
                this->genericTimer = 20;
            }
        } else if (fallLandingResult == 0) {
            Player_SetupReturnToStandStillSetAnim(this, anim, play);
        }
    }
}

static PlayerAnimSfxEntry sRollAnimSfx[] = {
    { NA_SE_VO_LI_SWORD_N, 0x2001 },
    { NA_SE_PL_WALK_GROUND, 0x1806 },
    { NA_SE_PL_ROLL, 0x806 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x2812 },
};

void Player_Rolling(Player* this, PlayState* play) {
    Actor* cylinderOc;
    s32 actionInterruptResult;
    s32 animDone;
    DynaPolyActor* wallPolyActor;
    s32 pad;
    f32 targetVelocity;
    s16 targetYaw;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    cylinderOc = NULL;
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 8.0f)) {
        Player_SetupInvincibilityNoDamageFlash(this, -10);
    }

    if (Player_IsBusy(this, play) == 0) {
        if (this->genericTimer != 0) {
            Math_StepToF(&this->linearVelocity, 0.0f, 2.0f);

            actionInterruptResult = Player_IsActionInterrupted(play, this, &this->skelAnime, 5.0f);
            if ((actionInterruptResult != PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) &&
                ((actionInterruptResult > PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) || animDone)) {
                Player_SetupReturnToStandStill(this, play);
            }
        } else {
            if (this->linearVelocity >= 7.0f) {
                if (((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sYawToTouchedWall2 < 0x2000)) ||
                    ((this->cylinder.base.ocFlags1 & OC1_HIT) &&
                     (cylinderOc = this->cylinder.base.oc,
                      ((cylinderOc->id == ACTOR_EN_WOOD02) &&
                       (ABS((s16)(this->actor.world.rot.y - cylinderOc->yawTowardsPlayer)) > 0x6000))))) {

                    if (cylinderOc != NULL) {
                        cylinderOc->home.rot.y = 1;
                    } else if (this->actor.wallBgId != BGCHECK_SCENE) {
                        wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
                        if ((wallPolyActor != NULL) && (wallPolyActor->actor.id == ACTOR_OBJ_KIBAKO2)) {
                            wallPolyActor->actor.home.rot.z = 1;
                        }
                    }

                    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_hip_down, this->modelAnimType));
                    this->linearVelocity = -this->linearVelocity;
                    Player_RequestQuake(play, 33267, 3, 12);
                    Player_RequestRumble(this, 255, 20, 150, 0);
                    func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
                    this->genericTimer = 1;
                    return;
                }
            }

            if ((this->skelAnime.curFrame < 15.0f) || !Player_SetupStartMeleeWeaponAttack(this, play)) {
                if (this->skelAnime.curFrame >= 20.0f) {
                    Player_SetupReturnToStandStill(this, play);
                    return;
                }

                Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

                targetVelocity *= 1.5f;
                if ((targetVelocity < 3.0f) || (this->relativeAnalogStickInputs[this->inputFrameCounter] != 0)) {
                    targetVelocity = 3.0f;
                }

                Player_SetRunVelocityAndYaw(this, targetVelocity, this->actor.shape.rot.y);

                if (Player_SetupSpawnDustAtFeet(play, this)) {
                    func_8002F8F0(&this->actor, NA_SE_PL_ROLL_DUST - SFX_FLAG);
                }

                Player_PlayAnimSfx(this, sRollAnimSfx);
            }
        }
    }
}

void Player_FallingDive(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_run_jump_water_fall_wait);
    }

    Math_StepToF(&this->linearVelocity, 0.0f, 0.05f);

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        this->actor.colChkInfo.damage = 0x10;
        Player_SetupDamage(play, this, PLAYER_DMGREACTION_KNOCKBACK, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
    }
}

void Player_JumpSlash(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    this->actor.gravity = -1.2f;
    LinkAnimation_Update(play, &this->skelAnime);

    if (!func_80842DF4(play, this)) {
        Player_SetupMeleeAttack(this, 6.0f, 7.0f, 99.0f);

        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
            func_8083DFE0(this, &targetVelocity, &this->currentYaw);
            return;
        }

        if (Player_SetupFallLanding(play, this) >= 0) {
            this->meleeAttackType += 2;
            Player_StartMeleeWeaponAttack(play, this, this->meleeAttackType);
            this->slashCounter = 3;
            Player_PlayLandingSfx(this);
        }
    }
}

s32 Player_SetupReleaseSpinAttack(Player* this, PlayState* play) {
    s32 meleeAttackType;

    if (Player_SetupCutscene(play, this)) {
        this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
    } else {
        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
            if ((this->spinAttackTimer >= 0.85f) || Player_CanQuickspin(this)) {
                meleeAttackType = sBigSpinAttackMWAs[Player_HoldsTwoHandedWeapon(this)];
            } else {
                meleeAttackType = sSmallSpinAttackMWAs[Player_HoldsTwoHandedWeapon(this)];
            }

            Player_StartMeleeWeaponAttack(play, this, meleeAttackType);
            Player_SetupInvincibilityNoDamageFlash(this, -8);

            this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
            if (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0) {
                this->stateFlags2 |= PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
            }
        } else {
            return 0;
        }
    }

    return 1;
}

void Player_SetupWalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_WalkChargingSpinAttack, 1);
}

void Player_SetupSidewalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_SidewalkChargingSpinAttack, 1);
}

void Player_CancelSpinAttackCharge(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    Player_InactivateMeleeWeapon(this);
    Player_ChangeAnimMorphToLastFrame(play, this, sCancelSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)]);
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupChargeSpinAttack(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_ChargeSpinAttack, 1);
    this->walkFrame = 0.0f;
    Player_PlayAnimLoop(play, this, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)]);
    this->genericTimer = 1;
}

void Player_UpdateSpinAttackTimer(Player* this) {
    Math_StepToF(&this->spinAttackTimer, 1.0f, 0.02f);
}

void Player_ChargeSpinAttack(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        Player_SetZTargetFriendlyYaw(this);
        this->stateFlags1 &= ~PLAYER_STATE1_Z_TARGETING_FRIENDLY;
        Player_PlayAnimLoop(play, this, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)]);
        this->genericTimer = -1;
    }

    Player_StepLinearVelocityToZero(this);

    if (!Player_IsBusy(this, play) && (this->genericTimer != 0)) {
        Player_UpdateSpinAttackTimer(this);

        if (this->genericTimer < 0) {
            if (this->spinAttackTimer >= 0.1f) {
                this->slashCounter = 0;
                this->genericTimer = 1;
            } else if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
                Player_CancelSpinAttackCharge(this, play);
            }
        } else if (!Player_SetupReleaseSpinAttack(this, play)) {
            Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

            moveDir = Player_GetSpinAttackMoveDirection(this, &targetVelocity, &targetYaw, play);
            if (moveDir > 0) {
                Player_SetupWalkChargingSpinAttack(this, play);
            } else if (moveDir < 0) {
                Player_SetupSidewalkChargingSpinAttack(this, play);
            }
        }
    }
}

void Player_WalkChargingSpinAttack(Player* this, PlayState* play) {
    s16 curYawDiff;
    s32 absCurYawDiff;
    f32 absLinearVelocity;
    f32 blendWeight;
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    s16 targetYawDiff;
    s32 absTargetYawDiff;

    curYawDiff = this->currentYaw - this->actor.shape.rot.y;
    absCurYawDiff = ABS(curYawDiff);

    absLinearVelocity = fabsf(this->linearVelocity);
    blendWeight = absLinearVelocity * 1.5f;

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (blendWeight < 1.5f) {
        blendWeight = 1.5f;
    }

    blendWeight = ((absCurYawDiff < 0x4000) ? -1.0f : 1.0f) * blendWeight;

    Player_SetupWalkSfx(this, blendWeight);

    blendWeight = CLAMP(absLinearVelocity * 0.5f, 0.5f, 1.0f);

    LinkAnimation_BlendToJoint(play, &this->skelAnime, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)], 0.0f,
                               sSpinAttackChargeWalkAnims[Player_HoldsTwoHandedWeapon(this)],
                               this->walkFrame * (21.0f / 29.0f), blendWeight, this->blendTable);

    if (!Player_IsBusy(this, play) && !Player_SetupReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        moveDir = Player_GetSpinAttackMoveDirection(this, &targetVelocity, &targetYaw, play);

        if (moveDir < 0) {
            Player_SetupSidewalkChargingSpinAttack(this, play);
            return;
        }

        if (moveDir == 0) {
            targetVelocity = 0.0f;
            targetYaw = this->currentYaw;
        }

        targetYawDiff = targetYaw - this->currentYaw;
        absTargetYawDiff = ABS(targetYawDiff);

        if (absTargetYawDiff > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                this->currentYaw = targetYaw;
            }
            return;
        }

        Math_AsymStepToF(&this->linearVelocity, targetVelocity * 0.2f, 1.0f, 0.5f);
        Math_ScaledStepToS(&this->currentYaw, targetYaw, absTargetYawDiff * 0.1f);

        if ((targetVelocity == 0.0f) && (this->linearVelocity == 0.0f)) {
            Player_SetupChargeSpinAttack(this, play);
        }
    }
}

void Player_SidewalkChargingSpinAttack(Player* this, PlayState* play) {
    f32 absLinearVelocity;
    f32 blendWeight;
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    s16 targetYawDiff;
    s32 absTargetYawDiff;

    absLinearVelocity = fabsf(this->linearVelocity);

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (absLinearVelocity == 0.0f) {
        absLinearVelocity = ABS(this->unk_87C) * 0.0015f;
        if (absLinearVelocity < 400.0f) {
            absLinearVelocity = 0.0f;
        }
        Player_SetupWalkSfx(this, ((this->unk_87C >= 0) ? 1 : -1) * absLinearVelocity);
    } else {
        blendWeight = absLinearVelocity * 1.5f;
        if (blendWeight < 1.5f) {
            blendWeight = 1.5f;
        }
        Player_SetupWalkSfx(this, blendWeight);
    }

    blendWeight = CLAMP(absLinearVelocity * 0.5f, 0.5f, 1.0f);

    LinkAnimation_BlendToJoint(play, &this->skelAnime, sSpinAttackChargeAnims[Player_HoldsTwoHandedWeapon(this)], 0.0f,
                               sSpinAttackChargeSidewalkAnims[Player_HoldsTwoHandedWeapon(this)],
                               this->walkFrame * (21.0f / 29.0f), blendWeight, this->blendTable);

    if (!Player_IsBusy(this, play) && !Player_SetupReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        moveDir = Player_GetSpinAttackMoveDirection(this, &targetVelocity, &targetYaw, play);

        if (moveDir > 0) {
            Player_SetupWalkChargingSpinAttack(this, play);
            return;
        }

        if (moveDir == 0) {
            targetVelocity = 0.0f;
            targetYaw = this->currentYaw;
        }

        targetYawDiff = targetYaw - this->currentYaw;
        absTargetYawDiff = ABS(targetYawDiff);

        if (absTargetYawDiff > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                this->currentYaw = targetYaw;
            }
            return;
        }

        Math_AsymStepToF(&this->linearVelocity, targetVelocity * 0.2f, 1.0f, 0.5f);
        Math_ScaledStepToS(&this->currentYaw, targetYaw, absTargetYawDiff * 0.1f);

        if ((targetVelocity == 0.0f) && (this->linearVelocity == 0.0f) && (absLinearVelocity == 0.0f)) {
            Player_SetupChargeSpinAttack(this, play);
        }
    }
}

void Player_JumpUpToLedge(Player* this, PlayState* play) {
    s32 animDone;
    f32 jumpVelocityY;
    s32 actionInterruptResult;
    f32 landingSfxFrame;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (this->skelAnime.animation == &gPlayerAnim_link_normal_250jump_start) {
        this->linearVelocity = 1.0f;

        if (LinkAnimation_OnFrame(&this->skelAnime, 8.0f)) {
            jumpVelocityY = this->wallHeight;

            if (jumpVelocityY > this->ageProperties->unk_0C) {
                jumpVelocityY = this->ageProperties->unk_0C;
            }

            if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                jumpVelocityY *= 0.085f;
            } else {
                jumpVelocityY *= 0.072f;
            }

            if (!LINK_IS_ADULT) {
                jumpVelocityY += 1.0f;
            }

            Player_SetupJumpWithSfx(this, NULL, jumpVelocityY, play, NA_SE_VO_LI_AUTO_JUMP);
            this->genericTimer = -1;
            return;
        }
    } else {
        actionInterruptResult = Player_IsActionInterrupted(play, this, &this->skelAnime, 4.0f);

        if (actionInterruptResult == PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) {
            this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_JUMPING);
            return;
        }

        if ((animDone != 0) || (actionInterruptResult > PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION)) {
            Player_SetupStandingStillNoMorph(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_JUMPING);
            return;
        }

        landingSfxFrame = 0.0f;

        if (this->skelAnime.animation == &gPlayerAnim_link_swimer_swim_15step_up) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                Player_StartJumpOutOfWater(play, this, 10.0f);
            }
            landingSfxFrame = 50.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_150step_up) {
            landingSfxFrame = 30.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_100step_up) {
            landingSfxFrame = 16.0f;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, landingSfxFrame)) {
            Player_PlayLandingSfx(this);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }

        if ((this->skelAnime.animation == &gPlayerAnim_link_normal_100step_up) || (this->skelAnime.curFrame > 5.0f)) {
            if (this->genericTimer == 0) {
                Player_PlayJumpSfx(this);
                this->genericTimer = 1;
            }
            Math_StepToF(&this->actor.shape.yOffset, 0.0f, 150.0f);
        }
    }
}

void Player_RunMiniCutsceneFunc(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    LinkAnimation_Update(play, &this->skelAnime);

    if (((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor != NULL) &&
         (this->getItemId == GI_NONE)) ||
        !Player_SetupCurrentUpperAction(this, play)) {
        this->miniCsFunc(play, this);
    }
}

s32 Player_CutsceneMoveUsingActionPos(PlayState* play, Player* this, CsCmdActorAction* linkCsAction, f32 targetVelocity,
                                      s16 targetYaw, s32 moveMode) {
    if ((moveMode != 0) && (this->linearVelocity == 0.0f)) {
        return LinkAnimation_Update(play, &this->skelAnime);
    }

    if (moveMode != 2) {
        f32 rate = R_UPDATE_RATE * 0.5f;
        f32 distToEndX = linkCsAction->endPos.x - this->actor.world.pos.x;
        f32 distToEndZ = linkCsAction->endPos.z - this->actor.world.pos.z;
        f32 distToEndXZ = sqrtf(SQ(distToEndX) + SQ(distToEndZ)) / rate;
        s32 framesUntilEnd = (linkCsAction->endFrame - play->csCtx.frames) + 1;

        // Set target yaw toward end position
        targetYaw = Math_Atan2S(distToEndZ, distToEndX);

        if (moveMode == 1) {
            f32 startDistToEndX = linkCsAction->endPos.x - linkCsAction->startPos.x;
            f32 startDistToEndZ = linkCsAction->endPos.z - linkCsAction->startPos.z;
            s32 temp = (((sqrtf(SQ(startDistToEndX) + SQ(startDistToEndZ)) / rate) /
                         (linkCsAction->endFrame - linkCsAction->startFrame)) /
                        1.5f) *
                       4.0f;

            if (temp >= framesUntilEnd) {
                targetYaw = this->actor.shape.rot.y;
                targetVelocity = 0.0f;
            } else {
                targetVelocity = distToEndXZ / ((framesUntilEnd - temp) + 1);
            }
        } else {
            targetVelocity = distToEndXZ / framesUntilEnd;
        }
    }

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupWalkAnims(this, play);
    Player_SetRunVelocityAndYaw(this, targetVelocity, targetYaw);

    if ((targetVelocity == 0.0f) && (this->linearVelocity == 0.0f)) {
        Player_EndRun(this, play);
    }

    return 0;
}

// Returns remaining distance until within range
s32 Player_CutsceneMoveUsingActionPosIntoRange(PlayState* play, Player* this, f32* targetVelocity, s32 endRange) {
    f32 dx = this->csStartPos.x - this->actor.world.pos.x;
    f32 dz = this->csStartPos.z - this->actor.world.pos.z;
    s32 dist = sqrtf(SQ(dx) + SQ(dz));
    s16 targetYaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->csStartPos);

    if (dist < endRange) {
        *targetVelocity = 0.0f;
        targetYaw = this->actor.shape.rot.y;
    }

    if (Player_CutsceneMoveUsingActionPos(play, this, NULL, *targetVelocity, targetYaw, 2)) {
        return 0;
    }

    return dist;
}

s32 func_80845C68(PlayState* play, s32 arg1) {
    if (arg1 == 0) {
        Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
    }
    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 0;
    return arg1;
}

void Player_MiniCsMovement(Player* this, PlayState* play) {
    f32 targetVelocity;
    s32 distToRange;
    f32 targetVelocity2;
    s32 endRange;
    s32 pad;

    if (!Player_SetupItemCutsceneOrFirstPerson(this, play)) {
        if (this->genericTimer == 0) {
            LinkAnimation_Update(play, &this->skelAnime);

            if (DECR(this->doorTimer) == 0) {
                this->linearVelocity = 0.1f;
                this->genericTimer = 1;
            }
        } else if (this->genericVar == 0) {
            targetVelocity = 5.0f * sWaterSpeedScale;

            if (Player_CutsceneMoveUsingActionPosIntoRange(play, this, &targetVelocity, -1) < 30) {
                this->genericVar = 1;
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;

                this->csStartPos.x = this->csEndPos.x;
                this->csStartPos.z = this->csEndPos.z;
            }
        } else {
            targetVelocity2 = 5.0f;
            endRange = 20;

            if (this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) {
                targetVelocity2 = gSaveContext.entranceSpeed;

                if (sConveyorSpeedIndex != 0) {
                    this->csStartPos.x = (Math_SinS(sConveyorYaw) * 400.0f) + this->actor.world.pos.x;
                    this->csStartPos.z = (Math_CosS(sConveyorYaw) * 400.0f) + this->actor.world.pos.z;
                }
            } else if (this->genericTimer < 0) {
                this->genericTimer++;

                targetVelocity2 = gSaveContext.entranceSpeed;
                endRange = -1;
            }

            distToRange = Player_CutsceneMoveUsingActionPosIntoRange(play, this, &targetVelocity2, endRange);

            if ((this->genericTimer == 0) || ((distToRange == 0) && (this->linearVelocity == 0.0f) &&
                                              (Play_GetCamera(play, CAM_ID_MAIN)->unk_14C & 0x10))) {

                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
                func_80845C68(play, gSaveContext.respawn[RESPAWN_MODE_DOWN].data);

                if (!Player_SetupSpeakOrCheck(this, play)) {
                    Player_EndMiniCsMovement(this, play);
                }
            }
        }
    }

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        Player_SetupCurrentUpperAction(this, play);
    }
}

void Player_OpenDoor(Player* this, PlayState* play) {
    s32 animDone;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    Player_SetupCurrentUpperAction(this, play);

    if (animDone) {
        if (this->genericTimer == 0) {
            if (DECR(this->doorTimer) == 0) {
                this->genericTimer = 1;
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
            if (play->roomCtx.prevRoom.num >= 0) {
                func_80097534(play, &play->roomCtx);
            }
            func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
        }
        return;
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) && LinkAnimation_OnFrame(&this->skelAnime, 15.0f)) {
        play->func_11D54(this, play);
    }
}

void Player_LiftActor(Player* this, PlayState* play) {
    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillType(this, play);
        Player_SetupHoldActorUpperAction(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 4.0f)) {
        Actor* interactRangeActor = this->interactRangeActor;

        if (!Player_InterruptHoldingActor(play, this, interactRangeActor)) {
            this->heldActor = interactRangeActor;
            this->actor.child = interactRangeActor;
            interactRangeActor->parent = &this->actor;
            interactRangeActor->bgCheckFlags &=
                ~(BGCHECKFLAG_GROUND | BGCHECKFLAG_GROUND_TOUCH | BGCHECKFLAG_GROUND_LEAVE | BGCHECKFLAG_WALL |
                  BGCHECKFLAG_CEILING | BGCHECKFLAG_WATER | BGCHECKFLAG_WATER_TOUCH | BGCHECKFLAG_GROUND_STRICT);
            this->leftHandRot.y = interactRangeActor->shape.rot.y - this->actor.shape.rot.y;
        }
        return;
    }

    Math_ScaledStepToS(&this->leftHandRot.y, 0, 4000);
}

static PlayerAnimSfxEntry sThrowStonePillarAnimSfx[] = {
    { NA_SE_VO_LI_SWORD_L, 0x2031 },
    { NA_SE_VO_LI_SWORD_N, -0x20E6 },
};

void Player_ThrowStonePillar(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->genericTimer++ > 20)) {
        if (!Player_SetupItemCutsceneOrFirstPerson(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_heavy_carry_end, play);
        }
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 41.0f)) {
        BgHeavyBlock* heavyBlock = (BgHeavyBlock*)this->interactRangeActor;

        this->heldActor = &heavyBlock->dyna.actor;
        this->actor.child = &heavyBlock->dyna.actor;
        heavyBlock->dyna.actor.parent = &this->actor;
        func_8002DBD0(&heavyBlock->dyna.actor, &heavyBlock->unk_164, &this->leftHandPos);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 229.0f)) {
        Actor* heldActor = this->heldActor;

        heldActor->speedXZ = Math_SinS(heldActor->shape.rot.x) * 40.0f;
        heldActor->velocity.y = Math_CosS(heldActor->shape.rot.x) * 40.0f;
        heldActor->gravity = -2.0f;
        heldActor->minVelocityY = -30.0f;
        Player_DetatchHeldActor(play, this);
        return;
    }

    Player_PlayAnimSfx(this, sThrowStonePillarAnimSfx);
}

void Player_LiftSilverBoulder(Player* this, PlayState* play) {
    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_silver_wait);
        this->genericTimer = 1;
        return;
    }

    if (this->genericTimer == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 27.0f)) {
            Actor* interactRangeActor = this->interactRangeActor;

            this->heldActor = interactRangeActor;
            this->actor.child = interactRangeActor;
            interactRangeActor->parent = &this->actor;
            return;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, 25.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
            return;
        }

    } else if (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
        Player_SetActionFunc(play, this, Player_ThrowSilverBoulder, 1);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_silver_throw);
    }
}

void Player_ThrowSilverBoulder(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillType(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Actor* heldActor = this->heldActor;

        heldActor->world.rot.y = this->actor.shape.rot.y;
        heldActor->speedXZ = 10.0f;
        heldActor->velocity.y = 20.0f;
        Player_SetupHeldItemUpperActionFunc(play, this);
        func_8002F7DC(&this->actor, NA_SE_PL_THROW);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
}

void Player_FailToLiftActor(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_nocarry_free_wait);
        this->genericTimer = 15;
        return;
    }

    if (this->genericTimer != 0) {
        this->genericTimer--;
        if (this->genericTimer == 0) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_nocarry_free_end, play);
            this->stateFlags1 &= ~PLAYER_STATE1_HOLDING_ACTOR;
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        }
    }
}

void Player_SetupPutDownActor(Player* this, PlayState* play) {
    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillType(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 4.0f)) {
        Actor* heldActor = this->heldActor;

        if (!Player_InterruptHoldingActor(play, this, heldActor)) {
            heldActor->velocity.y = 0.0f;
            heldActor->speedXZ = 0.0f;
            Player_SetupHeldItemUpperActionFunc(play, this);
            if (heldActor->id == ACTOR_EN_BOM_CHU) {
                Player_ForceFirstPerson(this, play);
            }
        }
    }
}

void Player_StartThrowActor(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime) ||
        ((this->skelAnime.curFrame >= 8.0f) &&
         Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play))) {
        Player_SetupStandingStillType(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Player_ThrowActor(play, this, this->linearVelocity + 8.0f, 12.0f);
    }
}

static ColliderCylinderInit sBodyCylInit = {
    {
        COLTYPE_HIT5,
        AT_NONE,
        AC_ON | AC_TYPE_ENEMY,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_PLAYER,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK1,
        { 0x00000000, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 12, 60, 0, { 0, 0, 0 } },
};

static ColliderQuadInit sMeleeWpnQuadInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000100, 0x00, 0x01 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

static ColliderQuadInit sShieldQuadInit = {
    {
        COLTYPE_METAL,
        AT_ON | AT_TYPE_PLAYER,
        AC_ON | AC_HARD | AC_TYPE_ENEMY,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00100000, 0x00, 0x00 },
        { 0xDFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_NONE,
    },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

void Player_DoNothing3(Actor* thisx, PlayState* play) {
}

void Player_SpawnNoUpdateOrDraw(PlayState* play, Player* this) {
    this->actor.update = Player_DoNothing3;
    this->actor.draw = NULL;
}

void Player_SetupSpawnFromBlueWarp(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SpawnFromBlueWarp, 0);
    if ((play->sceneId == SCENE_LAKE_HYLIA) && (gSaveContext.sceneLayer >= 4)) {
        this->genericVar = 1;
    }
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_okarina_warp_goal, 2.0f / 3.0f, 0.0f, 24.0f,
                         ANIMMODE_ONCE, 0.0f);
    this->actor.world.pos.y += 800.0f;
}

static u8 swordItemsByAge[] = { ITEM_SWORD_MASTER, ITEM_SWORD_KOKIRI };

void Player_CutsceneDrawSword(PlayState* play, Player* this, s32 arg2) {
    s32 item = swordItemsByAge[(void)0, gSaveContext.linkAge];
    s32 itemAction = sItemActions[item];

    Player_PutAwayHookshot(this);
    Player_DetatchHeldActor(play, this);

    this->heldItemId = item;
    this->nextModelGroup = Player_ActionToModelGroup(this, itemAction);

    Player_ChangeItem(play, this, itemAction);
    Player_SetupHeldItemUpperActionFunc(play, this);

    if (arg2 != 0) {
        func_8002F7DC(&this->actor, NA_SE_IT_SWORD_PICKOUT);
    }
}

static Vec3f sEndTimeTravelStartPos = { -1.0f, 69.0f, 20.0f };

void Player_SpawnFromTimeTravel(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_EndTimeTravel, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    Math_Vec3f_Copy(&this->actor.world.pos, &sEndTimeTravelStartPos);
    this->currentYaw = this->actor.shape.rot.y = -0x8000;
    LinkAnimation_Change(play, &this->skelAnime, this->ageProperties->timeTravelEndAnim, 2.0f / 3.0f, 0.0f, 0.0f,
                         ANIMMODE_ONCE, 0.0f);
    Player_SetupAnimMovement(play, this,
                             PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                 PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                 PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_7 |
                                 PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
    if (LINK_IS_ADULT) {
        Player_CutsceneDrawSword(play, this, 0);
    }
    this->genericTimer = 20;
}

void Player_SpawnOpeningDoor(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SetupOpenDoorFromSpawn, 0);
    Player_SetupAnimMovement(play, this,
                             PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                 PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                 PLAYER_ANIMMOVEFLAGS_7);
}

void Player_SpawnExitingGrotto(PlayState* play, Player* this) {
    Player_SetupJump(this, &gPlayerAnim_link_normal_jump, 12.0f, play);
    Player_SetActionFunc(play, this, Player_JumpFromGrotto, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    this->fallStartHeight = this->actor.world.pos.y;
    OnePointCutscene_Init(play, 5110, 40, &this->actor, CAM_ID_MAIN);
}

void Player_SpawnWithKnockback(PlayState* play, Player* this) {
    Player_SetupDamage(play, this, PLAYER_DMGREACTION_KNOCKBACK, 2.0f, 2.0f, this->actor.shape.rot.y + 0x8000, 0);
}

void Player_SetupSpawnFromWarpSong(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SpawnFromWarpSong, 0);
    this->actor.draw = NULL;
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
}

static s16 sMagicSpellActors[] = { ACTOR_MAGIC_WIND, ACTOR_MAGIC_DARK, ACTOR_MAGIC_FIRE };

Actor* Player_SpawnMagicSpellActor(PlayState* play, Player* this, s32 index) {
    return Actor_Spawn(&play->actorCtx, play, sMagicSpellActors[index], this->actor.world.pos.x,
                       this->actor.world.pos.y, this->actor.world.pos.z, 0, 0, 0, 0);
}

void Player_SetupSpawnFromFaroresWind(PlayState* play, Player* this) {
    this->actor.draw = NULL;
    Player_SetActionFunc(play, this, Player_SpawnFromFaroresWind, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
}

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(targetArrowOffset, 500, ICHAIN_STOP),
};

static EffectBlureInit2 sBlure2InitParams = {
    0, 8, 0, { 255, 255, 255, 255 }, { 255, 255, 255, 64 }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, 4,
    0, 2, 0, { 0, 0, 0, 0 },         { 0, 0, 0, 0 },
};

static Vec3s sBaseTransl = { -57, 3377, 0 };

void Player_InitCommon(Player* this, PlayState* play, FlexSkeletonHeader* skelHeader) {
    this->ageProperties = &sAgeProperties[gSaveContext.linkAge];
    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->meleeWeaponEffectIndex = TOTAL_EFFECT_COUNT;
    this->currentYaw = this->actor.world.rot.y;
    Player_SetupHeldItemUpperActionFunc(play, this);

    SkelAnime_InitLink(play, &this->skelAnime, skelHeader, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_wait, this->modelAnimType),
                       9, this->jointTable, this->morphTable, PLAYER_LIMB_MAX);
    this->skelAnime.baseTransl = sBaseTransl;
    SkelAnime_InitLink(play, &this->skelAnimeUpper, skelHeader, Player_GetStandingStillAnim(this), 9,
                       this->jointTableUpper, this->morphTableUpper, PLAYER_LIMB_MAX);
    this->skelAnimeUpper.baseTransl = sBaseTransl;

    Effect_Add(play, &this->meleeWeaponEffectIndex, EFFECT_BLURE2, 0, 0, &sBlure2InitParams);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawFeet, this->ageProperties->unk_04);
    this->subCamId = CAM_ID_NONE;

    Collider_InitCylinder(play, &this->cylinder);
    Collider_SetCylinder(play, &this->cylinder, &this->actor, &sBodyCylInit);
    Collider_InitQuad(play, &this->meleeWeaponQuads[0]);
    Collider_SetQuad(play, &this->meleeWeaponQuads[0], &this->actor, &sMeleeWpnQuadInit);
    Collider_InitQuad(play, &this->meleeWeaponQuads[1]);
    Collider_SetQuad(play, &this->meleeWeaponQuads[1], &this->actor, &sMeleeWpnQuadInit);
    Collider_InitQuad(play, &this->shieldQuad);
    Collider_SetQuad(play, &this->shieldQuad, &this->actor, &sShieldQuadInit);
}

static void (*sSpawnFuncs[])(PlayState* play, Player* this) = {
    Player_SpawnNoUpdateOrDraw,          // PLAYER_SPAWNMODE_NO_UPDATE_OR_DRAW
    Player_SpawnFromTimeTravel,          // PLAYER_SPAWNMODE_FROM_TIME_TRAVEL
    Player_SetupSpawnFromBlueWarp,       // PLAYER_SPAWNMODE_FROM_BLUE_WARP
    Player_SpawnOpeningDoor,             // PLAYER_SPAWNMODE_OPENING_DOOR
    Player_SpawnExitingGrotto,           // PLAYER_SPAWNMODE_EXITING_GROTTO
    Player_SetupSpawnFromWarpSong,       // PLAYER_SPAWNMODE_FROM_WARP_SONG
    Player_SetupSpawnFromFaroresWind,    // PLAYER_SPAWNMODE_FROM_FARORES_WIND
    Player_SpawnWithKnockback,           // PLAYER_SPAWNMODE_WITH_KNOCKBACK
    Player_SpawnWalkingSlow,             // PLAYER_SPAWNMODE_UNK_8
    Player_SpawnWalkingSlow,             // PLAYER_SPAWNMODE_UNK_9
    Player_SpawnWalkingSlow,             // PLAYER_SPAWNMODE_UNK_10
    Player_SpawnWalkingSlow,             // PLAYER_SPAWNMODE_UNK_11
    Player_SpawnWalkingSlow,             // PLAYER_SPAWNMODE_UNK_12
    Player_SpawnNoMomentum,              // PLAYER_SPAWNMODE_NO_MOMENTUM
    Player_SpawnWalkingSlow,             // PLAYER_SPAWNMODE_WALKING_SLOW
    Player_SpawnWalkingPreserveMomentum, // PLAYER_SPAWNMODE_WALKING_PRESERVE_MOMENTUM
};

static Vec3f sNaviPosOffset = { 0.0f, 50.0f, 0.0f };

void Player_Init(Actor* thisx, PlayState* play2) {
    Player* this = (Player*)thisx;
    PlayState* play = play2;
    SceneTableEntry* scene = play->loadedScene;
    u32 titleFileSize;
    s32 spawnMode;
    s32 respawnFlag;
    s32 respawnMode;

    play->shootingGalleryStatus = play->bombchuBowlingStatus = 0;

    play->playerInit = Player_InitCommon;
    play->playerUpdate = Player_UpdateCommon;
    play->isPlayerDroppingFish = Player_IsDroppingFish;
    play->startPlayerFishing = Player_StartFishing;
    play->grabPlayer = Player_SetupRestrainedByEnemy;
    play->startPlayerCutscene = Player_SetupPlayerCutscene;
    play->func_11D54 = Player_SetupStandingStillMorph;
    play->damagePlayer = Player_InflictDamageAndCheckForDeath;
    play->talkWithPlayer = Player_StartTalkingWithActor;

    thisx->room = -1;
    this->ageProperties = &sAgeProperties[gSaveContext.linkAge];
    this->itemAction = this->heldItemAction = -1;
    this->heldItemId = ITEM_NONE;

    Player_UseItem(play, this, ITEM_NONE);
    Player_SetEquipmentData(play, this);
    this->prevBoots = this->currentBoots;
    Player_InitCommon(this, play, gPlayerSkelHeaders[((void)0, gSaveContext.linkAge)]);
    this->giObjectSegment = (void*)(((uintptr_t)ZeldaArena_MallocDebug(0x3008, "../z_player.c", 17175) + 8) & ~0xF);

    respawnFlag = gSaveContext.respawnFlag;

    if (respawnFlag != 0) {
        if (respawnFlag == -3) {
            thisx->params = gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams;
        } else {
            if ((respawnFlag == 1) || (respawnFlag == -1)) {
                this->voidRespawnCounter = -2;
            }

            if (respawnFlag < 0) {
                respawnMode = RESPAWN_MODE_DOWN;
            } else {
                respawnMode = respawnFlag - 1;
                Math_Vec3f_Copy(&thisx->world.pos, &gSaveContext.respawn[respawnMode].pos);
                Math_Vec3f_Copy(&thisx->home.pos, &thisx->world.pos);
                Math_Vec3f_Copy(&thisx->prevPos, &thisx->world.pos);
                this->fallStartHeight = thisx->world.pos.y;
                this->currentYaw = thisx->shape.rot.y = gSaveContext.respawn[respawnMode].yaw;
                thisx->params = gSaveContext.respawn[respawnMode].playerParams;
            }

            play->actorCtx.flags.tempSwch = gSaveContext.respawn[respawnMode].tempSwchFlags & 0xFFFFFF;
            play->actorCtx.flags.tempCollect = gSaveContext.respawn[respawnMode].tempCollectFlags;
        }
    }

    if ((respawnFlag == 0) || (respawnFlag < -1)) {
        titleFileSize = scene->titleFile.vromEnd - scene->titleFile.vromStart;
        if ((titleFileSize != 0) && gSaveContext.showTitleCard) {
            if (!IS_CUTSCENE_LAYER &&
                (gEntranceTable[((void)0, gSaveContext.entranceIndex) + ((void)0, gSaveContext.sceneLayer)].field &
                 ENTRANCE_INFO_DISPLAY_TITLE_CARD_FLAG) &&
                ((play->sceneId != SCENE_DODONGOS_CAVERN) || GET_EVENTCHKINF(EVENTCHKINF_B0)) &&
                ((play->sceneId != SCENE_BOMBCHU_SHOP) || GET_EVENTCHKINF(EVENTCHKINF_25))) {
                TitleCard_InitPlaceName(play, &play->actorCtx.titleCtx, this->giObjectSegment, 160, 120, 144, 24, 20);
            }
        }
        gSaveContext.showTitleCard = true;
    }

    if (func_80845C68(play, (respawnFlag == 2) ? 1 : 0) == 0) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = (thisx->params & 0xFF) | 0xD00;
    }

    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 1;

    if (play->sceneId <= SCENE_INSIDE_GANONS_CASTLE_COLLAPSE) {
        gSaveContext.infTable[INFTABLE_1AX_INDEX] |= gBitFlags[play->sceneId];
    }

    spawnMode = (thisx->params & 0xF00) >> 8;
    if ((spawnMode == PLAYER_SPAWNMODE_FROM_WARP_SONG) || (spawnMode == PLAYER_SPAWNMODE_FROM_FARORES_WIND)) {
        if (gSaveContext.cutsceneIndex >= 0xFFF0) {
            spawnMode = PLAYER_SPAWNMODE_NO_MOMENTUM;
        }
    }

    sSpawnFuncs[spawnMode](play, this);

    if (spawnMode != PLAYER_SPAWNMODE_NO_UPDATE_OR_DRAW) {
        if ((gSaveContext.gameMode == GAMEMODE_NORMAL) || (gSaveContext.gameMode == GAMEMODE_END_CREDITS)) {
            this->naviActor = Player_SpawnFairy(play, this, &thisx->world.pos, &sNaviPosOffset, FAIRY_NAVI);
            if (gSaveContext.dogParams != 0) {
                gSaveContext.dogParams |= 0x8000;
            }
        }
    }

    if (gSaveContext.nayrusLoveTimer != 0) {
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        Player_SpawnMagicSpellActor(play, this, 1);
        this->stateFlags3 &= ~PLAYER_STATE3_RESTORE_NAYRUS_LOVE;
    }

    if (gSaveContext.entranceSound != 0) {
        Audio_PlayActorSfx2(&this->actor, ((void)0, gSaveContext.entranceSound));
        gSaveContext.entranceSound = 0;
    }

    Map_SavePlayerInitialInfo(play);
    MREG(64) = 0;
}

void Player_StepValueToZero(s16* pValue) {
    s16 step;

    step = (ABS(*pValue) * 100.0f) / 1000.0f;
    step = CLAMP(step, 400, 4000);

    Math_ScaledStepToS(pValue, 0, step);
}

void Player_StepLookToZero(Player* this) {
    s16 focusYawDiff;

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y)) {
        focusYawDiff = this->actor.focus.rot.y - this->actor.shape.rot.y;
        Player_StepValueToZero(&focusYawDiff);
        this->actor.focus.rot.y = this->actor.shape.rot.y + focusYawDiff;
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X)) {
        Player_StepValueToZero(&this->actor.focus.rot.x);
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_X)) {
        Player_StepValueToZero(&this->headRot.x);
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X)) {
        Player_StepValueToZero(&this->upperBodyRot.x);
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Z)) {
        Player_StepValueToZero(&this->actor.focus.rot.z);
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Y)) {
        Player_StepValueToZero(&this->headRot.y);
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Z)) {
        Player_StepValueToZero(&this->headRot.z);
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y)) {
        if (this->upperBodyYawOffset != 0) {
            Player_StepValueToZero(&this->upperBodyYawOffset);
        } else {
            Player_StepValueToZero(&this->upperBodyRot.y);
        }
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Z)) {
        Player_StepValueToZero(&this->upperBodyRot.z);
    }

    this->lookFlags = 0;
}

static f32 sScaleDiveDists[] = { 120.0f, 240.0f, 360.0f };

static u8 sDiveDoActions[] = { DO_ACTION_1, DO_ACTION_2, DO_ACTION_3, DO_ACTION_4,
                               DO_ACTION_5, DO_ACTION_6, DO_ACTION_7, DO_ACTION_8 };

void Player_SetupDoActionText(PlayState* play, Player* this) {
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) && (this->actor.category == ACTORCAT_PLAYER)) {
        Actor* heldActor = this->heldActor;
        Actor* interactRangeActor = this->interactRangeActor;
        s32 diveTimerNum;
        s32 relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];
        s32 isSwimming = Player_IsSwimming(this);
        s32 doAction = DO_ACTION_NONE;

        if (!Player_InBlockingCsMode(play, this)) {
            if (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE) {
                doAction = DO_ACTION_RETURN;
            } else if ((this->heldItemAction == PLAYER_IA_FISHING_POLE) && (this->fishingState != 0)) {
                if (this->fishingState == 2) {
                    doAction = DO_ACTION_REEL;
                }
            } else if ((Player_PlayOcarina != this->actionFunc) && !(this->stateFlags2 & PLAYER_STATE2_CRAWLING)) {
                if ((this->doorType != PLAYER_DOORTYPE_NONE) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                     ((heldActor != NULL) && (heldActor->id == ACTOR_EN_RU1)))) {
                    doAction = DO_ACTION_OPEN;
                } else if ((!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) || (heldActor == NULL)) &&
                           (interactRangeActor != NULL) &&
                           ((!isSwimming && (this->getItemId == GI_NONE)) ||
                            ((this->getItemId < 0) && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING)))) {
                    if (this->getItemId < 0) {
                        doAction = DO_ACTION_OPEN;
                    } else if ((interactRangeActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                        doAction = DO_ACTION_DROP;
                    } else {
                        doAction = DO_ACTION_GRAB;
                    }
                } else if (!isSwimming && (this->stateFlags2 & PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL)) {
                    doAction = DO_ACTION_GRAB;
                } else if ((this->stateFlags2 & PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL) ||
                           (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->rideActor != NULL))) {
                    doAction = DO_ACTION_CLIMB;
                } else if ((this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
                           !EN_HORSE_CHECK_4((EnHorse*)this->rideActor) && (Player_DismountHorse != this->actionFunc)) {
                    if ((this->stateFlags2 & PLAYER_STATE2_CAN_SPEAK_OR_CHECK) && (this->talkActor != NULL)) {
                        if (this->talkActor->category == ACTORCAT_NPC) {
                            doAction = DO_ACTION_SPEAK;
                        } else {
                            doAction = DO_ACTION_CHECK;
                        }
                    } else if (!Actor_PlayerIsAimingReadyFpsItem(this) &&
                               !(this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
                        doAction = DO_ACTION_FASTER;
                    }
                } else if ((this->stateFlags2 & PLAYER_STATE2_CAN_SPEAK_OR_CHECK) && (this->talkActor != NULL)) {
                    if (this->talkActor->category == ACTORCAT_NPC) {
                        doAction = DO_ACTION_SPEAK;
                    } else {
                        doAction = DO_ACTION_CHECK;
                    }
                } else if ((this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING)) ||
                           ((this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
                            (this->stateFlags2 & PLAYER_STATE2_CAN_DISMOUNT_HORSE))) {
                    doAction = DO_ACTION_DOWN;
                } else if (this->stateFlags2 & PLAYER_STATE2_DO_ACTION_ENTER) {
                    doAction = DO_ACTION_ENTER;
                } else if ((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->getItemId == GI_NONE) &&
                           (heldActor != NULL)) {
                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (heldActor->id == ACTOR_EN_NIW)) {
                        if (Player_CanThrowActor(this, heldActor) == 0) {
                            doAction = DO_ACTION_DROP;
                        } else {
                            doAction = DO_ACTION_THROW;
                        }
                    }
                } else if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) && Player_CanHoldActor(this) &&
                           (this->getItemId < GI_MAX)) {
                    doAction = DO_ACTION_GRAB;
                } else if (this->stateFlags2 & PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER) {
                    diveTimerNum = (sScaleDiveDists[CUR_UPG_VALUE(UPG_SCALE)] - this->actor.yDistToWater) / 40.0f;
                    diveTimerNum = CLAMP(diveTimerNum, 0, 7);
                    doAction = sDiveDoActions[diveTimerNum];
                } else if (isSwimming && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
                    doAction = DO_ACTION_DIVE;
                } else if (!isSwimming && (!(this->stateFlags1 & PLAYER_STATE1_22) || Player_IsZTargeting(this) ||
                                           !Player_IsChildWithHylianShield(this))) {
                    if ((!(this->stateFlags1 & PLAYER_STATE1_CLIMBING_ONTO_LEDGE) &&
                         (relativeStickInput <= PLAYER_RELATIVESTICKINPUT_FORWARD) &&
                         (Player_IsUnfriendlyZTargeting(this) ||
                          ((sFloorType != FLOOR_TYPE_QUICKSAND_NO_HORSE) &&
                           (Player_IsFriendlyZTargeting(this) ||
                            ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
                             !(this->stateFlags1 & PLAYER_STATE1_22) &&
                             (relativeStickInput == PLAYER_RELATIVESTICKINPUT_FORWARD))))))) {
                        doAction = DO_ACTION_ATTACK;
                    } else if ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
                               Player_IsZTargeting(this) && (relativeStickInput > PLAYER_RELATIVESTICKINPUT_FORWARD)) {
                        doAction = DO_ACTION_JUMP;
                    } else if ((this->heldItemAction >= PLAYER_IA_SWORD_MASTER) ||
                               ((this->stateFlags2 & PLAYER_STATE2_NAVI_IS_ACTIVE) &&
                                (play->actorCtx.targetCtx.arrowPointedActor == NULL))) {
                        doAction = DO_ACTION_PUTAWAY;
                    }
                }
            }
        }

        if (doAction != DO_ACTION_PUTAWAY) {
            this->putAwayTimer = 20;
        } else if (this->putAwayTimer != 0) {
            doAction = DO_ACTION_NONE;
            this->putAwayTimer--;
        }

        Interface_SetDoAction(play, doAction);

        if (this->stateFlags2 & PLAYER_STATE2_NAVI_REQUESTING_TALK) {
            if (this->targetActor != NULL) {
                Interface_SetNaviCall(play, 0x1E);
            } else {
                Interface_SetNaviCall(play, 0x1D);
            }
            Interface_SetNaviCall(play, 0x1E);
        } else {
            Interface_SetNaviCall(play, 0x1F);
        }
    }
}

s32 Player_SetupHover(Player* this) {
    s32 hoverActive;

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && (this->hoverBootsTimer != 0)) {
        this->hoverBootsTimer--;
    } else {
        this->hoverBootsTimer = 0;
    }

    hoverActive =
        (this->currentBoots == PLAYER_BOOTS_HOVER) &&
        ((this->actor.yDistToWater >= 0.0f) || (Player_GetHurtFloorType(sFloorType) >= PLAYER_HURTFLOORTYPE_DEFAULT) ||
         Player_IsFloorSinkingSand(sFloorType));

    if (hoverActive && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->hoverBootsTimer != 0)) {
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        if (!hoverActive) {
            this->hoverBootsTimer = 19;
        }
        return false;
    }

    sFloorType = FLOOR_TYPE_NONE;
    this->angleToFloorX = this->angleToFloorY = sAngleToFloorX = 0;

    return true;
}

static Vec3f sWallCheckOffset = { 0.0f, 18.0f, 0.0f };

void Player_UpdateBgcheck(PlayState* play, Player* this) {
    u8 wallJumpType = 0;
    CollisionPoly* floorPoly;
    Vec3f playerPos;
    f32 wallCheckRadius;
    f32 wallCheckHeight;
    f32 ceilingCheckHeight;
    u32 updBgcheckFlags;

    sFloorProperty = this->floorProperty;

    if (this->stateFlags2 & PLAYER_STATE2_CRAWLING) {
        wallCheckRadius = 10.0f;
        wallCheckHeight = 15.0f;
        ceilingCheckHeight = 30.0f;
    } else {
        wallCheckRadius = this->ageProperties->wallCheckRadius;
        wallCheckHeight = 26.0f;
        ceilingCheckHeight = this->ageProperties->ceilingCheckHeight;
    }

    if (this->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID)) {
        if (this->stateFlags1 & PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID) {
            this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
            updBgcheckFlags = UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if ((this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                   ((this->sceneExitPosY - (s32)this->actor.world.pos.y) >= 100)) {
            updBgcheckFlags =
                UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                   ((Player_OpenDoor == this->actionFunc) || (Player_MiniCsMovement == this->actionFunc))) {
            this->actor.bgCheckFlags &= ~(BGCHECKFLAG_WALL | BGCHECKFLAG_PLAYER_WALL_INTERACT);
            updBgcheckFlags =
                UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else {
            updBgcheckFlags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 |
                              UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        }
    } else {
        updBgcheckFlags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 |
                          UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
    }

    if (this->stateFlags3 & PLAYER_STATE3_IGNORE_CEILING_FLOOR_AND_WATER) {
        updBgcheckFlags &= ~(UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
    }

    if (updBgcheckFlags & UPDBGCHECKINFO_FLAG_2) {
        this->stateFlags3 |= PLAYER_STATE3_CHECKING_FLOOR_AND_WATER_COLLISION;
    }

    Math_Vec3f_Copy(&playerPos, &this->actor.world.pos);
    Actor_UpdateBgCheckInfo(play, &this->actor, wallCheckHeight, wallCheckRadius, ceilingCheckHeight, updBgcheckFlags);

    if (this->actor.bgCheckFlags & BGCHECKFLAG_CEILING) {
        this->actor.velocity.y = 0.0f;
    }

    sPlayerYDistToFloor = this->actor.world.pos.y - this->actor.floorHeight;
    sConveyorSpeedIndex = 0;

    floorPoly = this->actor.floorPoly;

    if (floorPoly != NULL) {
        this->floorProperty = SurfaceType_GetFloorProperty(&play->colCtx, floorPoly, this->actor.floorBgId);
        this->prevFloorSfxOffset = this->floorSfxOffset;

        if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
            if (this->actor.yDistToWater < 20.0f) {
                this->floorSfxOffset = SURFACE_SFX_OFFSET_WATER_SHALLOW;
            } else {
                this->floorSfxOffset = SURFACE_SFX_OFFSET_WATER_DEEP;
            }
        } else {
            if (this->stateFlags2 & PLAYER_STATE2_SPAWN_DUST_AT_FEET) {
                this->floorSfxOffset = SURFACE_SFX_OFFSET_SAND;
            } else {
                this->floorSfxOffset = SurfaceType_GetSfxOffset(&play->colCtx, floorPoly, this->actor.floorBgId);
            }
        }

        if (this->actor.category == ACTORCAT_PLAYER) {
            Audio_SetCodeReverb(SurfaceType_GetEcho(&play->colCtx, floorPoly, this->actor.floorBgId));

            if (this->actor.floorBgId == BGCHECK_SCENE) {
                Environment_ChangeLightSetting(
                    play, SurfaceType_GetLightSetting(&play->colCtx, floorPoly, this->actor.floorBgId));
            } else {
                DynaPoly_SetPlayerAbove(&play->colCtx, this->actor.floorBgId);
            }
        }

        // This block extracts the conveyor properties from the floor poly
        sConveyorSpeedIndex = SurfaceType_GetConveyorSpeed(&play->colCtx, floorPoly, this->actor.floorBgId);
        if (sConveyorSpeedIndex != CONVEYOR_SPEED_DISABLED) {
            sIsFloorConveyor = SurfaceType_IsFloorConveyor(&play->colCtx, floorPoly, this->actor.floorBgId);
            if ((!sIsFloorConveyor && (this->actor.yDistToWater > 20.0f) &&
                 (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                (sIsFloorConveyor && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {
                sConveyorYaw = CONVEYOR_DIRECTION_TO_BINANG(
                    SurfaceType_GetConveyorDirection(&play->colCtx, floorPoly, this->actor.floorBgId));
            } else {
                sConveyorSpeedIndex = CONVEYOR_SPEED_DISABLED;
            }
        }
    }

    Player_SetupExit(play, this, floorPoly, this->actor.floorBgId);

    this->actor.bgCheckFlags &= ~BGCHECKFLAG_PLAYER_WALL_INTERACT;

    if (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) {
        CollisionPoly* wallPoly;
        s32 wallBgId;
        s16 wallYawDiff;
        s32 pad;

        sWallCheckOffset.y = 18.0f;
        sWallCheckOffset.z = this->ageProperties->wallCheckRadius + 10.0f;

        if (!(this->stateFlags2 & PLAYER_STATE2_CRAWLING) &&
            Player_WallLineTestWithOffset(play, this, &sWallCheckOffset, &wallPoly, &wallBgId, &sWallIntersectPos)) {
            this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
            if (this->actor.wallPoly != wallPoly) {
                this->actor.wallPoly = wallPoly;
                this->actor.wallBgId = wallBgId;
                this->actor.wallYaw = Math_Atan2S(wallPoly->normal.z, wallPoly->normal.x);
            }
        }

        wallYawDiff = this->actor.shape.rot.y - (s16)(this->actor.wallYaw + 0x8000);

        sInteractWallFlags = SurfaceType_GetWallFlags(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId);

        sYawToTouchedWall = ABS(wallYawDiff);

        wallYawDiff = this->currentYaw - (s16)(this->actor.wallYaw + 0x8000);

        sYawToTouchedWall2 = ABS(wallYawDiff);

        wallCheckRadius = sYawToTouchedWall2 * 0.00008f;
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || wallCheckRadius >= 1.0f) {
            this->speedLimit = R_RUN_SPEED_LIMIT / 100.0f;
        } else {
            wallCheckHeight = (R_RUN_SPEED_LIMIT / 100.0f * wallCheckRadius);
            this->speedLimit = wallCheckHeight;
            if (wallCheckHeight < 0.1f) {
                this->speedLimit = 0.1f;
            }
        }

        // Check if player can climb/jump onto a ledge above the wall
        if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
            (sYawToTouchedWall < DEG_TO_BINANG(67.5f))) {
            CollisionPoly* wallPoly = this->actor.wallPoly;

            if (ABS(wallPoly->normal.y) < 600) {
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalY = COLPOLY_GET_NORMAL(wallPoly->normal.y);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 wallHeight;
                CollisionPoly* floorPoly2;
                CollisionPoly* wallPoly2;
                s32 wallBgId;
                Vec3f checkPos;
                f32 ledgePosY;
                f32 ceilingPosY;
                s32 yawDiff;

                this->wallDistance = Math3D_UDistPlaneToPos(wallPolyNormalX, wallPolyNormalY, wallPolyNormalZ,
                                                            wallPoly->dist, &this->actor.world.pos);

                wallCheckRadius = this->wallDistance + 10.0f;
                checkPos.x = this->actor.world.pos.x - (wallCheckRadius * wallPolyNormalX);
                checkPos.z = this->actor.world.pos.z - (wallCheckRadius * wallPolyNormalZ);
                checkPos.y = this->actor.world.pos.y + this->ageProperties->unk_0C;

                ledgePosY = BgCheck_EntityRaycastDown1(&play->colCtx, &floorPoly2, &checkPos);
                wallHeight = ledgePosY - this->actor.world.pos.y;
                this->wallHeight = wallHeight;

                if ((this->wallHeight < 18.0f) ||
                    BgCheck_EntityCheckCeiling(&play->colCtx, &ceilingPosY, &this->actor.world.pos,
                                               (ledgePosY - this->actor.world.pos.y) + 20.0f, &wallPoly2, &wallBgId,
                                               &this->actor)) {
                    this->wallHeight = 399.96002f;
                } else {
                    sWallCheckOffset.y = (ledgePosY + 5.0f) - this->actor.world.pos.y;

                    if (Player_WallLineTestWithOffset(play, this, &sWallCheckOffset, &wallPoly2, &wallBgId,
                                                      &sWallIntersectPos) &&
                        (yawDiff = this->actor.wallYaw - Math_Atan2S(wallPoly2->normal.z, wallPoly2->normal.x),
                         ABS(yawDiff) < DEG_TO_BINANG(90.0f)) &&
                        !SurfaceType_CheckWallFlag1(&play->colCtx, wallPoly2, wallBgId)) {
                        this->wallHeight = 399.96002f;
                    } else if (SurfaceType_CheckWallFlag0(&play->colCtx, wallPoly, this->actor.wallBgId) == 0) {
                        if (this->ageProperties->unk_1C <= this->wallHeight) {
                            if (ABS(floorPoly2->normal.y) > 28000) {
                                if (this->ageProperties->unk_14 <= this->wallHeight) {
                                    wallJumpType = PLAYER_WALLJUMPTYPE_JUMP_UP_TO_LEDGE;
                                } else if (this->ageProperties->unk_18 <= this->wallHeight) {
                                    wallJumpType = PLAYER_WALLJUMPTYPE_LARGE_CLIMB_UP;
                                } else {
                                    wallJumpType = PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP;
                                }
                            }
                        } else {
                            wallJumpType = PLAYER_WALLJUMPTYPE_HOP_UP;
                        }
                    }
                }
            }
        }
    } else {
        this->speedLimit = R_RUN_SPEED_LIMIT / 100.0f;
        this->wallTouchTimer = 0;
        this->wallHeight = 0.0f;
    }

    if (wallJumpType == this->touchedWallJumpType) {
        if ((this->linearVelocity != 0.0f) && (this->wallTouchTimer < 100)) {
            this->wallTouchTimer++;
        }
    } else {
        this->touchedWallJumpType = wallJumpType;
        this->wallTouchTimer = 0;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        sFloorType = SurfaceType_GetFloorType(&play->colCtx, floorPoly, this->actor.floorBgId);

        if (!Player_SetupHover(this)) {
            f32 floorPolyNormalX;
            f32 invFloorPolyNormalY;
            f32 floorPolyNormalZ;
            f32 yawSin;
            s32 pad2;
            f32 yawCos;
            s32 pad3;

            if (this->actor.floorBgId != BGCHECK_SCENE) {
                DynaPoly_SetPlayerOnTop(&play->colCtx, this->actor.floorBgId);
            }

            floorPolyNormalX = COLPOLY_GET_NORMAL(floorPoly->normal.x);
            invFloorPolyNormalY = 1.0f / COLPOLY_GET_NORMAL(floorPoly->normal.y);
            floorPolyNormalZ = COLPOLY_GET_NORMAL(floorPoly->normal.z);

            yawSin = Math_SinS(this->currentYaw);
            yawCos = Math_CosS(this->currentYaw);

            this->angleToFloorX =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * yawSin) - (floorPolyNormalZ * yawCos)) * invFloorPolyNormalY);
            this->angleToFloorY =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * yawCos) - (floorPolyNormalZ * yawSin)) * invFloorPolyNormalY);

            yawSin = Math_SinS(this->actor.shape.rot.y);
            yawCos = Math_CosS(this->actor.shape.rot.y);

            sAngleToFloorX =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * yawSin) - (floorPolyNormalZ * yawCos)) * invFloorPolyNormalY);

            Player_WalkOnSlope(play, this, floorPoly);
        }
    } else {
        Player_SetupHover(this);
    }

    if (this->prevFloorSpecialProperty == sFloorType) {
        this->hurtFloorTimer++;
    } else {
        this->prevFloorSpecialProperty = sFloorType;
        this->hurtFloorTimer = 0;
    }
}

void Player_UpdateCamAndSeqModes(PlayState* play, Player* this) {
    u8 seqMode;
    s32 pad;
    Actor* targetActor;
    s32 camMode;

    if (this->actor.category == ACTORCAT_PLAYER) {
        seqMode = SEQ_MODE_DEFAULT;

        if (this->csMode != PLAYER_CSMODE_NONE) {
            Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_NORMAL);
        } else if (!(this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
            if ((this->actor.parent != NULL) && (this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
                camMode = CAM_MODE_HOOKSHOT;
                Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->actor.parent);
            } else if (Player_StartKnockback == this->actionFunc) {
                camMode = CAM_MODE_STILL;
            } else if (this->stateFlags2 & PLAYER_STATE2_ENABLE_PUSH_PULL_CAM) {
                camMode = CAM_MODE_PUSHPULL;
            } else if ((targetActor = this->targetActor) != NULL) {
                if (CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_8)) {
                    camMode = CAM_MODE_TALK;
                } else if (this->stateFlags1 & PLAYER_STATE1_FORCE_STRAFING) {
                    if (this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) {
                        camMode = CAM_MODE_FOLLOWBOOMERANG;
                    } else {
                        camMode = CAM_MODE_FOLLOWTARGET;
                    }
                } else {
                    camMode = CAM_MODE_BATTLE;
                }
                Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, targetActor);
            } else if (this->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK) {
                camMode = CAM_MODE_CHARGE;
            } else if (this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) {
                camMode = CAM_MODE_FOLLOWBOOMERANG;
                Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->boomerangActor);
            } else if (this->stateFlags1 &
                       (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE)) {
                if (Player_IsFriendlyZTargeting(this)) {
                    camMode = CAM_MODE_HANGZ;
                } else {
                    camMode = CAM_MODE_HANG;
                }
            } else if (this->stateFlags1 & (PLAYER_STATE1_Z_TARGETING_FRIENDLY | PLAYER_STATE1_30)) {
                if (Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) {
                    camMode = CAM_MODE_BOWARROWZ;
                } else if (this->stateFlags1 & PLAYER_STATE1_CLIMBING) {
                    camMode = CAM_MODE_CLIMBZ;
                } else {
                    camMode = CAM_MODE_TARGET;
                }
            } else if (this->stateFlags1 & (PLAYER_STATE1_JUMPING | PLAYER_STATE1_CLIMBING)) {
                if ((Player_JumpUpToLedge == this->actionFunc) || (this->stateFlags1 & PLAYER_STATE1_CLIMBING)) {
                    camMode = CAM_MODE_CLIMB;
                } else {
                    camMode = CAM_MODE_JUMP;
                }
            } else if (this->stateFlags1 & PLAYER_STATE1_FREEFALLING) {
                camMode = CAM_MODE_FREEFALL;
            } else if ((this->isMeleeWeaponAttacking != 0) &&
                       (this->meleeAttackType >= PLAYER_MELEEATKTYPE_FORWARD_SLASH_1H) &&
                       (this->meleeAttackType < PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H)) {
                camMode = CAM_MODE_STILL;
            } else {
                camMode = CAM_MODE_NORMAL;
                if ((this->linearVelocity == 0.0f) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) || (this->rideActor->speedXZ == 0.0f))) {
                    // not moving
                    seqMode = SEQ_MODE_STILL;
                }
            }

            Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), camMode);
        } else {
            // First person mode
            seqMode = SEQ_MODE_STILL;
        }

        if (play->actorCtx.targetCtx.bgmEnemy != NULL) {
            seqMode = SEQ_MODE_ENEMY;
            Audio_SetBgmEnemyVolume(sqrtf(play->actorCtx.targetCtx.bgmEnemy->xyzDistToPlayerSq));
        }

        if (play->sceneId != SCENE_FISHING_POND) {
            Audio_SetSequenceMode(seqMode);
        }
    }
}

static Vec3f sStickFlameVelocity = { 0.0f, 0.5f, 0.0f };
static Vec3f sStickFlameAccel = { 0.0f, 0.5f, 0.0f };

static Color_RGBA8 sStickFlamePrimColor = { 255, 255, 100, 255 };
static Color_RGBA8 sStickFlameEnvColor = { 255, 50, 0, 0 };

void Player_UpdateDekuStick(PlayState* play, Player* this) {
    f32 newDekuStickLength;

    if (this->dekuStickLength == 0.0f) {
        Player_UseItem(play, this, ITEM_NONE);
        return;
    }

    newDekuStickLength = 1.0f;
    if (DECR(this->stickFlameTimer) == 0) {
        Inventory_ChangeAmmo(ITEM_DEKU_STICK, -1);
        this->stickFlameTimer = 1;
        newDekuStickLength = 0.0f;
        this->dekuStickLength = newDekuStickLength;
    } else if (this->stickFlameTimer > 200) {
        newDekuStickLength = (210 - this->stickFlameTimer) / 10.0f;
    } else if (this->stickFlameTimer < 20) {
        newDekuStickLength = this->stickFlameTimer / 20.0f;
        this->dekuStickLength = newDekuStickLength;
    }

    // Spawn flame effect
    func_8002836C(play, &this->meleeWeaponInfo[0].tip, &sStickFlameVelocity, &sStickFlameAccel, &sStickFlamePrimColor,
                  &sStickFlameEnvColor, newDekuStickLength * 200.0f, 0, 8);
}

void Player_ElectricShock(PlayState* play, Player* this) {
    Vec3f shockPos;
    Vec3f* randBodyPart;
    s32 shockScale;

    this->shockTimer--;
    this->unk_892 += this->shockTimer;

    if (this->unk_892 > 20) {
        shockScale = this->shockTimer * 2;
        this->unk_892 -= 20;

        if (shockScale > 40) {
            shockScale = 40;
        }

        randBodyPart = this->bodyPartsPos + (s32)Rand_ZeroFloat(PLAYER_BODYPART_MAX - 0.1f);
        shockPos.x = (Rand_CenteredFloat(5.0f) + randBodyPart->x) - this->actor.world.pos.x;
        shockPos.y = (Rand_CenteredFloat(5.0f) + randBodyPart->y) - this->actor.world.pos.y;
        shockPos.z = (Rand_CenteredFloat(5.0f) + randBodyPart->z) - this->actor.world.pos.z;

        EffectSsFhgFlash_SpawnShock(play, &this->actor, &shockPos, shockScale, FHGFLASH_SHOCK_PLAYER);
        func_8002F8F0(&this->actor, NA_SE_PL_SPARK - SFX_FLAG);
    }
}

void Player_Burning(PlayState* play, Player* this) {
    s32 spawnedFlame;
    u8* timerPtr;
    s32 timerStep;
    f32 flameScale;
    f32 flameIntensity;
    s32 dmgCooldown;
    s32 i;
    s32 timerExtraStep;
    s32 timerBaseStep;

    if (this->currentTunic == PLAYER_TUNIC_GORON) {
        timerBaseStep = 20;
    } else {
        timerBaseStep = (s32)(this->linearVelocity * 0.4f) + 1;
    }

    spawnedFlame = false;
    timerPtr = this->flameTimers;

    if (this->stateFlags2 & PLAYER_STATE2_MAKING_REACTABLE_NOISE) {
        timerExtraStep = 100;
    } else {
        timerExtraStep = 0;
    }

    Player_BurnDekuShield(this, play);

    for (i = 0; i < PLAYER_BODYPART_MAX; i++, timerPtr++) {
        timerStep = timerExtraStep + timerBaseStep;

        if (*timerPtr <= timerStep) {
            *timerPtr = 0;
        } else {
            spawnedFlame = true;
            *timerPtr -= timerStep;

            if (*timerPtr > 20.0f) {
                flameIntensity = (*timerPtr - 20.0f) * 0.01f;
                flameScale = CLAMP(flameIntensity, 0.19999999f, 0.2f);
            } else {
                flameScale = *timerPtr * 0.01f;
            }

            flameIntensity = (*timerPtr - 25.0f) * 0.02f;
            flameIntensity = CLAMP(flameIntensity, 0.0f, 1.0f);
            EffectSsFireTail_SpawnFlameOnPlayer(play, flameScale, i, flameIntensity);
        }
    }

    if (spawnedFlame) {
        func_8002F7DC(&this->actor, NA_SE_EV_TORCH - SFX_FLAG);

        if (play->sceneId == SCENE_SPIRIT_TEMPLE_BOSS) {
            dmgCooldown = 0;
        } else {
            dmgCooldown = 7;
        }

        if ((dmgCooldown & play->gameplayFrames) == 0) {
            Player_InflictDamageAndCheckForDeath(play, -1);
        }
    } else {
        this->isBurning = false;
    }
}

void Player_SetupStoneOfAgonyRumble(Player* this) {
    if (CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY)) {
        f32 timerStep = 200000.0f - (this->stoneOfAgonyActorDistSq * 5.0f);

        if (timerStep < 0.0f) {
            timerStep = 0.0f;
        }

        this->stoneOfAgonyRumbleTimer += timerStep;
        if (this->stoneOfAgonyRumbleTimer > 4000000.0f) {
            this->stoneOfAgonyRumbleTimer = 0.0f;
            Player_RequestRumble(this, 120, 20, 10, 0);
        }
    }
}

static s8 slinkCsActions[] = {
    0,  3,  3,  5,   4,   8,   9,   13, 14, 15, 16, 17, 18, -22, 23, 24, 25,  26, 27,  28,  29, 31, 32, 33, 34, -35,
    30, 36, 38, -39, -40, -41, 42,  43, 45, 46, 0,  0,  0,  67,  48, 47, -50, 51, -52, -53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63,  64,  -65, -66, 68, 11, 69, 70, 71, 8,  8,   72, 73, 78,  79, 80,  89,  90, 91, 92, 77, 19, 94,
};

static Vec3f sHorseRaycastOffset = { 0.0f, 0.0f, 200.0f };

static f32 sWaterConveyorSpeeds[CONVEYOR_SPEED_MAX - 1] = {
    2.0f, // CONVEYOR_SPEED_SLOW
    4.0f, // CONVEYOR_SPEED_MEDIUM
    7.0f, // CONVEYOR_SPEED_FAST
};
static f32 sFloorConveyorSpeeds[CONVEYOR_SPEED_MAX - 1] = {
    0.5f, // CONVEYOR_SPEED_SLOW
    1.0f, // CONVEYOR_SPEED_MEDIUM
    3.0f, // CONVEYOR_SPEED_FAST
};

void Player_UpdateCommon(Player* this, PlayState* play, Input* input) {
    s32 pad;

    sControlInput = input;

    if (this->voidRespawnCounter < 0) {
        this->voidRespawnCounter++;
        if (this->voidRespawnCounter == 0) {
            this->voidRespawnCounter = 1;
            func_80078884(NA_SE_OC_REVENGE);
        }
    }

    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.home.pos);

    if (this->fpsItemShotTimer != 0) {
        this->fpsItemShotTimer--;
    }

    if (this->endTalkTimer != 0) {
        this->endTalkTimer--;
    }

    if (this->deathTimer != 0) {
        this->deathTimer--;
    }

    if (this->invincibilityTimer < 0) {
        this->invincibilityTimer++;
    } else if (this->invincibilityTimer > 0) {
        this->invincibilityTimer--;
    }

    if (this->runDamageTimer != 0) {
        this->runDamageTimer--;
    }

    Player_SetupDoActionText(play, this);
    Player_SetupZTargeting(this, play);

    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) && (this->stickFlameTimer != 0)) {
        Player_UpdateDekuStick(play, this);
    } else if ((this->heldItemAction == PLAYER_IA_FISHING_POLE) && (this->fishingState < 0)) {
        this->fishingState++;
    }

    if (this->shockTimer != 0) {
        Player_ElectricShock(play, this);
    }

    if (this->isBurning) {
        Player_Burning(play, this);
    }

    if ((this->stateFlags3 & PLAYER_STATE3_RESTORE_NAYRUS_LOVE) && (gSaveContext.nayrusLoveTimer != 0) &&
        (gSaveContext.magicState == MAGIC_STATE_IDLE)) {
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        Player_SpawnMagicSpellActor(play, this, 1);
        this->stateFlags3 &= ~PLAYER_STATE3_RESTORE_NAYRUS_LOVE;
    }

    if (this->stateFlags2 & PLAYER_STATE2_PAUSE_MOST_UPDATING) {
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            Player_StopMovement(this);
            Actor_MoveForward(&this->actor);
        }

        Player_UpdateBgcheck(play, this);
    } else {
        f32 temp_f0;
        f32 phi_f12;

        if (this->currentBoots != this->prevBoots) {
            if (this->currentBoots == PLAYER_BOOTS_IRON) {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    Player_ResetSubCam(play, this);
                    if (this->ageProperties->diveWaterSurface < this->actor.yDistToWater) {
                        this->stateFlags2 |= PLAYER_STATE2_DIVING;
                    }
                }
            } else {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    if ((this->prevBoots == PLAYER_BOOTS_IRON) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                        func_8083D36C(play, this);
                        this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
                    }
                }
            }

            this->prevBoots = this->currentBoots;
        }

        if ((this->actor.parent == NULL) && (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
            this->actor.parent = this->rideActor;
            Player_SetupRideHorse(play, this);
            this->stateFlags1 |= PLAYER_STATE1_RIDING_HORSE;
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_uma_wait_1);
            Player_SetupAnimMovement(play, this,
                                     PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                         PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                         PLAYER_ANIMMOVEFLAGS_7);
            this->genericTimer = 99;
        }

        if (this->comboTimer == 0) {
            this->slashCounter = 0;
        } else if (this->comboTimer < 0) {
            this->comboTimer++;
        } else {
            this->comboTimer--;
        }

        Math_ScaledStepToS(&this->shapePitchOffset, 0, DEG_TO_BINANG(2.1973f));
        func_80032CB4(this->unk_3A8, 20, 80, 6);

        this->actor.shape.face = this->unk_3A8[0] + ((play->gameplayFrames & 32) ? 0 : 3);

        if (this->currentMask == PLAYER_MASK_BUNNY) {
            Player_BunnyHoodPhysics(this);
        }

        if (Actor_PlayerIsAimingFpsItem(this) != 0) {
            Player_BowStringMoveAfterShot(this);
        }

        if (!(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_7)) {
            if (((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (sFloorType == FLOOR_TYPE_SLIPPERY_ICE) &&
                 (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                ((this->currentBoots == PLAYER_BOOTS_HOVER) &&
                 !(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)))) {
                f32 sp70 = this->linearVelocity;
                s16 sp6E = this->currentYaw;
                s16 yawDiff = this->actor.world.rot.y - sp6E;
                s32 pad;

                if ((ABS(yawDiff) > 0x6000) && (this->actor.speedXZ != 0.0f)) {
                    sp70 = 0.0f;
                    sp6E += 0x8000;
                }

                if (Math_StepToF(&this->actor.speedXZ, sp70, 0.35f) && (sp70 == 0.0f)) {
                    this->actor.world.rot.y = this->currentYaw;
                }

                if (this->linearVelocity != 0.0f) {
                    s32 phi_v0;

                    phi_v0 = (fabsf(this->linearVelocity) * 700.0f) - (fabsf(this->actor.speedXZ) * 100.0f);
                    phi_v0 = CLAMP(phi_v0, 0, 1350);

                    Math_ScaledStepToS(&this->actor.world.rot.y, sp6E, phi_v0);
                }

                if ((this->linearVelocity == 0.0f) && (this->actor.speedXZ != 0.0f)) {
                    func_800F4138(&this->actor.projectedPos, 0xD0, this->actor.speedXZ);
                }
            } else {
                this->actor.speedXZ = this->linearVelocity;
                this->actor.world.rot.y = this->currentYaw;
            }

            func_8002D868(&this->actor);

            if ((this->pushedSpeed != 0.0f) && !Player_InCsMode(play) &&
                !(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                       PLAYER_STATE1_CLIMBING)) &&
                (Player_JumpUpToLedge != this->actionFunc) && (Player_UpdateMagicSpell != this->actionFunc)) {
                this->actor.velocity.x += this->pushedSpeed * Math_SinS(this->pushedYaw);
                this->actor.velocity.z += this->pushedSpeed * Math_CosS(this->pushedYaw);
            }

            func_8002D7EC(&this->actor);
            Player_UpdateBgcheck(play, this);
        } else {
            sFloorType = FLOOR_TYPE_NONE;
            this->floorProperty = FLOOR_PROPERTY_NONE;

            if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
                EnHorse* rideActor = (EnHorse*)this->rideActor;
                CollisionPoly* floorPoly;
                s32 floorBgId;
                Vec3f raycastPos;

                if (!(rideActor->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_RaycastFloorWithOffset(play, this, &sHorseRaycastOffset, &raycastPos, &floorPoly,
                                                  &floorBgId);
                } else {
                    floorPoly = rideActor->actor.floorPoly;
                    floorBgId = rideActor->actor.floorBgId;
                }

                if ((floorPoly != NULL) && Player_SetupExit(play, this, floorPoly, floorBgId)) {
                    if (DREG(25) != 0) {
                        DREG(25) = 0;
                    } else {
                        AREG(6) = 1;
                    }
                }
            }

            sConveyorSpeedIndex = CONVEYOR_SPEED_DISABLED;
            this->pushedSpeed = 0.0f;
        }

        // This block applies the bg conveyor to pushedSpeed
        if ((sConveyorSpeedIndex != CONVEYOR_SPEED_DISABLED) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
            f32 conveyorSpeed;

            // converts 1-index to 0-index
            sConveyorSpeedIndex--;

            if (!sIsFloorConveyor) {
                conveyorSpeed = sWaterConveyorSpeeds[sConveyorSpeedIndex];

                if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
                    conveyorSpeed *= 0.25f;
                }
            } else {
                conveyorSpeed = sFloorConveyorSpeeds[sConveyorSpeedIndex];
            }

            Math_StepToF(&this->pushedSpeed, conveyorSpeed, conveyorSpeed * 0.1f);

            Math_ScaledStepToS(&this->pushedYaw, sConveyorYaw,
                               ((this->stateFlags1 & PLAYER_STATE1_SWIMMING) ? 400.0f : 800.0f) * conveyorSpeed);
        } else if (this->pushedSpeed != 0.0f) {
            Math_StepToF(&this->pushedSpeed, 0.0f, (this->stateFlags1 & PLAYER_STATE1_SWIMMING) ? 0.5f : 1.0f);
        }

        if (!Player_InBlockingCsMode(play, this) && !(this->stateFlags2 & PLAYER_STATE2_CRAWLING)) {
            func_8083D53C(play, this);

            if ((this->actor.category == ACTORCAT_PLAYER) && (gSaveContext.health == 0)) {
                if (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                         PLAYER_STATE1_CLIMBING)) {
                    Player_ResetAttributes(play, this);
                    Player_SetupFallFromLedge(this, play);
                } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                           (this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
                    Player_SetupDie(play, this,
                                    Player_IsSwimming(this)   ? &gPlayerAnim_link_swimer_swim_down
                                    : (this->shockTimer != 0) ? &gPlayerAnim_link_normal_electric_shock_end
                                                              : &gPlayerAnim_link_derth_rebirth);
                }
            } else {
                if ((this->actor.parent == NULL) && ((play->transitionTrigger == TRANS_TRIGGER_START) ||
                                                     (this->deathTimer != 0) || !Player_UpdateDamage(this, play))) {
                    Player_SetupMidairBehavior(this, play);
                } else {
                    this->fallStartHeight = this->actor.world.pos.y;
                }
                Player_SetupStoneOfAgonyRumble(this);
            }
        }

        if ((play->csCtx.state != CS_STATE_IDLE) && (this->csMode != PLAYER_CSMODE_UNK_6) &&
            !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
            !(this->stateFlags2 & PLAYER_STATE2_RESTRAINED_BY_ENEMY) && (this->actor.category == ACTORCAT_PLAYER)) {
            CsCmdActorAction* linkCsAction = play->csCtx.linkAction;

            if ((linkCsAction != NULL) && (slinkCsActions[linkCsAction->action] != 0)) {
                Actor_SetPlayerCutscene(play, NULL, PLAYER_CSMODE_UNK_6);
                Player_StopMovement(this);
            } else if ((this->csMode == PLAYER_CSMODE_NONE) && !(this->stateFlags2 & PLAYER_STATE2_DIVING) &&
                       (play->csCtx.state != CS_STATE_UNSKIPPABLE_INIT)) {
                Actor_SetPlayerCutscene(play, NULL, PLAYER_CSMODE_IDLE_4);
                Player_StopMovement(this);
            }
        }

        if (this->csMode != PLAYER_CSMODE_NONE) {
            if ((this->csMode != PLAYER_CSMODE_END) ||
                !(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE |
                                       PLAYER_STATE1_CLIMBING | PLAYER_STATE1_TAKING_DAMAGE))) {
                this->attentionMode = PLAYER_ATTENTIONMODE_CUTSCENE;
            } else if (Player_StartCutscene != this->actionFunc) {
                Player_CutsceneEnd(play, this, NULL);
            }
        } else {
            this->prevCsMode = 0;
        }

        func_8083D6EC(play, this);

        if ((this->targetActor == NULL) && (this->naviTextId == 0)) {
            this->stateFlags2 &= ~(PLAYER_STATE2_CAN_SPEAK_OR_CHECK | PLAYER_STATE2_NAVI_REQUESTING_TALK);
        }

        this->stateFlags1 &= ~(PLAYER_STATE1_SWINGING_BOTTLE | PLAYER_STATE1_READY_TO_SHOOT |
                               PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_22);
        this->stateFlags2 &=
            ~(PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL |
              PLAYER_STATE2_MAKING_REACTABLE_NOISE | PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING |
              PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION | PLAYER_STATE2_ENABLE_PUSH_PULL_CAM |
              PLAYER_STATE2_SPAWN_DUST_AT_FEET | PLAYER_STATE2_IDLE_WHILE_CLIMBING | PLAYER_STATE2_FROZEN_IN_ICE |
              PLAYER_STATE2_DO_ACTION_ENTER | PLAYER_STATE2_CAN_DISMOUNT_HORSE | PLAYER_STATE2_ENABLE_REFLECTION);
        this->stateFlags3 &= ~PLAYER_STATE3_CHECKING_FLOOR_AND_WATER_COLLISION;

        Player_StepLookToZero(this);
        Player_StoreAnalogStickInput(play, this);

        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            sWaterSpeedScale = 0.5f;
        } else {
            sWaterSpeedScale = 1.0f;
        }

        sInvertedWaterSpeedScale = 1.0f / sWaterSpeedScale;
        sUsingItemAlreadyInHand = sUsingItemAlreadyInHand2 = 0;
        sCurrentMask = this->currentMask;

        if (!(this->stateFlags3 & PLAYER_STATE3_PAUSE_ACTION_FUNC)) {
            this->actionFunc(this, play);
        }

        Player_UpdateCamAndSeqModes(play, this);

        if (this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION) {
            AnimationContext_SetMoveActor(play, &this->actor, &this->skelAnime,
                                          (this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE)
                                              ? 1.0f
                                              : this->ageProperties->translationScale);
        }

        Player_UpdateYaw(this, play);

        if (CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_8)) {
            this->talkActorDistance = 0.0f;
        } else {
            this->talkActor = NULL;
            this->talkActorDistance = FLT_MAX;
            this->exchangeItemId = EXCH_ITEM_NONE;
        }

        if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            this->interactRangeActor = NULL;
            this->getItemDirection = 0x6000;
        }

        if (this->actor.parent == NULL) {
            this->rideActor = NULL;
        }

        this->naviTextId = 0;

        if (!(this->stateFlags2 & PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR)) {
            this->ocarinaActor = NULL;
        }

        this->stateFlags2 &= ~PLAYER_STATE2_NEAR_OCARINA_ACTOR;
        this->stoneOfAgonyActorDistSq = FLT_MAX;

        temp_f0 = this->actor.world.pos.y - this->actor.prevPos.y;

        this->doorType = PLAYER_DOORTYPE_NONE;
        this->damageEffect = PLAYER_DMGEFFECT_NONE;
        this->forcedTargetActor = NULL;

        phi_f12 =
            ((this->bodyPartsPos[PLAYER_BODYPART_L_FOOT].y + this->bodyPartsPos[PLAYER_BODYPART_R_FOOT].y) * 0.5f) +
            temp_f0;
        temp_f0 += this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 10.0f;

        this->cylinder.dim.height = temp_f0 - phi_f12;

        if (this->cylinder.dim.height < 0) {
            phi_f12 = temp_f0;
            this->cylinder.dim.height = -this->cylinder.dim.height;
        }

        this->cylinder.dim.yShift = phi_f12 - this->actor.world.pos.y;

        if (this->stateFlags1 & PLAYER_STATE1_22) {
            this->cylinder.dim.height = this->cylinder.dim.height * 0.8f;
        }

        Collider_UpdateCylinder(&this->actor, &this->cylinder);

        if (!(this->stateFlags2 & PLAYER_STATE2_FROZEN_IN_ICE)) {
            if (!(this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP |
                                       PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_RIDING_HORSE))) {
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->cylinder.base);
            }

            if (!(this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_TAKING_DAMAGE)) &&
                (this->invincibilityTimer <= 0)) {
                CollisionCheck_SetAC(play, &play->colChkCtx, &this->cylinder.base);

                if (this->invincibilityTimer < 0) {
                    CollisionCheck_SetAT(play, &play->colChkCtx, &this->cylinder.base);
                }
            }
        }

        AnimationContext_SetNextQueue(play);
    }

    Math_Vec3f_Copy(&this->actor.home.pos, &this->actor.world.pos);
    Math_Vec3f_Copy(&this->prevWaistPos, &this->bodyPartsPos[PLAYER_BODYPART_WAIST]);

    if (this->stateFlags1 &
        (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE)) {
        this->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        this->actor.colChkInfo.mass = 50;
    }

    this->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;

    Collider_ResetCylinderAC(play, &this->cylinder.base);

    Collider_ResetQuadAT(play, &this->meleeWeaponQuads[0].base);
    Collider_ResetQuadAT(play, &this->meleeWeaponQuads[1].base);

    Collider_ResetQuadAC(play, &this->shieldQuad.base);
    Collider_ResetQuadAT(play, &this->shieldQuad.base);
}

static Vec3f D_80854838 = { 0.0f, 0.0f, -30.0f };

void Player_Update(Actor* thisx, PlayState* play) {
    static Vec3f sDogSpawnPos;
    Player* this = (Player*)thisx;
    s32 dogParams;
    s32 pad;
    Input input;
    Actor* dog;

    if (Player_CheckNoDebugModeCombo(this, play)) {
        if (gSaveContext.dogParams < 0) {
            if (Object_GetIndex(&play->objectCtx, OBJECT_DOG) < 0) {
                gSaveContext.dogParams = 0;
            } else {
                gSaveContext.dogParams &= 0x7FFF;
                Player_GetWorldPosRelativeToPlayer(this, &this->actor.world.pos, &D_80854838, &sDogSpawnPos);
                dogParams = gSaveContext.dogParams;

                dog = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_DOG, sDogSpawnPos.x, sDogSpawnPos.y, sDogSpawnPos.z,
                                  0, this->actor.shape.rot.y, 0, dogParams | 0x8000);
                if (dog != NULL) {
                    dog->room = 0;
                }
            }
        }

        if ((this->interactRangeActor != NULL) && (this->interactRangeActor->update == NULL)) {
            this->interactRangeActor = NULL;
        }

        if ((this->heldActor != NULL) && (this->heldActor->update == NULL)) {
            Player_DetatchHeldActor(play, this);
        }

        if (this->stateFlags1 & (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE)) {
            bzero(&input, sizeof(input));
        } else {
            input = play->state.input[0];
            if (this->endTalkTimer != 0) {
                input.cur.button &= ~(BTN_A | BTN_B | BTN_CUP);
                input.press.button &= ~(BTN_A | BTN_B | BTN_CUP);
            }
        }

        Player_UpdateCommon(this, play, &input);
    }

    MREG(52) = this->actor.world.pos.x;
    MREG(53) = this->actor.world.pos.y;
    MREG(54) = this->actor.world.pos.z;
    MREG(55) = this->actor.world.rot.y;
}

static struct_80858AC8 D_80858AC8;
static Vec3s sFishingBlendTable[25];

static Gfx* sMaskDlists[PLAYER_MASK_MAX - 1] = {
    gLinkChildKeatonMaskDL, gLinkChildSkullMaskDL, gLinkChildSpookyMaskDL, gLinkChildBunnyHoodDL,
    gLinkChildGoronMaskDL,  gLinkChildZoraMaskDL,  gLinkChildGerudoMaskDL, gLinkChildMaskOfTruthDL,
};

static Vec3s D_80854864 = { 0, 0, 0 };

void Player_DrawGameplay(PlayState* play, Player* this, s32 lod, Gfx* cullDList, OverrideLimbDrawOpa overrideLimbDraw) {
    static s32 D_8085486C = 255;

    OPEN_DISPS(play->state.gfxCtx, "../z_player.c", 19228);

    gSPSegment(POLY_OPA_DISP++, 0x0C, cullDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, cullDList);

    Player_DrawImpl(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount, lod,
                    this->currentTunic, this->currentBoots, this->actor.shape.face, overrideLimbDraw,
                    Player_PostLimbDrawGameplay, this);

    if ((overrideLimbDraw == Player_OverrideLimbDrawGameplayDefault) && (this->currentMask != PLAYER_MASK_NONE)) {
        Mtx* sp70 = Graph_Alloc(play->state.gfxCtx, 2 * sizeof(Mtx));

        if (this->currentMask == PLAYER_MASK_BUNNY) {
            Vec3s sp68;

            gSPSegment(POLY_OPA_DISP++, 0x0B, sp70);

            sp68.x = D_80858AC8.unk_02 + 0x3E2;
            sp68.y = D_80858AC8.unk_04 + 0xDBE;
            sp68.z = D_80858AC8.unk_00 - 0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, -240.0f, &sp68);
            Matrix_ToMtx(sp70++, "../z_player.c", 19273);

            sp68.x = D_80858AC8.unk_02 - 0x3E2;
            sp68.y = -0xDBE - D_80858AC8.unk_04;
            sp68.z = D_80858AC8.unk_00 - 0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, 240.0f, &sp68);
            Matrix_ToMtx(sp70, "../z_player.c", 19279);
        }

        gSPDisplayList(POLY_OPA_DISP++, sMaskDlists[this->currentMask - 1]);
    }

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
        !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->hoverBootsTimer != 0)) {
        s32 sp5C;
        s32 hoverBootsTimer = this->hoverBootsTimer;

        if (this->hoverBootsTimer < 19) {
            if (hoverBootsTimer >= 15) {
                D_8085486C = (19 - hoverBootsTimer) * 51.0f;
            } else if (hoverBootsTimer < 19) {
                sp5C = hoverBootsTimer;

                if (sp5C > 9) {
                    sp5C = 9;
                }

                D_8085486C = (-sp5C * 4) + 36;
                D_8085486C = D_8085486C * D_8085486C;
                D_8085486C = (s32)((Math_CosS(D_8085486C) * 100.0f) + 100.0f) + 55.0f;
                D_8085486C = D_8085486C * (sp5C * (1.0f / 9.0f));
            }

            Matrix_SetTranslateRotateYXZ(this->actor.world.pos.x, this->actor.world.pos.y + 2.0f,
                                         this->actor.world.pos.z, &D_80854864);
            Matrix_Scale(4.0f, 4.0f, 4.0f, MTXMODE_APPLY);

            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, "../z_player.c", 19317),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, 0, 16, 32, 1, 0,
                                        (play->gameplayFrames * -15) % 128, 16, 32));
            gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, D_8085486C);
            gDPSetEnvColor(POLY_XLU_DISP++, 120, 90, 30, 128);
            gSPDisplayList(POLY_XLU_DISP++, gHoverBootsCircleDL);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../z_player.c", 19328);
}

void Player_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Player* this = (Player*)thisx;

    OPEN_DISPS(play->state.gfxCtx, "../z_player.c", 19346);

    if (!(this->stateFlags2 & PLAYER_STATE2_DISABLE_DRAW)) {
        OverrideLimbDrawOpa overrideLimbDraw = Player_OverrideLimbDrawGameplayDefault;
        s32 lod;
        s32 pad;

        if ((this->csMode != PLAYER_CSMODE_NONE) || (Player_IsUnfriendlyZTargeting(this) && 0) ||
            (this->actor.projectedPos.z < 160.0f)) {
            lod = 0;
        } else {
            lod = 1;
        }

        func_80093C80(play);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);

        if (this->invincibilityTimer > 0) {
            this->damageFlashTimer += CLAMP(50 - this->invincibilityTimer, 8, 40);
            POLY_OPA_DISP = Gfx_SetFog2(POLY_OPA_DISP, 255, 0, 0, 0, 0,
                                        4000 - (s32)(Math_CosS(this->damageFlashTimer * 256) * 2000.0f));
        }

        func_8002EBCC(&this->actor, play, 0);
        func_8002ED80(&this->actor, play, 0);

        if (this->attentionMode != PLAYER_ATTENTIONMODE_NONE) {
            Vec3f projectedHeadPos;

            SkinMatrix_Vec3fMtxFMultXYZ(&play->viewProjectionMtxF, &this->actor.focus.pos, &projectedHeadPos);
            if (projectedHeadPos.z < -4.0f) {
                overrideLimbDraw = Player_OverrideLimbDrawGameplayFirstPerson;
            }
        } else if (this->stateFlags2 & PLAYER_STATE2_CRAWLING) {
            if (this->actor.projectedPos.z < 0.0f) {
                // Player is behind the camera
                overrideLimbDraw = Player_OverrideLimbDrawGameplayCrawling;
            }
        }

        if (this->stateFlags2 & PLAYER_STATE2_ENABLE_REFLECTION) {
            f32 pitchOffset = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 600));
            f32 yawOffset = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 1000));

            Matrix_Push();
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_SetTranslateRotateYXZ(
                this->actor.world.pos.x,
                (this->actor.floorHeight + (this->actor.floorHeight - this->actor.world.pos.y)) +
                    (this->actor.shape.yOffset * this->actor.scale.y),
                this->actor.world.pos.z, &this->actor.shape.rot);
            Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
            Matrix_RotateX(pitchOffset, MTXMODE_APPLY);
            Matrix_RotateY(yawOffset, MTXMODE_APPLY);
            Matrix_Scale(1.1f, 0.95f, 1.05f, MTXMODE_APPLY);
            Matrix_RotateY(-yawOffset, MTXMODE_APPLY);
            Matrix_RotateX(-pitchOffset, MTXMODE_APPLY);
            Player_DrawGameplay(play, this, lod, gCullFrontDList, overrideLimbDraw);
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_Pop();
        }

        gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BOTH);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BOTH);

        Player_DrawGameplay(play, this, lod, gCullBackDList, overrideLimbDraw);

        if (this->invincibilityTimer > 0) {
            POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);
        }

        if (this->stateFlags2 & PLAYER_STATE2_FROZEN_IN_ICE) {
            f32 scale = (this->genericVar >> 1) * 22.0f;

            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, (0 - play->gameplayFrames) % 128, 32,
                                        32, 1, 0, (play->gameplayFrames * -2) % 128, 32, 32));

            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, "../z_player.c", 19459),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 50, 100, 255);
            gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment3DL);
        }

        if (this->giDrawIdPlusOne > 0) {
            Player_DrawGetItem(play, this);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../z_player.c", 19473);
}

void Player_Destroy(Actor* thisx, PlayState* play) {
    Player* this = (Player*)thisx;

    Effect_Delete(play, this->meleeWeaponEffectIndex);

    Collider_DestroyCylinder(play, &this->cylinder);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[0]);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[1]);
    Collider_DestroyQuad(play, &this->shieldQuad);

    Magic_Reset(play);

    gSaveContext.linkAge = play->linkAgeOnLoad;
}

s16 Player_SetFirstPersonAimLookAngles(PlayState* play, Player* this, s32 arg2, s16 lookYawOffset) {
    s32 angleMinMax;
    s16 aimAngleY;
    s16 aimAngleX;

    if (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this) && (arg2 == 0)) {
        aimAngleY = sControlInput->rel.stick_y * 240.0f;
        Math_SmoothStepToS(&this->actor.focus.rot.x, aimAngleY, 14, DEG_TO_BINANG(21.973f), 0x1E);

        aimAngleY = sControlInput->rel.stick_x * -16.0f;
        aimAngleY = CLAMP(aimAngleY, -DEG_TO_BINANG(16.4795f), DEG_TO_BINANG(16.4795f));
        this->actor.focus.rot.y += aimAngleY;
    } else {
        angleMinMax =
            (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) ? DEG_TO_BINANG(19.23f) : DEG_TO_BINANG(76.9043f);
        aimAngleX = ((sControlInput->rel.stick_y >= 0) ? 1 : -1) *
                    (s32)((1.0f - Math_CosS(sControlInput->rel.stick_y * 200)) * 1500.0f);
        this->actor.focus.rot.x += aimAngleX;
        this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -angleMinMax, angleMinMax);

        angleMinMax = DEG_TO_BINANG(105.0f);
        aimAngleY = this->actor.focus.rot.y - this->actor.shape.rot.y;
        aimAngleX = ((sControlInput->rel.stick_x >= 0) ? 1 : -1) *
                    (s32)((1.0f - Math_CosS(sControlInput->rel.stick_x * 200)) * -1500.0f);
        aimAngleY += aimAngleX;
        this->actor.focus.rot.y = CLAMP(aimAngleY, -angleMinMax, angleMinMax) + this->actor.shape.rot.y;
    }

    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y;
    return Player_UpdateLookAngles(this, (play->shootingGalleryStatus != 0) || Actor_PlayerIsAimingReadyFpsItem(this) ||
                                             Player_IsAimingReadyBoomerang(this)) -
           lookYawOffset;
}

void Player_UpdateSwimMovement(Player* this, f32* linearVelocity, f32 targetVelocity, s16 targetYaw) {
    f32 step;
    f32 speedLimit;

    step = this->skelAnime.curFrame - 10.0f;

    speedLimit = (R_RUN_SPEED_LIMIT / 100.0f) * 0.8f;
    if (*linearVelocity > speedLimit) {
        *linearVelocity = speedLimit;
    }

    if ((0.0f < step) && (step < 10.0f)) {
        step *= 6.0f;
    } else {
        step = 0.0f;
        targetVelocity = 0.0f;
    }

    Math_AsymStepToF(linearVelocity, targetVelocity * 0.8f, step, (fabsf(*linearVelocity) * 0.02f) + 0.05f);
    Math_ScaledStepToS(&this->currentYaw, targetYaw, 1600);
}

void Player_SetVerticalWaterVelocity(Player* this) {
    f32 accel;
    f32 waterSurface;
    f32 targetVelocity;
    f32 yDistToWater;

    targetVelocity = -5.0f;

    waterSurface = this->ageProperties->waterSurface;
    if (this->actor.velocity.y < 0.0f) {
        waterSurface += 1.0f;
    }

    if (this->actor.yDistToWater < waterSurface) {
        if (this->actor.velocity.y <= 0.0f) {
            waterSurface = 0.0f;
        } else {
            waterSurface = this->actor.velocity.y * 0.5f;
        }
        accel = -0.1f - waterSurface;
    } else {
        if (!(this->stateFlags1 & PLAYER_STATE1_IN_DEATH_CUTSCENE) && (this->currentBoots == PLAYER_BOOTS_IRON) &&
            (this->actor.velocity.y >= -3.0f)) {
            accel = -0.2f;
        } else {
            targetVelocity = 2.0f;
            if (this->actor.velocity.y >= 0.0f) {
                waterSurface = 0.0f;
            } else {
                waterSurface = this->actor.velocity.y * -0.3f;
            }
            accel = waterSurface + 0.1f;
        }

        yDistToWater = this->actor.yDistToWater;
        if (yDistToWater > 100.0f) {
            this->stateFlags2 |= PLAYER_STATE2_DIVING;
        }
    }

    this->actor.velocity.y += accel;

    if (((this->actor.velocity.y - targetVelocity) * accel) > 0) {
        this->actor.velocity.y = targetVelocity;
    }

    this->actor.gravity = 0.0f;
}

void Player_PlaySwimAnim(PlayState* play, Player* this, Input* input, f32 linearVelocity) {
    f32 playSpeed;

    if ((input != NULL) && CHECK_BTN_ANY(input->press.button, BTN_A | BTN_B)) {
        playSpeed = 1.0f;
    } else {
        playSpeed = 0.5f;
    }

    playSpeed *= linearVelocity;

    if (playSpeed < 1.0f) {
        playSpeed = 1.0f;
    }

    this->skelAnime.playSpeed = playSpeed;
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_FirstPersonAiming(Player* this, PlayState* play) {
    if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
        Player_SetVerticalWaterVelocity(this);
        Player_UpdateSwimMovement(this, &this->linearVelocity, 0, this->actor.shape.rot.y);
    } else {
        Player_StepLinearVelocityToZero(this);
    }

    if ((this->attentionMode == PLAYER_ATTENTIONMODE_AIMING) &&
        (Actor_PlayerIsAimingFpsItem(this) || Player_IsAimingBoomerang(this))) {
        Player_SetupCurrentUpperAction(this, play);
    }

    if ((this->csMode != PLAYER_CSMODE_NONE) || (this->attentionMode == PLAYER_ATTENTIONMODE_NONE) ||
        (this->attentionMode >= PLAYER_ATTENTIONMODE_ITEM_CUTSCENE) || Player_SetupStartUnfriendlyZTargeting(this) ||
        (this->targetActor != NULL) || !Player_SetupCameraMode(play, this) ||
        (((this->attentionMode == PLAYER_ATTENTIONMODE_AIMING) &&
          (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_R) || Player_IsFriendlyZTargeting(this) ||
           (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this)))) ||
         ((this->attentionMode == PLAYER_ATTENTIONMODE_C_UP) &&
          CHECK_BTN_ANY(sControlInput->press.button,
                        BTN_A | BTN_B | BTN_R | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)))) {
        Player_ClearLookAndAttention(this, play);
        func_80078884(NA_SE_SY_CAMERA_ZOOM_UP);
    } else if ((DECR(this->genericTimer) == 0) || (this->attentionMode != PLAYER_ATTENTIONMODE_AIMING)) {
        if (Player_IsShootingHookshot(this)) {
            this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y |
                               PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
        } else {
            this->actor.shape.rot.y = Player_SetFirstPersonAimLookAngles(play, this, 0, 0);
        }
    }

    this->currentYaw = this->actor.shape.rot.y;
}

s32 Player_SetupShootingGalleryPlay(PlayState* play, Player* this) {
    if (play->shootingGalleryStatus != 0) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetActionFunc(play, this, Player_ShootingGalleryPlay, 0);

        if (!Actor_PlayerIsAimingFpsItem(this) || Player_HoldsHookshot(this)) {
            Player_UseItem(play, this, ITEM_BOW);
        }

        this->stateFlags1 |= PLAYER_STATE1_IN_FIRST_PERSON_MODE;
        Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
        Player_StopMovement(this);
        Player_ResetLookAngles(this);
        return 1;
    }

    return 0;
}

void Player_SetOcarinaItemAction(Player* this) {
    this->itemAction =
        (INV_CONTENT(ITEM_OCARINA_FAIRY) == ITEM_OCARINA_FAIRY) ? PLAYER_IA_OCARINA_FAIRY : PLAYER_IA_OCARINA_OF_TIME;
}

s32 Player_SetupForcePullOcarina(PlayState* play, Player* this) {
    if (this->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA) {
        this->stateFlags3 &= ~PLAYER_STATE3_FORCE_PULL_OCARINA;
        Player_SetOcarinaItemAction(this);
        this->attentionMode = PLAYER_ATTENTIONMODE_ITEM_CUTSCENE;
        Player_SetupItemCutsceneOrFirstPerson(this, play);
        return 1;
    }

    return 0;
}

void Player_TalkWithActor(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_SetupCurrentUpperAction(this, play);

    if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
        this->actor.flags &= ~ACTOR_FLAG_8;

        if (!CHECK_FLAG_ALL(this->talkActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
            this->stateFlags2 &= ~PLAYER_STATE2_USING_SWITCH_Z_TARGETING;
        }

        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));

        if (!Player_SetupForcePullOcarina(play, this) && !Player_SetupShootingGalleryPlay(play, this) &&
            !Player_SetupCutscene(play, this)) {
            if ((this->talkActor != this->interactRangeActor) || !Player_SetupGetItemOrHoldBehavior(this, play)) {
                if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
                    s32 preservedTimer = this->genericTimer;
                    Player_SetupRideHorse(play, this);
                    this->genericTimer = preservedTimer;
                } else if (Player_IsSwimming(this)) {
                    Player_SetupSwimIdle(play, this);
                } else {
                    Player_SetupStandingStillMorph(this, play);
                }
            }
        }

        this->endTalkTimer = 10;
        return;
    }

    if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
        Player_RideHorse(this, play);
    } else if (Player_IsSwimming(this)) {
        Player_UpdateSwimIdle(this, play);
    } else if (!Player_IsUnfriendlyZTargeting(this) && LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->skelAnime.moveFlags != 0) {
            Player_EndAnimMovement(this);
            if ((this->talkActor->category == ACTORCAT_NPC) && (this->heldItemAction != PLAYER_IA_FISHING_POLE)) {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_normal_talk_free);
            } else {
                Player_PlayAnimLoop(play, this, Player_GetStandingStillAnim(this));
            }
        } else {
            Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_normal_talk_free_wait);
        }
    }

    if (this->targetActor != NULL) {
        this->currentYaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_GrabPushPullWall(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 pushPullDir;

    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                         PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;
    Player_PushPullSetPositionAndYaw(play, this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_SetupPushPullWallIdle(play, this)) {
            Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
            pushPullDir = Player_GetPushPullDirection(this, &targetVelocity, &targetYaw);
            if (pushPullDir > 0) {
                Player_SetupPushWall(this, play);
            } else if (pushPullDir < 0) {
                Player_SetupPullWall(this, play);
            }
        }
    }
}

// Moves dynapoly push/pull actors?
void func_8084B840(PlayState* play, Player* this, f32 arg2) {
    if (this->actor.wallBgId != BGCHECK_SCENE) {
        DynaPolyActor* dynaPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);

        if (dynaPolyActor != NULL) {
            func_8002DFA4(dynaPolyActor, arg2, this->actor.world.rot.y);
        }
    }
}

static PlayerAnimSfxEntry sPushPullWallAnimSfx[] = {
    { NA_SE_PL_SLIP, 0x1003 },
    { NA_SE_PL_SLIP, -0x1015 },
};

void Player_PushWall(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 pushPullDir;

    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                         PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;

    if (Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_normal_pushing)) {
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
        }
    }

    Player_PlayAnimSfx(this, sPushPullWallAnimSfx);
    Player_PushPullSetPositionAndYaw(play, this);

    if (!Player_SetupPushPullWallIdle(play, this)) {
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        pushPullDir = Player_GetPushPullDirection(this, &targetVelocity, &targetYaw);
        if (pushPullDir < 0) {
            Player_SetupPullWall(this, play);
        } else if (pushPullDir == 0) {
            Player_StartGrabPushPullWall(this, &gPlayerAnim_link_normal_push_end, play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        }
    }

    if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
        func_8084B840(play, this, 2.0f);
        this->linearVelocity = 2.0f;
    }
}

static PlayerAnimSfxEntry sPushPullWallAnim2Sfx[] = {
    { NA_SE_PL_SLIP, 0x1004 },
    { NA_SE_PL_SLIP, -0x1018 },
};

static Vec3f sPullWallRaycastOffset = { 0.0f, 26.0f, -40.0f };

void Player_PullWall(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 targetVelocity;
    s16 targetYaw;
    s32 pushPullDir;
    Vec3f raycastPos;
    f32 floorPosY;
    CollisionPoly* colPoly;
    s32 polyBgId;
    Vec3f lineCheckPos;
    Vec3f posResult;

    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_pulling, this->modelAnimType);
    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                         PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;

    if (Player_LoopAnimContinuously(play, this, anim)) {
        this->genericTimer = 1;
    } else {
        if (this->genericTimer == 0) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
            }
        } else {
            Player_PlayAnimSfx(this, sPushPullWallAnim2Sfx);
        }
    }

    Player_PushPullSetPositionAndYaw(play, this);

    if (!Player_SetupPushPullWallIdle(play, this)) {
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        pushPullDir = Player_GetPushPullDirection(this, &targetVelocity, &targetYaw);
        if (pushPullDir > 0) {
            Player_SetupPushWall(this, play);
        } else if (pushPullDir == 0) {
            Player_StartGrabPushPullWall(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_pull_end, this->modelAnimType), play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        }
    }

    if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
        floorPosY =
            Player_RaycastFloorWithOffset2(play, this, &sPullWallRaycastOffset, &raycastPos) - this->actor.world.pos.y;
        if (fabsf(floorPosY) < 20.0f) {
            lineCheckPos.x = this->actor.world.pos.x;
            lineCheckPos.z = this->actor.world.pos.z;
            lineCheckPos.y = raycastPos.y;
            if (!BgCheck_EntityLineTest1(&play->colCtx, &lineCheckPos, &raycastPos, &posResult, &colPoly, true, false,
                                         false, true, &polyBgId)) {
                func_8084B840(play, this, -2.0f);
                return;
            }
        }
        this->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    }
}

void Player_GrabLedge(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    LinkAnimationHeader* anim;
    f32 temp;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        // clang-format off
        anim = (this->climbStatus > PLAYER_CLIMBSTATUS_MOVING_UP) ? &gPlayerAnim_link_normal_fall_wait : GET_PLAYER_ANIM(PLAYER_ANIMGROUP_jump_climb_wait, this->modelAnimType); Player_PlayAnimLoop(play, this, anim);
        // clang-format on
    } else if (this->climbStatus == PLAYER_CLIMBSTATUS_MOVING_UP) {
        if (this->skelAnime.animation == &gPlayerAnim_link_normal_fall) {
            temp = 11.0f;
        } else {
            temp = 1.0f;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, temp)) {
            Player_PlayMoveSfx(this, NA_SE_PL_WALK_GROUND);
            if (this->skelAnime.animation == &gPlayerAnim_link_normal_fall) {
                this->climbStatus = PLAYER_CLIMBSTATUS_KNOCKED_DOWN;
            } else {
                this->climbStatus = PLAYER_CLIMBSTATUS_MOVING_DOWN;
            }
        }
    }

    Math_ScaledStepToS(&this->actor.shape.rot.y, this->currentYaw, 0x800);

    if (this->climbStatus != PLAYER_CLIMBSTATUS_MOVING_UP) {
        Player_GetTargetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        if (this->analogStickInputs[this->inputFrameCounter] >= 0) {
            if (this->climbStatus > PLAYER_CLIMBSTATUS_MOVING_UP) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_fall_up, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_jump_climb_up, this->modelAnimType);
            }
            Player_SetupClimbOntoLedge(this, anim, play);
            return;
        }

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) || (this->actor.shape.feetFloorFlag != 0)) {
            Player_SetLedgeGrabPosition(this);
            if (this->genericVar < 0) {
                this->linearVelocity = -0.8f;
            } else {
                this->linearVelocity = 0.8f;
            }
            Player_SetupFallFromLedge(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE);
        }
    }
}

void Player_ClimbOntoLedge(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_UpdateAnimMovement(this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ);
        Player_SetupStandingStillNoMorph(this, play);
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, this->skelAnime.endFrame - 6.0f)) {
        Player_PlayLandingSfx(this);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, this->skelAnime.endFrame - 34.0f)) {
        this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE);
        func_8002F7DC(&this->actor, NA_SE_PL_CLIMB_CLIFF);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
    }
}

void Player_PlayClimbingSfx(Player* this) {
    func_8002F7DC(&this->actor, (this->climbStatus != 0) ? NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_VINE
                                                         : NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD);
}

void Player_ClimbingWallOrDownLedge(Player* this, PlayState* play) {
    static Vec3f raycastOffset = { 0.0f, 0.0f, 26.0f };
    s32 stickDistY;
    s32 stickDistX;
    f32 animSpeedInfluence;
    f32 playbackDir;
    Vec3f wallPosDiff;
    s32 sp68;
    Vec3f raycastPos;
    f32 floorPosY;
    LinkAnimationHeader* verticalClimbAnim;
    LinkAnimationHeader* horizontalClimbAnim;

    stickDistY = sControlInput->rel.stick_y;
    stickDistX = sControlInput->rel.stick_x;

    this->fallStartHeight = this->actor.world.pos.y;
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if ((this->genericVar != 0) && (ABS(stickDistY) < ABS(stickDistX))) {
        animSpeedInfluence = ABS(stickDistX) * 0.0325f;
        stickDistY = 0;
    } else {
        animSpeedInfluence = ABS(stickDistY) * 0.05f;
        stickDistX = 0;
    }

    if (animSpeedInfluence < 1.0f) {
        animSpeedInfluence = 1.0f;
    } else if (animSpeedInfluence > 3.35f) {
        animSpeedInfluence = 3.35f;
    }

    if (this->skelAnime.playSpeed >= 0.0f) {
        // Playing anim forward
        playbackDir = 1.0f;
    } else {
        // Playing anim backward
        playbackDir = -1.0f;
    }

    this->skelAnime.playSpeed = playbackDir * animSpeedInfluence;

    if (this->genericTimer >= 0) {
        if ((this->actor.wallPoly != NULL) && (this->actor.wallBgId != BGCHECK_SCENE)) {
            DynaPolyActor* wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
            if (wallPolyActor != NULL) {
                Math_Vec3f_Diff(&wallPolyActor->actor.world.pos, &wallPolyActor->actor.prevPos, &wallPosDiff);
                Math_Vec3f_Sum(&this->actor.world.pos, &wallPosDiff, &this->actor.world.pos);
            }
        }

        Actor_UpdateBgCheckInfo(play, &this->actor, 26.0f, 6.0f, this->ageProperties->ceilingCheckHeight,
                                UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
        Player_SetPositionAndYawOnClimbWall(play, this, 26.0f, this->ageProperties->unk_3C, 50.0f, -20.0f);
    }

    if ((this->genericTimer < 0) || !Player_SetupClimbingLetGo(this, play)) {
        if (LinkAnimation_Update(play, &this->skelAnime) != 0) {
            if (this->genericTimer < 0) {
                this->genericTimer = ABS(this->genericTimer) & 1;
                return;
            }

            if (stickDistY != 0) {
                sp68 = this->genericVar + this->genericTimer;

                if (stickDistY > 0) {
                    raycastOffset.y = this->ageProperties->unk_40;
                    floorPosY = Player_RaycastFloorWithOffset2(play, this, &raycastOffset, &raycastPos);

                    if (this->actor.world.pos.y < floorPosY) {
                        if (this->genericVar != 0) {
                            this->actor.world.pos.y = floorPosY;
                            this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
                            Player_SetupGrabLedge(play, this, this->actor.wallPoly, this->ageProperties->unk_3C,
                                                  &gPlayerAnim_link_normal_jump_climb_up_free);
                            this->currentYaw += 0x8000;
                            this->actor.shape.rot.y = this->currentYaw;
                            Player_SetupClimbOntoLedge(this, &gPlayerAnim_link_normal_jump_climb_up_free, play);
                            this->stateFlags1 |= PLAYER_STATE1_CLIMBING_ONTO_LEDGE;
                        } else {
                            Player_SetupEndClimb(this, this->ageProperties->endClimb2Anim[this->genericTimer], play);
                        }
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_4A[sp68];
                        Player_PlayAnimOnce(play, this, this->ageProperties->verticalClimbAnim[sp68]);
                    }
                } else {
                    if ((this->actor.world.pos.y - this->actor.floorHeight) < 15.0f) {
                        if (this->genericVar != 0) {
                            Player_ClimbingLetGo(this, play);
                        } else {
                            if (this->genericTimer != 0) {
                                this->skelAnime.prevTransl = this->ageProperties->unk_44;
                            }
                            Player_SetupEndClimb(this, this->ageProperties->endClimb1Anim[this->genericTimer], play);
                            this->genericTimer = 1;
                        }
                    } else {
                        sp68 ^= 1;
                        this->skelAnime.prevTransl = this->ageProperties->unk_62[sp68];
                        verticalClimbAnim = this->ageProperties->verticalClimbAnim[sp68];
                        LinkAnimation_Change(play, &this->skelAnime, verticalClimbAnim, -1.0f,
                                             Animation_GetLastFrame(verticalClimbAnim), 0.0f, ANIMMODE_ONCE, 0.0f);
                    }
                }
                this->genericTimer ^= 1;
            } else {
                if ((this->genericVar != 0) && (stickDistX != 0)) {
                    horizontalClimbAnim = this->ageProperties->horizontalClimbAnim[this->genericTimer];

                    if (stickDistX > 0) {
                        this->skelAnime.prevTransl = this->ageProperties->unk_7A[this->genericTimer];
                        Player_PlayAnimOnce(play, this, horizontalClimbAnim);
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_86[this->genericTimer];
                        LinkAnimation_Change(play, &this->skelAnime, horizontalClimbAnim, -1.0f,
                                             Animation_GetLastFrame(horizontalClimbAnim), 0.0f, ANIMMODE_ONCE, 0.0f);
                    }
                } else {
                    this->stateFlags2 |= PLAYER_STATE2_IDLE_WHILE_CLIMBING;
                }
            }

            return;
        }
    }

    if (this->genericTimer < 0) {
        if (((this->genericTimer == -2) &&
             (LinkAnimation_OnFrame(&this->skelAnime, 14.0f) || LinkAnimation_OnFrame(&this->skelAnime, 29.0f))) ||
            ((this->genericTimer == -4) &&
             (LinkAnimation_OnFrame(&this->skelAnime, 22.0f) || LinkAnimation_OnFrame(&this->skelAnime, 35.0f) ||
              LinkAnimation_OnFrame(&this->skelAnime, 49.0f) || LinkAnimation_OnFrame(&this->skelAnime, 55.0f)))) {
            Player_PlayClimbingSfx(this);
        }
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, (this->skelAnime.playSpeed > 0.0f) ? 20.0f : 0.0f)) {
        Player_PlayClimbingSfx(this);
    }
}

static f32 sClimbEndFrames[] = { 10.0f, 20.0f };
static f32 sClimbLadderEndFrames[] = { 40.0f, 50.0f };

static PlayerAnimSfxEntry sClimbLadderEndAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD, 0x80A },
    { NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD, 0x814 },
    { NA_SE_PL_WALK_GROUND + SURFACE_SFX_OFFSET_WOOD, -0x81E },
};

void Player_EndClimb(Player* this, PlayState* play) {
    s32 actionInterruptResult;
    f32* endFrames;
    CollisionPoly* floorPoly;
    s32 floorBgId;
    Vec3f raycastPos;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    actionInterruptResult = Player_IsActionInterrupted(play, this, &this->skelAnime, 4.0f);

    if (actionInterruptResult == PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) {
        this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
        return;
    }

    if ((actionInterruptResult > PLAYER_ACTIONINTERRUPT_BY_SUB_ACTION) ||
        LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
        return;
    }

    endFrames = sClimbEndFrames;

    if (this->genericTimer != 0) {
        Player_PlayAnimSfx(this, sClimbLadderEndAnimSfx);
        endFrames = sClimbLadderEndFrames;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, endFrames[0]) ||
        LinkAnimation_OnFrame(&this->skelAnime, endFrames[1])) {
        raycastPos.x = this->actor.world.pos.x;
        raycastPos.y = this->actor.world.pos.y + 20.0f;
        raycastPos.z = this->actor.world.pos.z;
        if (BgCheck_EntityRaycastDown3(&play->colCtx, &floorPoly, &floorBgId, &raycastPos) != 0.0f) {
            //! @bug should use `SurfaceType_GetSfxOffset` instead of `SurfaceType_GetMaterial`.
            // Most material and sfxOffsets share identical enum values,
            // so this will mostly result in the correct sfx played, but not in all cases, such as carpet and ice.
            this->floorSfxOffset = SurfaceType_GetMaterial(&play->colCtx, floorPoly, floorBgId);
            Player_PlayLandingSfx(this);
        }
    }
}

static PlayerAnimSfxEntry sCrawlspaceCrawlAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3028 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3030 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3038 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3040 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3048 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3050 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3058 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3060 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3068 },
};

/**
 * Update player's animation while entering the crawlspace.
 * Once inside, stop all player animations and update player's movement.
 */
void Player_InsideCrawlspace(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE)) {
            // While inside a crawlspace, player's skeleton does not move
            if (this->skelAnime.moveFlags != 0) {
                this->skelAnime.moveFlags = 0;
                return;
            }

            if (!Player_TryLeavingCrawlspace(this, play)) {
                // Move forward and back while inside the crawlspace
                this->linearVelocity = sControlInput->rel.stick_y * 0.03f;
            }
        }
        return;
    }

    // Still entering crawlspace
    Player_PlayAnimSfx(this, sCrawlspaceCrawlAnimSfx);
}

static PlayerAnimSfxEntry sExitCrawlspaceAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x300A },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3012 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x301A },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3022 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3034 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x303C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3044 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x304C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3054 },
};

/**
 * Update player's animation while leaving the crawlspace.
 */
void Player_ExitCrawlspace(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        // Player is finished exiting the crawlspace and control is returned
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_CRAWLING;
        return;
    }

    // Continue animation of leaving crawlspace
    Player_PlayAnimSfx(this, sExitCrawlspaceAnimSfx);
}

static Vec3f sHorseDismountRaycastOffset[] = {
    { 40.0f, 0.0f, 0.0f },
    { -40.0f, 0.0f, 0.0f },
};

static Vec3f sHorseLineTestTopOffset[] = {
    { 60.0f, 20.0f, 0.0f },
    { -60.0f, 20.0f, 0.0f },
};

static Vec3f sHorseLineTestBottomOffset[] = {
    { 60.0f, -20.0f, 0.0f },
    { -60.0f, -20.0f, 0.0f },
};

s32 Player_CanDismountHorse(PlayState* play, Player* this, s32 mountedLeftOfHorse, f32* floorPosY) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 posYMax;
    f32 posYMin;
    Vec3f raycastPos;
    Vec3f wallPos;
    CollisionPoly* wallPoly;
    s32 wallBgId;

    posYMax = rideActor->actor.world.pos.y + 20.0f;
    posYMin = rideActor->actor.world.pos.y - 20.0f;

    *floorPosY =
        Player_RaycastFloorWithOffset2(play, this, &sHorseDismountRaycastOffset[mountedLeftOfHorse], &raycastPos);

    return (posYMin < *floorPosY) && (*floorPosY < posYMax) &&
           !Player_WallLineTestWithOffset(play, this, &sHorseLineTestTopOffset[mountedLeftOfHorse], &wallPoly,
                                          &wallBgId, &wallPos) &&
           !Player_WallLineTestWithOffset(play, this, &sHorseLineTestBottomOffset[mountedLeftOfHorse], &wallPoly,
                                          &wallBgId, &wallPos);
}

s32 Player_SetupDismountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    s32 mountedLeftOfHorse;
    f32 floorPosY;

    if (this->genericTimer < 0) {
        this->genericTimer = 99;
    } else {
        mountedLeftOfHorse = (this->mountSide < 0) ? 0 : 1;
        if (!Player_CanDismountHorse(play, this, mountedLeftOfHorse, &floorPosY)) {
            mountedLeftOfHorse ^= 1;
            if (!Player_CanDismountHorse(play, this, mountedLeftOfHorse, &floorPosY)) {
                return 0;
            } else {
                this->mountSide = -this->mountSide;
            }
        }

        if ((play->csCtx.state == CS_STATE_IDLE) && (play->transitionMode == TRANS_MODE_OFF) &&
            (EN_HORSE_CHECK_1(rideActor) || EN_HORSE_CHECK_4(rideActor))) {
            this->stateFlags2 |= PLAYER_STATE2_CAN_DISMOUNT_HORSE;

            if (EN_HORSE_CHECK_1(rideActor) ||
                (EN_HORSE_CHECK_4(rideActor) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A))) {
                rideActor->actor.child = NULL;
                Player_SetActionFuncPreserveMoveFlags(play, this, Player_DismountHorse, 0);
                this->rideOffsetY = floorPosY - rideActor->actor.world.pos.y;
                Player_PlayAnimOnce(play, this,
                                    (this->mountSide < 0) ? &gPlayerAnim_link_uma_left_down
                                                          : &gPlayerAnim_link_uma_right_down);
                return 1;
            }
        }
    }

    return 0;
}

// Something to do with offseting the player when getting on/off the horse, but no visible difference to the player's
// model/shape
void func_8084CBF4(Player* this, f32 arg1, f32 arg2) {
    f32 temp;
    f32 dir;

    if ((this->rideOffsetY != 0.0f) && (arg2 <= this->skelAnime.curFrame)) {
        if (arg1 < fabsf(this->rideOffsetY)) {
            if (this->rideOffsetY >= 0.0f) {
                dir = 1;
            } else {
                dir = -1;
            }
            temp = dir * arg1;
        } else {
            temp = this->rideOffsetY;
        }
        this->actor.world.pos.y += temp;
        this->rideOffsetY -= temp;
    }
}

static LinkAnimationHeader* sHorseMoveAnims[] = {
    &gPlayerAnim_link_uma_anim_stop,
    &gPlayerAnim_link_uma_anim_stand,
    &gPlayerAnim_link_uma_anim_walk,
    &gPlayerAnim_link_uma_anim_slowrun,
    &gPlayerAnim_link_uma_anim_fastrun,
    &gPlayerAnim_link_uma_anim_jump100,
    &gPlayerAnim_link_uma_anim_jump200,
    NULL,
    NULL,
};

static LinkAnimationHeader* sHorseWhipAnims[] = {
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_slowrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    NULL,
    NULL,
};

static LinkAnimationHeader* sHorseIdleAnims[] = {
    &gPlayerAnim_link_uma_wait_3,
    &gPlayerAnim_link_uma_wait_1,
    &gPlayerAnim_link_uma_wait_2,
};

static u8 sMountSfxFrames[2][2] = {
    { 32, 58 },
    { 25, 42 },
};

static Vec3s sRideHorsePrevTransl = { -69, 7146, -266 };

static PlayerAnimSfxEntry sHorseIdleAnimSfx[] = {
    { NA_SE_PL_CALM_HIT, 0x830 }, { NA_SE_PL_CALM_HIT, 0x83A },  { NA_SE_PL_CALM_HIT, 0x844 },
    { NA_SE_PL_CALM_PAT, 0x85C }, { NA_SE_PL_CALM_PAT, 0x86E },  { NA_SE_PL_CALM_PAT, 0x87E },
    { NA_SE_PL_CALM_PAT, 0x884 }, { NA_SE_PL_CALM_PAT, -0x888 },
};

void Player_RideHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    u8* mountSfxFrames;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    func_8084CBF4(this, 1.0f, 10.0f);

    if (this->genericTimer == 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->skelAnime.animation = &gPlayerAnim_link_uma_wait_1;
            this->genericTimer = 99;
            return;
        }

        mountSfxFrames = sMountSfxFrames[(this->mountSide < 0) ? 0 : 1];

        if (LinkAnimation_OnFrame(&this->skelAnime, mountSfxFrames[0])) {
            func_8002F7DC(&this->actor, NA_SE_PL_CLIMB_CLIFF);
            return;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, mountSfxFrames[1])) {
            func_8002DE74(play, this);
            func_8002F7DC(&this->actor, NA_SE_PL_SIT_ON_HORSE);
            return;
        }

        return;
    }

    func_8002DE74(play, this);
    this->skelAnime.prevTransl = sRideHorsePrevTransl;

    if ((rideActor->animationIdx != this->genericTimer) &&
        ((rideActor->animationIdx >= 2) || (this->genericTimer >= 2))) {
        if ((this->genericTimer = rideActor->animationIdx) < 2) {
            f32 rand = Rand_ZeroOne();
            s32 idleAnimIdx = 0;

            this->genericTimer = 1;

            if (rand < 0.1f) {
                idleAnimIdx = 2;
            } else if (rand < 0.2f) {
                idleAnimIdx = 1;
            }
            Player_PlayAnimOnce(play, this, sHorseIdleAnims[idleAnimIdx]);
        } else {
            this->skelAnime.animation = sHorseMoveAnims[this->genericTimer - 2];
            Animation_SetMorph(play, &this->skelAnime, 8.0f);
            if (this->genericTimer < 4) {
                Player_SetupHeldItemUpperActionFunc(play, this);
                this->genericVar = 0;
            }
        }
    }

    if (this->genericTimer == 1) {
        if ((D_808535E0 != 0) || Player_CheckActorTalkRequested(play)) {
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_uma_wait_3);
        } else if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->genericTimer = 99;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_uma_wait_1) {
            Player_PlayAnimSfx(this, sHorseIdleAnimSfx);
        }
    } else {
        this->skelAnime.curFrame = rideActor->curFrame;
        LinkAnimation_AnimateFrame(play, &this->skelAnime);
    }

    AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.morphTable,
                                this->skelAnime.jointTable);

    if ((play->csCtx.state != CS_STATE_IDLE) || (this->csMode != PLAYER_CSMODE_NONE)) {
        if (this->csMode == PLAYER_CSMODE_END) {
            this->csMode = PLAYER_CSMODE_NONE;
        }
        this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
        this->genericVar = 0;
    } else if ((this->genericTimer < 2) || (this->genericTimer >= 4)) {
        D_808535E0 = Player_SetupCurrentUpperAction(this, play);
        if (D_808535E0 != 0) {
            this->genericVar = 0;
        }
    }

    this->actor.world.pos.x = rideActor->actor.world.pos.x + rideActor->riderPos.x;
    this->actor.world.pos.y = (rideActor->actor.world.pos.y + rideActor->riderPos.y) - 27.0f;
    this->actor.world.pos.z = rideActor->actor.world.pos.z + rideActor->riderPos.z;

    this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

    if ((this->csMode != PLAYER_CSMODE_NONE) ||
        (!Player_CheckActorTalkRequested(play) &&
         ((rideActor->actor.speedXZ != 0.0f) || !Player_SetupSpeakOrCheck(this, play)) &&
         !Player_SetupRollOrPutAway(this, play))) {
        if (D_808535E0 == 0) {
            if (this->genericVar != 0) {
                if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
                    rideActor->stateFlags &= ~ENHORSE_FLAG_8;
                    this->genericVar = 0;
                }

                if (this->skelAnimeUpper.animation == &gPlayerAnim_link_uma_stop_muti) {
                    if (LinkAnimation_OnFrame(&this->skelAnimeUpper, 23.0f)) {
                        func_8002F7DC(&this->actor, NA_SE_IT_LASH);
                        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
                    }

                    AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                                this->skelAnimeUpper.jointTable);
                } else {
                    if (LinkAnimation_OnFrame(&this->skelAnimeUpper, 10.0f)) {
                        func_8002F7DC(&this->actor, NA_SE_IT_LASH);
                        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
                    }

                    AnimationContext_SetCopyTrue(play, this->skelAnime.limbCount, this->skelAnime.jointTable,
                                                 this->skelAnimeUpper.jointTable, D_80853410);
                }
            } else {
                LinkAnimationHeader* anim = NULL;

                if (EN_HORSE_CHECK_3(rideActor)) {
                    anim = &gPlayerAnim_link_uma_stop_muti;
                } else if (EN_HORSE_CHECK_2(rideActor)) {
                    if ((this->genericTimer >= 2) && (this->genericTimer != 99)) {
                        anim = sHorseWhipAnims[this->genericTimer - 2];
                    }
                }

                if (anim != NULL) {
                    LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, anim);
                    this->genericVar = 1;
                }
            }
        }

        if (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE) {
            if (!Player_SetupCameraMode(play, this) || CHECK_BTN_ANY(sControlInput->press.button, BTN_A) ||
                Player_IsZTargeting(this)) {
                this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
                this->stateFlags1 &= ~PLAYER_STATE1_IN_FIRST_PERSON_MODE;
            } else {
                this->upperBodyRot.y =
                    Player_SetFirstPersonAimLookAngles(play, this, 1, -5000) - this->actor.shape.rot.y;
                this->upperBodyRot.y += 5000;
                this->upperBodyYawOffset = -5000;
            }
            return;
        }

        if ((this->csMode != PLAYER_CSMODE_NONE) ||
            (!Player_SetupDismountHorse(this, play) && !Player_SetupItemCutsceneOrFirstPerson(this, play))) {
            if (this->targetActor != NULL) {
                if (Actor_PlayerIsAimingReadyFpsItem(this) != 0) {
                    this->upperBodyRot.y = Player_LookAtTargetActor(this, 1) - this->actor.shape.rot.y;
                    this->upperBodyRot.y = CLAMP(this->upperBodyRot.y, -0x4AAA, 0x4AAA);
                    this->actor.focus.rot.y = this->actor.shape.rot.y + this->upperBodyRot.y;
                    this->upperBodyRot.y += 5000;
                    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y;
                } else {
                    Player_LookAtTargetActor(this, 0);
                }
            } else {
                if (Actor_PlayerIsAimingReadyFpsItem(this) != 0) {
                    this->upperBodyRot.y =
                        Player_SetFirstPersonAimLookAngles(play, this, 1, -5000) - this->actor.shape.rot.y;
                    this->upperBodyRot.y += 5000;
                    this->upperBodyYawOffset = -5000;
                }
            }
        }
    }
}

static PlayerAnimSfxEntry sHorseDismountAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x2800 },
    { NA_SE_PL_GET_OFF_HORSE, 0x80A },
    { NA_SE_PL_SLIPDOWN, -0x819 },
};

void Player_DismountHorse(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    func_8084CBF4(this, 1.0f, 10.0f);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        EnHorse* rideActor = (EnHorse*)this->rideActor;

        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_RIDING_HORSE;
        this->actor.parent = NULL;
        AREG(6) = 0;

        if (Flags_GetEventChkInf(EVENTCHKINF_EPONA_OBTAINED) || (DREG(1) != 0)) {
            gSaveContext.horseData.pos.x = rideActor->actor.world.pos.x;
            gSaveContext.horseData.pos.y = rideActor->actor.world.pos.y;
            gSaveContext.horseData.pos.z = rideActor->actor.world.pos.z;
            gSaveContext.horseData.angle = rideActor->actor.shape.rot.y;
        }
    } else {
        Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_NORMAL0);

        if (this->mountSide < 0) {
            sHorseDismountAnimSfx[0].field = 0x2828;
        } else {
            sHorseDismountAnimSfx[0].field = 0x281D;
        }
        Player_PlayAnimSfx(this, sHorseDismountAnimSfx);
    }
}

static PlayerAnimSfxEntry sSwimAnimSfx[] = {
    { NA_SE_PL_SWIM, -0x800 },
};

void Player_SetupSwimMovement(Player* this, f32* linearVelocity, f32 swimVelocity, s16 swimYaw) {
    Player_UpdateSwimMovement(this, linearVelocity, swimVelocity, swimYaw);
    Player_PlayAnimSfx(this, sSwimAnimSfx);
}

void Player_SetupSwim(PlayState* play, Player* this, s16 swimYaw) {
    Player_SetActionFunc(play, this, Player_Swim, 0);
    this->actor.shape.rot.y = this->currentYaw = swimYaw;
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_SetupZTargetSwimming(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_ZTargetSwimming, 0);
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_UpdateSwimIdle(Player* this, PlayState* play) {
    f32 swimVelocity;
    s16 swimYaw;

    Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_swimer_swim_wait);
    Player_SetVerticalWaterVelocity(this);

    if (!Player_CheckActorTalkRequested(play) && !Player_SetupSubAction(play, this, sSwimSubActions, 1) &&
        !Player_SetupDive(play, this, sControlInput)) {
        if (this->attentionMode != PLAYER_ATTENTIONMODE_C_UP) {
            this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
        }

        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            swimVelocity = 0.0f;
            swimYaw = this->actor.shape.rot.y;

            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_SetupReturnToStandStillSetAnim(
                    this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_short_landing, this->modelAnimType), play);
                Player_PlayLandingSfx(this);
            }
        } else {
            Player_GetTargetVelocityAndYaw(this, &swimVelocity, &swimYaw, 0.0f, play);

            if (swimVelocity != 0.0f) {
                s16 temp = this->actor.shape.rot.y - swimYaw;

                if ((ABS(temp) > 0x6000) && !Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                    return;
                }

                if (Player_IsZTargetingSetupStartUnfriendly(this)) {
                    Player_SetupZTargetSwimming(play, this);
                } else {
                    Player_SetupSwim(play, this, swimYaw);
                }
            }
        }

        Player_UpdateSwimMovement(this, &this->linearVelocity, swimVelocity, swimYaw);
    }
}

void Player_SpawnSwimming(Player* this, PlayState* play) {
    if (!Player_SetupItemCutsceneOrFirstPerson(this, play)) {
        this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

        Player_PlaySwimAnim(play, this, NULL, this->linearVelocity);
        Player_SetVerticalWaterVelocity(this);

        if (DECR(this->genericTimer) == 0) {
            Player_SetupSwimIdle(play, this);
        }
    }
}

void Player_Swim(Player* this, PlayState* play) {
    f32 swimVelocity;
    s16 swimYaw;
    s16 yawDiff;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    Player_PlaySwimAnim(play, this, sControlInput, this->linearVelocity);
    Player_SetVerticalWaterVelocity(this);

    if (!Player_SetupSubAction(play, this, sSwimSubActions, 1) && !Player_SetupDive(play, this, sControlInput)) {
        Player_GetTargetVelocityAndYaw(this, &swimVelocity, &swimYaw, 0.0f, play);

        yawDiff = this->actor.shape.rot.y - swimYaw;
        if ((swimVelocity == 0.0f) || (ABS(yawDiff) > DEG_TO_BINANG(135.0f)) ||
            (this->currentBoots == PLAYER_BOOTS_IRON)) {
            Player_SetupSwimIdle(play, this);
        } else if (Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupZTargetSwimming(play, this);
        }

        Player_SetupSwimMovement(this, &this->linearVelocity, swimVelocity, swimYaw);
    }
}

s32 Player_SetupZTargetSwimAnims(PlayState* play, Player* this, f32* targetVelocity, s16* targetYaw) {
    LinkAnimationHeader* anim;
    s16 yawDiff;
    s32 moveDir;

    yawDiff = this->currentYaw - *targetYaw;

    if (ABS(yawDiff) > DEG_TO_BINANG(135.0f)) {
        anim = &gPlayerAnim_link_swimer_swim_wait;

        if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
            this->currentYaw = *targetYaw;
        } else {
            *targetVelocity = 0.0f;
            *targetYaw = this->currentYaw;
        }
    } else {
        moveDir = Player_GetFriendlyZTargetingMoveDirection(this, targetVelocity, targetYaw, play);

        if (moveDir > 0) {
            anim = &gPlayerAnim_link_swimer_swim;
        } else if (moveDir < 0) {
            anim = &gPlayerAnim_link_swimer_back_swim;
        } else if ((yawDiff = this->actor.shape.rot.y - *targetYaw) > 0) {
            anim = &gPlayerAnim_link_swimer_Rside_swim;
        } else {
            anim = &gPlayerAnim_link_swimer_Lside_swim;
        }
    }

    if (anim != this->skelAnime.animation) {
        Player_ChangeAnimLongMorphLoop(play, this, anim);
        return 1;
    }

    return 0;
}

void Player_ZTargetSwimming(Player* this, PlayState* play) {
    f32 swimVelocity;
    s16 swimYaw;

    Player_PlaySwimAnim(play, this, sControlInput, this->linearVelocity);
    Player_SetVerticalWaterVelocity(this);

    if (!Player_SetupSubAction(play, this, sSwimSubActions, 1) && !Player_SetupDive(play, this, sControlInput)) {
        Player_GetTargetVelocityAndYaw(this, &swimVelocity, &swimYaw, 0.0f, play);

        if (swimVelocity == 0.0f) {
            Player_SetupSwimIdle(play, this);
        } else if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupSwim(play, this, swimYaw);
        } else {
            Player_SetupZTargetSwimAnims(play, this, &swimVelocity, &swimYaw);
        }

        Player_SetupSwimMovement(this, &this->linearVelocity, swimVelocity, swimYaw);
    }
}

void Player_UpdateDiveMovement(PlayState* play, Player* this, f32 swimNextVelocity) {
    f32 swimTargetVelocity;
    s16 swimTargetYaw;

    Player_GetTargetVelocityAndYaw(this, &swimTargetVelocity, &swimTargetYaw, 0.0f, play);
    Player_UpdateSwimMovement(this, &this->linearVelocity, swimTargetVelocity * 0.5f, swimTargetYaw);
    Player_UpdateSwimMovement(this, &this->actor.velocity.y, swimNextVelocity, this->currentYaw);
}

void Player_Dive(Player* this, PlayState* play) {
    f32 sp2C;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    this->actor.gravity = 0.0f;
    Player_SetupCurrentUpperAction(this, play);

    if (!Player_SetupItemCutsceneOrFirstPerson(this, play)) {
        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            Player_SetupSwimIdle(play, this);
            return;
        }

        if (this->genericVar == 0) {
            if (this->genericTimer == 0) {
                if (LinkAnimation_Update(play, &this->skelAnime) ||
                    ((this->skelAnime.curFrame >= 22.0f) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))) {
                    Player_RiseFromDive(play, this);
                } else if (LinkAnimation_OnFrame(&this->skelAnime, 20.0f) != 0) {
                    this->actor.velocity.y = -2.0f;
                }

                Player_StepLinearVelocityToZero(this);
                return;
            }

            Player_PlaySwimAnim(play, this, sControlInput, this->actor.velocity.y);
            this->shapePitchOffset = DEG_TO_BINANG(87.891f);

            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) && !Player_SetupGetItemOrHoldBehavior(this, play) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
                (this->actor.yDistToWater < sScaleDiveDists[CUR_UPG_VALUE(UPG_SCALE)])) {
                Player_UpdateDiveMovement(play, this, -2.0f);
            } else {
                this->genericVar++;
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
            }
        } else if (this->genericVar == 1) {
            LinkAnimation_Update(play, &this->skelAnime);
            Player_SetVerticalWaterVelocity(this);

            if (this->shapePitchOffset < DEG_TO_BINANG(54.932f)) {
                this->genericVar++;
                this->genericTimer = this->actor.yDistToWater;
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
            }
        } else if (!Player_SetupDive(play, this, sControlInput)) {
            sp2C = (this->genericTimer * 0.018f) + 4.0f;

            if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
                sControlInput = NULL;
            }

            Player_PlaySwimAnim(play, this, sControlInput, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->shapePitchOffset, -DEG_TO_BINANG(54.932f), DEG_TO_BINANG(4.395f));

            if (sp2C > 8.0f) {
                sp2C = 8.0f;
            }

            Player_UpdateDiveMovement(play, this, sp2C);
        }
    }
}

void Player_EndGetItem(PlayState* play, Player* this) {
    this->giDrawIdPlusOne = 0;
    this->stateFlags1 &= ~(PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR);
    this->getItemId = GI_NONE;
    func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
}

void Player_SetupEndGetItem(PlayState* play, Player* this) {
    Player_EndGetItem(play, this);
    Player_AddRootYawToShapeYaw(this);
    Player_SetupStandingStillNoMorph(this, play);
    this->currentYaw = this->actor.shape.rot.y;
}

s32 Player_SetupGetItemText(PlayState* play, Player* this) {
    GetItemEntry* giEntry;
    s32 temp1;
    s32 temp2;

    if (this->getItemId == GI_NONE) {
        return 1;
    }

    if (this->genericVar == 0) {
        giEntry = &sGetItemTable[this->getItemId - 1];
        this->genericVar = 1;

        Message_StartTextbox(play, giEntry->textId, &this->actor);
        Item_Give(play, giEntry->itemId);

        if (((this->getItemId >= GI_RUPEE_GREEN) && (this->getItemId <= GI_RUPEE_RED)) ||
            ((this->getItemId >= GI_RUPEE_PURPLE) && (this->getItemId <= GI_RUPEE_GOLD)) ||
            ((this->getItemId >= GI_RUPEE_GREEN_LOSE) && (this->getItemId <= GI_RUPEE_PURPLE_LOSE)) ||
            (this->getItemId == GI_RECOVERY_HEART)) {
            Audio_PlaySfxGeneral(NA_SE_SY_GET_BOXITEM, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        } else {
            if ((this->getItemId == GI_HEART_CONTAINER_2) || (this->getItemId == GI_HEART_CONTAINER) ||
                ((this->getItemId == GI_HEART_PIECE) &&
                 ((gSaveContext.inventory.questItems & 0xF0000000) == (4 << QUEST_HEART_PIECE_COUNT)))) {
                temp1 = NA_BGM_HEART_GET | 0x900;
            } else {
                temp1 = temp2 = (this->getItemId == GI_HEART_PIECE) ? NA_BGM_SMALL_ITEM_GET : NA_BGM_ITEM_GET | 0x900;
            }
            Audio_PlayFanfare(temp1);
        }
    } else {
        if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
            if (this->getItemId == GI_SILVER_GAUNTLETS) {
                play->nextEntranceIndex = ENTR_DESERT_COLOSSUS_0;
                play->transitionTrigger = TRANS_TRIGGER_START;
                gSaveContext.nextCutsceneIndex = 0xFFF1;
                play->transitionType = TRANS_TYPE_SANDSTORM_END;
                this->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
                Player_SetupPlayerCutscene(play, NULL, PLAYER_CSMODE_WAIT);
            }
            this->getItemId = GI_NONE;
        }
    }

    return 0;
}

void Player_GetItemInWater(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) || Player_SetupGetItemText(play, this)) {
            Player_EndGetItem(play, this);
            Player_SetupSwimIdle(play, this);
            Player_ResetSubCam(play, this);
        }
    } else {
        if ((this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
            Player_SetGetItemDrawIdPlusOne(this, play);
            Player_ResetSubCam(play, this);
            Player_SetCameraTurnAround(play, 8);
        } else if (LinkAnimation_OnFrame(&this->skelAnime, 5.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
        }
    }

    Player_SetVerticalWaterVelocity(this);
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_DamagedSwim(Player* this, PlayState* play) {
    Player_SetVerticalWaterVelocity(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupSwimIdle(play, this);
    }

    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_Drown(Player* this, PlayState* play) {
    Player_SetVerticalWaterVelocity(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndDie(play, this);
    }

    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

static s16 sWarpSongEntrances[] = {
    ENTR_SACRED_FOREST_MEADOW_2,
    ENTR_DEATH_MOUNTAIN_CRATER_4,
    ENTR_LAKE_HYLIA_8,
    ENTR_DESERT_COLOSSUS_5,
    ENTR_GRAVEYARD_7,
    ENTR_TEMPLE_OF_TIME_7,
};

void Player_PlayOcarina(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_normal_okarina_swing);
        this->genericTimer = 1;
        if (this->stateFlags2 & (PLAYER_STATE2_NEAR_OCARINA_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR)) {
            this->stateFlags2 |= PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR;
        } else {
            func_8010BD58(play, OCARINA_ACTION_FREE_PLAY);
        }
        return;
    }

    if (this->genericTimer == 0) {
        return;
    }

    if (play->msgCtx.ocarinaMode == OCARINA_MODE_04) {
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));

        if ((this->talkActor != NULL) && (this->talkActor == this->ocarinaActor)) {
            Player_StartTalkingWithActor(play, this->talkActor);
        } else if (this->naviTextId < 0) {
            this->talkActor = this->naviActor;
            this->naviActor->textId = -this->naviTextId;
            Player_StartTalkingWithActor(play, this->talkActor);
        } else if (!Player_SetupItemCutsceneOrFirstPerson(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_okarina_end, play);
        }

        this->stateFlags2 &= ~(PLAYER_STATE2_NEAR_OCARINA_ACTOR | PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR |
                               PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
        this->ocarinaActor = NULL;
    } else if (play->msgCtx.ocarinaMode == OCARINA_MODE_02) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex = sWarpSongEntrances[play->msgCtx.lastPlayedSong];
        gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams = 0x5FF;
        gSaveContext.respawn[RESPAWN_MODE_RETURN].data = play->msgCtx.lastPlayedSong;

        this->csMode = PLAYER_CSMODE_NONE;
        this->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;

        Player_SetupPlayerCutscene(play, NULL, PLAYER_CSMODE_WAIT);
        play->mainCamera.unk_14C &= ~8;

        this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
        this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_GENERAL;

        if (Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, DEMOKANKYO_WARP_OUT) ==
            NULL) {
            Environment_WarpSongLeave(play);
        }

        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    }
}

void Player_ThrowDekuNut(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_light_bom_end, play);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Inventory_ChangeAmmo(ITEM_DEKU_NUT, -1);
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ARROW, this->bodyPartsPos[PLAYER_BODYPART_R_HAND].x,
                    this->bodyPartsPos[PLAYER_BODYPART_R_HAND].y, this->bodyPartsPos[PLAYER_BODYPART_R_HAND].z, 4000,
                    this->actor.shape.rot.y, 0, ARROW_NUT);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }

    Player_StepLinearVelocityToZero(this);
}

static PlayerAnimSfxEntry sChildOpenChestAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3857 },
    { NA_SE_VO_LI_CLIMB_END, 0x2057 },
    { NA_SE_VO_LI_AUTO_JUMP, 0x2045 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x287B },
};

void Player_GetItem(Player* this, PlayState* play) {
    s32 cond;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericTimer != 0) {
            if (this->genericTimer >= 2) {
                this->genericTimer--;
            }

            if (Player_SetupGetItemText(play, this) && (this->genericTimer == 1)) {
                cond = ((this->talkActor != NULL) && (this->exchangeItemId < 0)) ||
                       (this->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA);

                if (cond || (gSaveContext.healthAccumulator == 0)) {
                    if (cond) {
                        Player_EndGetItem(play, this);
                        this->exchangeItemId = EXCH_ITEM_NONE;

                        if (Player_SetupForcePullOcarina(play, this) == 0) {
                            Player_StartTalkingWithActor(play, this->talkActor);
                        }
                    } else {
                        Player_SetupEndGetItem(play, this);
                    }
                }
            }
        } else {
            Player_EndAnimMovement(this);

            if (this->getItemId == GI_ICE_TRAP) {
                this->stateFlags1 &= ~(PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR);

                if (this->getItemId != GI_ICE_TRAP) {
                    Actor_Spawn(&play->actorCtx, play, ACTOR_EN_CLEAR_TAG, this->actor.world.pos.x,
                                this->actor.world.pos.y + 100.0f, this->actor.world.pos.z, 0, 0, 0, 0);
                    Player_SetupStandingStillNoMorph(this, play);
                } else {
                    this->actor.colChkInfo.damage = 0;
                    Player_SetupDamage(play, this, PLAYER_DMGREACTION_FROZEN, 0.0f, 0.0f, 0, 20);
                }
                return;
            }

            if (this->skelAnime.animation == &gPlayerAnim_link_normal_box_kick) {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_get_itemB);
            } else {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_get_itemA);
            }

            this->genericTimer = 2;
            Player_SetCameraTurnAround(play, 9);
        }
    } else {
        if (this->genericTimer == 0) {
            if (!LINK_IS_ADULT) {
                Player_PlayAnimSfx(this, sChildOpenChestAnimSfx);
            }
            return;
        }

        if (this->skelAnime.animation == &gPlayerAnim_link_demo_get_itemB) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) + 0x8000, 4000);
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, 21.0f)) {
            Player_SetGetItemDrawIdPlusOne(this, play);
        }
    }
}

static PlayerAnimSfxEntry sSwingSwordAnimSfx[] = {
    { NA_SE_IT_MASTER_SWORD_SWING, -0x83C },
};

void Player_PlaySwingSwordSfx(Player* this) {
    Player_PlayAnimSfx(this, sSwingSwordAnimSfx);
}

static PlayerAnimSfxEntry sChildEndTimeTravelAnimSfx[] = {
    { NA_SE_VO_LI_AUTO_JUMP, 0x2005 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x280F },
};

void Player_EndTimeTravel(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericVar == 0) {
            if (DECR(this->genericTimer) == 0) {
                this->genericVar = 1;
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
        }
    } else {
        if (LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 158.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
            return;
        }

        if (!LINK_IS_ADULT) {
            Player_PlayAnimSfx(this, sChildEndTimeTravelAnimSfx);
        } else {
            Player_PlaySwingSwordSfx(this);
        }
    }
}

static u8 sBottleDrinkEffects[] = {
    PLAYER_BOTTLEDRINKEFFECT_HEAL_STRONG,                                       // PLAYER_IA_BOTTLE_POTION_RED
    PLAYER_BOTTLEDRINKEFFECT_HEAL_STRONG | PLAYER_BOTTLEDRINKEFFECT_FILL_MAGIC, // PLAYER_IA_BOTTLE_POTION_BLUE
    PLAYER_BOTTLEDRINKEFFECT_FILL_MAGIC,                                        // PLAYER_IA_BOTTLE_POTION_GREEN
    PLAYER_BOTTLEDRINKEFFECT_HEAL_WEAK,                                         // PLAYER_IA_BOTTLE_MILK
    PLAYER_BOTTLEDRINKEFFECT_HEAL_WEAK,                                         // PLAYER_IA_BOTTLE_MILK_HALF
};

void Player_DrinkFromBottle(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericTimer == 0) {
            if (this->itemAction == PLAYER_IA_BOTTLE_POE) {
                s32 rand = Rand_S16Offset(-1, 3);

                if (rand == 0) {
                    rand = 3;
                }

                if ((rand < 0) && (gSaveContext.health <= 0x10)) {
                    rand = 3;
                }

                if (rand < 0) {
                    Health_ChangeBy(play, -0x10);
                } else {
                    gSaveContext.healthAccumulator = rand * 0x10;
                }
            } else {
                s32 bottleDrinkEffects = sBottleDrinkEffects[this->itemAction - PLAYER_IA_BOTTLE_POTION_RED];

                if (bottleDrinkEffects & PLAYER_BOTTLEDRINKEFFECT_HEAL_STRONG) {
                    gSaveContext.healthAccumulator = 0x140;
                }

                if (bottleDrinkEffects & PLAYER_BOTTLEDRINKEFFECT_FILL_MAGIC) {
                    Magic_Fill(play);
                }

                if (bottleDrinkEffects & PLAYER_BOTTLEDRINKEFFECT_HEAL_WEAK) {
                    gSaveContext.healthAccumulator = 0x50;
                }
            }

            Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_bottle_drink_demo_wait);
            this->genericTimer = 1;
            return;
        }

        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    } else if (this->genericTimer == 1) {
        if ((gSaveContext.healthAccumulator == 0) && (gSaveContext.magicState != MAGIC_STATE_FILL)) {
            Player_ChangeAnimSlowedMorphToLastFrame(play, this, &gPlayerAnim_link_bottle_drink_demo_end);
            this->genericTimer = 2;
            Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_EMPTY, PLAYER_IA_BOTTLE);
        }
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DRINK - SFX_FLAG);
    } else if ((this->genericTimer == 2) && LinkAnimation_OnFrame(&this->skelAnime, 29.0f)) {
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
    }
}

static BottleCatchInfo sBottleCatchInfos[] = {
    { ACTOR_EN_ELF, ITEM_BOTTLE_FAIRY, PLAYER_IA_BOTTLE_FAIRY, 0x46 },
    { ACTOR_EN_FISH, ITEM_BOTTLE_FISH, PLAYER_IA_BOTTLE_FISH, 0x47 },
    { ACTOR_EN_ICE_HONO, ITEM_BOTTLE_BLUE_FIRE, PLAYER_IA_BOTTLE_FIRE, 0x5D },
    { ACTOR_EN_INSECT, ITEM_BOTTLE_BUG, PLAYER_IA_BOTTLE_BUG, 0x7A },
};

void Player_SwingBottle(Player* this, PlayState* play) {
    BottleSwingAnimInfo* bottleSwingAnims;
    BottleCatchInfo* catchInfo;
    s32 temp;
    s32 i;

    bottleSwingAnims = &sBottleSwingAnims[this->genericTimer];
    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericVar != 0) {
            if (this->genericTimer == 0) {
                Message_StartTextbox(play, sBottleCatchInfos[this->genericVar - 1].textId, &this->actor);
                Audio_PlayFanfare(NA_BGM_ITEM_GET | 0x900);
                this->genericTimer = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->genericVar = 0;
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
        }
    } else {
        if (this->genericVar == 0) {
            temp = this->skelAnime.curFrame - bottleSwingAnims->unk_08;

            if (temp >= 0) {
                if (bottleSwingAnims->unk_09 >= temp) {
                    if (this->genericTimer != 0) {
                        if (temp == 0) {
                            func_8002F7DC(&this->actor, NA_SE_IT_SCOOP_UP_WATER);
                        }
                    }

                    if (this->interactRangeActor != NULL) {
                        catchInfo = &sBottleCatchInfos[0];
                        for (i = 0; i < ARRAY_COUNT(sBottleCatchInfos); i++, catchInfo++) {
                            if (this->interactRangeActor->id == catchInfo->actorId) {
                                break;
                            }
                        }

                        if (i < ARRAY_COUNT(sBottleCatchInfos)) {
                            this->genericVar = i + 1;
                            this->genericTimer = 0;
                            this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                            this->interactRangeActor->parent = &this->actor;
                            Player_UpdateBottleHeld(play, this, catchInfo->itemId, ABS(catchInfo->itemAction));
                            Player_PlayAnimOnceSlowed(play, this, bottleSwingAnims->bottleCatchAnim);
                            Player_SetCameraTurnAround(play, 4);
                        }
                    }
                }
            }
        }
    }

    //! @bug If the animation is changed at any point above (such as by Player_SetupStandingStillNoMorph() or
    //! Player_PlayAnimOnceSlowed()), it will change the curFrame to 0. This causes this flag to be set for one frame,
    //! at a time when it does not look like Player is swinging the bottle.
    if (this->skelAnime.curFrame <= 7.0f) {
        this->stateFlags1 |= PLAYER_STATE1_SWINGING_BOTTLE;
    }
}

static Vec3f sBottleFairyPosOffset = { 0.0f, 0.0f, 5.0f };

void Player_HealWithFairy(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 37.0f)) {
        Player_SpawnFairy(play, this, &this->leftHandPos, &sBottleFairyPosOffset, FAIRY_REVIVE_BOTTLE);
        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_EMPTY, PLAYER_IA_BOTTLE);
        func_8002F7DC(&this->actor, NA_SE_EV_BOTTLE_CAP_OPEN);
        func_8002F7DC(&this->actor, NA_SE_EV_FIATY_HEAL - SFX_FLAG);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 47.0f)) {
        gSaveContext.healthAccumulator = 0x140;
    }
}

static BottleDropInfo D_80854A28[] = {
    { ACTOR_EN_FISH, FISH_DROPPED },
    { ACTOR_EN_ICE_HONO, 0 },
    { ACTOR_EN_INSECT, INSECT_TYPE_FIRST_DROPPED },
};

static PlayerAnimSfxEntry sBottleDropAnimSfx[] = {
    { NA_SE_VO_LI_AUTO_JUMP, 0x2026 },
    { NA_SE_EV_BOTTLE_CAP_OPEN, -0x828 },
};

void Player_DropItemFromBottle(Player* this, PlayState* play) {
    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 76.0f)) {
        BottleDropInfo* dropInfo = &D_80854A28[this->itemAction - PLAYER_IA_BOTTLE_FISH];

        Actor_Spawn(&play->actorCtx, play, dropInfo->actorId,
                    (Math_SinS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.x, this->leftHandPos.y,
                    (Math_CosS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.z, 0x4000, this->actor.shape.rot.y,
                    0, dropInfo->actorParams);

        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE_EMPTY, PLAYER_IA_BOTTLE);
        return;
    }

    Player_PlayAnimSfx(this, sBottleDropAnimSfx);
}

static PlayerAnimSfxEntry sExchangeItemAnimSfx[] = {
    { NA_SE_PL_PUT_OUT_ITEM, -0x81E },
};

void Player_PresentExchangeItem(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericTimer < 0) {
            Player_SetupStandingStillNoMorph(this, play);
        } else if (this->exchangeItemId == EXCH_ITEM_NONE) {
            Actor* targetActor = this->talkActor;

            this->giDrawIdPlusOne = 0;
            if (targetActor->textId != 0xFFFF) {
                this->actor.flags |= ACTOR_FLAG_8;
            }

            Player_StartTalkingWithActor(play, targetActor);
        } else {
            GetItemEntry* giEntry = &sGetItemTable[sExchangeGetItemIDs[this->exchangeItemId - 1] - 1];

            if (this->itemAction >= PLAYER_IA_ZELDAS_LETTER) {
                this->giDrawIdPlusOne = ABS(giEntry->gi);
            }

            if (this->genericTimer == 0) {
                Message_StartTextbox(play, this->actor.textId, &this->actor);

                if ((this->itemAction == PLAYER_IA_CHICKEN) || (this->itemAction == PLAYER_IA_POCKET_CUCCO)) {
                    func_8002F7DC(&this->actor, NA_SE_EV_CHICKEN_CRY_M);
                }

                this->genericTimer = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->actor.flags &= ~ACTOR_FLAG_8;
                this->giDrawIdPlusOne = 0;

                if (this->genericVar == 1) {
                    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_bottle_read_end);
                    this->genericTimer = -1;
                } else {
                    Player_SetupStandingStillNoMorph(this, play);
                }

                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        }
    } else if (this->genericTimer >= 0) {
        Player_PlayAnimSfx(this, sExchangeItemAnimSfx);
    }

    if ((this->genericVar == 0) && (this->targetActor != NULL)) {
        this->currentYaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_RestrainedByEnemy(Player* this, PlayState* play) {
    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_re_dead_attack_wait);
    }

    if (Player_MashTimerThresholdExceeded(this, 0, 100)) {
        Player_SetupStandingStillType(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
    }
}

void Player_SlipOnSlope(Player* this, PlayState* play) {
    CollisionPoly* floorPoly;
    f32 targetVelocity;
    f32 velocityIncrStep;
    f32 velocityDecrStep;
    s16 downwardSlopeYaw;
    s16 targetYaw;
    Vec3f slopeNormal;

    this->stateFlags2 |=
        PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    LinkAnimation_Update(play, &this->skelAnime);
    Player_SetupSpawnDustAtFeet(play, this);
    func_800F4138(&this->actor.projectedPos, NA_SE_PL_SLIP_LEVEL - SFX_FLAG, this->actor.speedXZ);

    if (Player_SetupItemCutsceneOrFirstPerson(this, play) == false) {
        floorPoly = this->actor.floorPoly;

        if (floorPoly == NULL) {
            Player_SetupFallFromLedge(this, play);
            return;
        }

        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);

        targetYaw = downwardSlopeYaw;
        if (this->genericVar != 0) {
            targetYaw = downwardSlopeYaw + 0x8000;
        }

        if (this->linearVelocity < 0) {
            downwardSlopeYaw += 0x8000;
        }

        targetVelocity = (1.0f - slopeNormal.y) * 40.0f;
        targetVelocity = CLAMP(targetVelocity, 0, 10.0f);
        velocityIncrStep = (targetVelocity * targetVelocity) * 0.015f;
        velocityDecrStep = slopeNormal.y * 0.01f;

        if (SurfaceType_GetFloorEffect(&play->colCtx, floorPoly, this->actor.floorBgId) != FLOOR_EFFECT_1) {
            targetVelocity = 0;
            velocityDecrStep = slopeNormal.y * 10.0f;
        }

        if (velocityIncrStep < 1.0f) {
            velocityIncrStep = 1.0f;
        }

        if (Math_AsymStepToF(&this->linearVelocity, targetVelocity, velocityIncrStep, velocityDecrStep) &&
            (targetVelocity == 0)) {
            LinkAnimationHeader* anim;

            if (this->genericVar == 0) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_down_slope_slip_end, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_up_slope_slip_end, this->modelAnimType);
            }
            Player_SetupReturnToStandStillSetAnim(this, anim, play);
        }

        Math_SmoothStepToS(&this->currentYaw, downwardSlopeYaw, 10, 4000, 800);
        Math_ScaledStepToS(&this->actor.shape.rot.y, targetYaw, 2000);
    }
}

void Player_SetDrawAndStartCutsceneAfterTimer(Player* this, PlayState* play) {
    if ((DECR(this->genericTimer) == 0) && Player_SetupCutscene(play, this)) {
        Player_CutsceneSetDraw(play, this, NULL);
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        Player_StartCutscene(this, play);
    }
}

void Player_SpawnFromWarpSong(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_SetDrawAndStartCutsceneAfterTimer, 0);
    this->genericTimer = 40;
    Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, DEMOKANKYO_WARP_IN);
}

void Player_SpawnFromBlueWarp(Player* this, PlayState* play) {
    s32 pad;

    if ((this->genericVar != 0) && (play->csCtx.frames < 305)) {
        this->actor.gravity = 0.0f;
        this->actor.velocity.y = 0.0f;
    } else if (sPlayerYDistToFloor < 150.0f) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (this->genericTimer == 0) {
                if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                    this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
                    Player_PlayLandingSfx(this);
                    this->genericTimer = 1;
                }
            } else {
                if ((play->sceneId == SCENE_KOKIRI_FOREST) && Player_SetupCutscene(play, this)) {
                    return;
                }
                Player_SetupStandingStillMorph(this, play);
            }
        }
        Math_SmoothStepToF(&this->actor.velocity.y, 2.0f, 0.3f, 8.0f, 0.5f);
    }

    if ((play->sceneId == SCENE_CHAMBER_OF_THE_SAGES) && Player_SetupCutscene(play, this)) {
        return;
    }

    if ((play->csCtx.state != CS_STATE_IDLE) && (play->csCtx.linkAction != NULL)) {
        f32 preservedPlayerPos = this->actor.world.pos.y;
        Player_CutsceneSetPosAndYaw(play, this, play->csCtx.linkAction);
        this->actor.world.pos.y = preservedPlayerPos;
    }
}

void Player_EnterGrotto(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);

    if ((this->genericTimer++ > 8) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {

        if (this->genericVar != 0) {
            if (play->sceneId == SCENE_ICE_CAVERN) {
                Play_TriggerRespawn(play);
                play->nextEntranceIndex = ENTR_ICE_CAVERN_0;
            } else if (this->genericVar < 0) {
                Play_TriggerRespawn(play);
            } else {
                Play_TriggerVoidOut(play);
            }

            play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
            func_80078884(NA_SE_OC_ABYSS);
        } else {
            play->transitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = 0xFF;
        }

        play->transitionTrigger = TRANS_TRIGGER_START;
    }
}

void Player_SetupOpenDoorFromSpawn(Player* this, PlayState* play) {
    Player_SetupOpenDoor(this, play);
}

void Player_JumpFromGrotto(Player* this, PlayState* play) {
    this->actor.gravity = -1.0f;

    LinkAnimation_Update(play, &this->skelAnime);

    if (this->actor.velocity.y < 0.0f) {
        Player_SetupFallFromLedge(this, play);
    } else if (this->actor.velocity.y < 6.0f) {
        Math_StepToF(&this->linearVelocity, 3.0f, 0.5f);
    }
}

void Player_ShootingGalleryPlay(Player* this, PlayState* play) {
    this->attentionMode = PLAYER_ATTENTIONMODE_AIMING;

    Player_SetupCameraMode(play, this);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_SetupCurrentUpperAction(this, play);

    this->upperBodyRot.y = Player_SetFirstPersonAimLookAngles(play, this, 1, 0) - this->actor.shape.rot.y;
    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y;

    if (play->shootingGalleryStatus < 0) {
        play->shootingGalleryStatus++;
        if (play->shootingGalleryStatus == 0) {
            Player_ClearLookAndAttention(this, play);
        }
    }
}

void Player_FrozenInIce(Player* this, PlayState* play) {
    if (this->genericVar >= 0) {
        if (this->genericVar < 6) {
            this->genericVar++;
        }

        if (Player_MashTimerThresholdExceeded(this, 1, 100)) {
            this->genericVar = -1;
            EffectSsIcePiece_SpawnBurst(play, &this->actor.world.pos, this->actor.scale.x);
            func_8002F7DC(&this->actor, NA_SE_PL_ICE_BROKEN);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_FROZEN_IN_ICE;
        }

        if ((play->gameplayFrames % 4) == 0) {
            Player_InflictDamageAndCheckForDeath(play, -1);
        }
    } else {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_SetupStandingStillType(this, play);
            Player_SetupInvincibilityNoDamageFlash(this, -20);
        }
    }
}

void Player_SetupElectricShock(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_RoundUpInvincibilityTimer(this);

    if (((this->genericTimer % 25) != 0) || Player_InflictDamage(play, this, -1)) {
        if (DECR(this->genericTimer) == 0) {
            Player_SetupStandingStillType(this, play);
        }
    }

    this->shockTimer = 40;
    func_8002F8F0(&this->actor, NA_SE_VO_LI_TAKEN_AWAY - SFX_FLAG + this->ageProperties->ageVoiceSfxOffset);
}

// Returns false if debug mode combo is detected and debug mode is set up, otherwise returns true
s32 Player_CheckNoDebugModeCombo(Player* this, PlayState* play) {
    sControlInput = &play->state.input[0];

    if ((CHECK_BTN_ALL(sControlInput->cur.button, BTN_A | BTN_L | BTN_R) &&
         CHECK_BTN_ALL(sControlInput->press.button, BTN_B)) ||
        (CHECK_BTN_ALL(sControlInput->cur.button, BTN_L) && CHECK_BTN_ALL(sControlInput->press.button, BTN_DRIGHT))) {

        sDebugModeFlag ^= 1;

        if (sDebugModeFlag) {
            Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_BOWARROWZ);
        }
    }

    if (sDebugModeFlag) {
        f32 speed;

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
            speed = 100.0f;
        } else {
            speed = 20.0f;
        }

        func_8006375C(3, 2, "DEBUG MODE");

        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_L)) {
            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
                this->actor.world.pos.y += speed;
            } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {
                this->actor.world.pos.y -= speed;
            }

            if (CHECK_BTN_ANY(sControlInput->cur.button, BTN_DUP | BTN_DLEFT | BTN_DDOWN | BTN_DRIGHT)) {
                s16 angle;
                s16 temp;

                angle = temp = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

                if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_DDOWN)) {
                    angle = temp + 0x8000;
                } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_DLEFT)) {
                    angle = temp + 0x4000;
                } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_DRIGHT)) {
                    angle = temp - 0x4000;
                }

                this->actor.world.pos.x += speed * Math_SinS(angle);
                this->actor.world.pos.z += speed * Math_CosS(angle);
            }
        }

        Player_StopMovement(this);

        this->actor.gravity = 0.0f;
        this->actor.velocity.z = 0.0f;
        this->actor.velocity.y = 0.0f;
        this->actor.velocity.x = 0.0f;

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_L) && CHECK_BTN_ALL(sControlInput->press.button, BTN_DLEFT)) {
            Flags_SetTempClear(play, play->roomCtx.curRoom.num);
        }

        Math_Vec3f_Copy(&this->actor.home.pos, &this->actor.world.pos);

        return 0;
    }

    return 1;
}

void Player_BowStringMoveAfterShot(Player* this) {
    this->unk_858 += this->bowStringScale;
    this->bowStringScale -= this->unk_858 * 5.0f;
    this->bowStringScale *= 0.3f;

    if (ABS(this->bowStringScale) < 0.00001f) {
        this->bowStringScale = 0.0f;
        if (ABS(this->unk_858) < 0.00001f) {
            this->unk_858 = 0.0f;
        }
    }
}

void Player_BunnyHoodPhysics(Player* this) {
    s32 pad;
    s16 sp2A;
    s16 sp28;
    s16 sp26;

    D_80858AC8.unk_06 -= D_80858AC8.unk_06 >> 3;
    D_80858AC8.unk_08 -= D_80858AC8.unk_08 >> 3;
    D_80858AC8.unk_06 += -D_80858AC8.unk_00 >> 2;
    D_80858AC8.unk_08 += -D_80858AC8.unk_02 >> 2;

    sp26 = this->actor.world.rot.y - this->actor.shape.rot.y;

    sp28 = (s32)(this->actor.speedXZ * -200.0f * Math_CosS(sp26) * (Rand_CenteredFloat(2.0f) + 10.0f)) & 0xFFFF;
    sp2A = (s32)(this->actor.speedXZ * 100.0f * Math_SinS(sp26) * (Rand_CenteredFloat(2.0f) + 10.0f)) & 0xFFFF;

    D_80858AC8.unk_06 += sp28 >> 2;
    D_80858AC8.unk_08 += sp2A >> 2;

    if (D_80858AC8.unk_06 > 6000) {
        D_80858AC8.unk_06 = 6000;
    } else if (D_80858AC8.unk_06 < -6000) {
        D_80858AC8.unk_06 = -6000;
    }

    if (D_80858AC8.unk_08 > 6000) {
        D_80858AC8.unk_08 = 6000;
    } else if (D_80858AC8.unk_08 < -6000) {
        D_80858AC8.unk_08 = -6000;
    }

    D_80858AC8.unk_00 += D_80858AC8.unk_06;
    D_80858AC8.unk_02 += D_80858AC8.unk_08;

    if (D_80858AC8.unk_00 < 0) {
        D_80858AC8.unk_04 = D_80858AC8.unk_00 >> 1;
    } else {
        D_80858AC8.unk_04 = 0;
    }
}

s32 Player_SetupStartMeleeWeaponAttack(Player* this, PlayState* play) {
    if (Player_CanSwingBottleOrCastFishingRod(play, this) == false) {
        if (Player_CanJumpSlash(this) != false) {
            s32 attackAnim = Player_GetMeleeAttackAnim(this);

            Player_StartMeleeWeaponAttack(play, this, attackAnim);

            if (attackAnim >= PLAYER_MELEEATKTYPE_SPIN_ATTACK_1H) {
                this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
                Player_SetupSpinAttackActor(play, this, 0);
                return 1;
            }
        } else {
            return 0;
        }
    }

    return 1;
}

static Vec3f sShockwaveRaycastPos = { 0.0f, 40.0f, 45.0f };

void Player_MeleeWeaponAttack(Player* this, PlayState* play) {
    MeleeAttackAnimInfo* attackAnim = &sMeleeAttackAnims[this->meleeAttackType];

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (!func_80842DF4(play, this)) {
        Player_SetupMeleeAttack(this, 0.0f, attackAnim->startFrame, attackAnim->endFrame);

        if ((this->stateFlags2 & PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK) &&
            (this->heldItemAction != PLAYER_IA_HAMMER) && LinkAnimation_OnFrame(&this->skelAnime, 0.0f)) {
            this->linearVelocity = 15.0f;
            this->stateFlags2 &= ~PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
        }

        if (this->linearVelocity > 12.0f) {
            Player_SetupSpawnDustAtFeet(play, this);
        }

        Math_StepToF(&this->linearVelocity, 0.0f, 5.0f);
        Player_SetupDeactivateComboTimer(this);

        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (!Player_SetupStartMeleeWeaponAttack(this, play)) {
                u8 savedMoveFlags = this->skelAnime.moveFlags;
                LinkAnimationHeader* anim;

                if (Player_IsUnfriendlyZTargeting(this)) {
                    anim = attackAnim->fightEndAnim;
                } else {
                    anim = attackAnim->endAnim;
                }

                Player_InactivateMeleeWeapon(this);
                this->skelAnime.moveFlags = 0;

                if ((anim == &gPlayerAnim_link_fighter_Lpower_jump_kiru_end) &&
                    (this->modelAnimType != PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON)) {
                    anim = &gPlayerAnim_link_fighter_power_jump_kiru_end;
                }

                Player_SetupReturnToStandStillSetAnim(this, anim, play);

                this->skelAnime.moveFlags = savedMoveFlags;
                this->stateFlags3 |= PLAYER_STATE3_ENDING_MELEE_ATTACK;
            }
        } else if (this->heldItemAction == PLAYER_IA_HAMMER) {
            if ((this->meleeAttackType == PLAYER_MELEEATKTYPE_HAMMER_FORWARD) ||
                (this->meleeAttackType == PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH)) {
                static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
                Vec3f shockwavePos;
                f32 shockwaveDistToPlayerY;

                shockwavePos.y = Player_RaycastFloorWithOffset2(play, this, &sShockwaveRaycastPos, &shockwavePos);
                shockwaveDistToPlayerY = this->actor.world.pos.y - shockwavePos.y;

                Math_ScaledStepToS(&this->actor.focus.rot.x, Math_Atan2S(45.0f, shockwaveDistToPlayerY), 800);
                Player_UpdateLookAngles(this, true);

                if ((((this->meleeAttackType == PLAYER_MELEEATKTYPE_HAMMER_FORWARD) &&
                      LinkAnimation_OnFrame(&this->skelAnime, 7.0f)) ||
                     ((this->meleeAttackType == PLAYER_MELEEATKTYPE_JUMPSLASH_FINISH) &&
                      LinkAnimation_OnFrame(&this->skelAnime, 2.0f))) &&
                    (shockwaveDistToPlayerY > -40.0f) && (shockwaveDistToPlayerY < 40.0f)) {
                    Player_SetupHammerHit(play, this);
                    EffectSsBlast_SpawnWhiteShockwave(play, &shockwavePos, &zeroVec, &zeroVec);
                }
            }
        }
    }
}

void Player_MeleeWeaponRebound(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_StepLinearVelocityToZero(this);

    if (this->skelAnime.curFrame >= 6.0f) {
        Player_ReturnToStandStill(this, play);
    }
}

void Player_ChooseFaroresWindOption(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    LinkAnimation_Update(play, &this->skelAnime);
    Player_SetupCurrentUpperAction(this, play);

    if (this->genericTimer == 0) {
        Message_StartTextbox(play, 0x3B, &this->actor);
        this->genericTimer = 1;
        return;
    }

    if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
        s32 respawnData = gSaveContext.respawn[RESPAWN_MODE_TOP].data;

        if (play->msgCtx.choiceIndex == 0) {
            gSaveContext.respawnFlag = 3;
            play->transitionTrigger = TRANS_TRIGGER_START;
            play->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex;
            play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
            Interface_SetSubTimerToFinalSecond(play);
            return;
        }

        if (play->msgCtx.choiceIndex == 1) {
            gSaveContext.respawn[RESPAWN_MODE_TOP].data = -respawnData;
            gSaveContext.fw.set = 0;
            func_80078914(&gSaveContext.respawn[RESPAWN_MODE_TOP].pos, NA_SE_PL_MAGIC_WIND_VANISH);
        }

        Player_SetupStandingStillMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    }
}

void Player_SpawnFromFaroresWind(Player* this, PlayState* play) {
    s32 respawnData = gSaveContext.respawn[RESPAWN_MODE_TOP].data;

    if (this->genericTimer > 20) {
        this->actor.draw = Player_Draw;
        this->actor.world.pos.y += 60.0f;
        Player_SetupFallFromLedge(this, play);
        return;
    }

    if (this->genericTimer++ == 20) {
        gSaveContext.respawn[RESPAWN_MODE_TOP].data = respawnData + 1;
        func_80078914(&gSaveContext.respawn[RESPAWN_MODE_TOP].pos, NA_SE_PL_MAGIC_WIND_WARP);
    }
}

static LinkAnimationHeader* sStartCastMagicSpellAnims[] = {
    &gPlayerAnim_link_magic_kaze1,
    &gPlayerAnim_link_magic_honoo1,
    &gPlayerAnim_link_magic_tamashii1,
};

static LinkAnimationHeader* sCastMagicSpellAnims[] = {
    &gPlayerAnim_link_magic_kaze2,
    &gPlayerAnim_link_magic_honoo2,
    &gPlayerAnim_link_magic_tamashii2,
};

static LinkAnimationHeader* sEndCastMagicSpellAnims[] = {
    &gPlayerAnim_link_magic_kaze3,
    &gPlayerAnim_link_magic_honoo3,
    &gPlayerAnim_link_magic_tamashii3,
};

static u8 sMagicSpellTimeLimits[] = { 70, 10, 10 };

static PlayerAnimSfxEntry sMagicSpellCastAnimSfx[] = {
    { NA_SE_PL_SKIP, 0x814 },
    { NA_SE_VO_LI_SWORD_N, 0x2014 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x301A },
};

static PlayerAnimSfxEntry sMagicSpellAnimSfx[][2] = {
    {
        { 0, 0x4014 },
        { NA_SE_VO_LI_MAGIC_FROL, -0x201E },
    },
    {
        { 0, 0x4014 },
        { NA_SE_VO_LI_MAGIC_NALE, -0x202C },
    },
    {
        { NA_SE_VO_LI_MAGIC_ATTACK, 0x2014 },
        { NA_SE_IT_SWORD_SWING_HARD, -0x814 },
    },
};

void Player_UpdateMagicSpell(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->magicSpellType < 0) {
            if ((this->itemAction == PLAYER_IA_NAYRUS_LOVE) || (gSaveContext.magicState == MAGIC_STATE_IDLE)) {
                Player_ReturnToStandStill(this, play);
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            if (this->genericTimer == 0) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sStartCastMagicSpellAnims[this->magicSpellType],
                                               0.83f);

                if (Player_SpawnMagicSpellActor(play, this, this->magicSpellType) != NULL) {
                    this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                    if ((this->genericVar != 0) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
                    }
                } else {
                    Magic_Reset(play);
                }
            } else {
                LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, sCastMagicSpellAnims[this->magicSpellType],
                                               0.83f);

                if (this->magicSpellType == 0) {
                    this->genericTimer = -10;
                }
            }

            this->genericTimer++;
        }
    } else {
        if (this->genericTimer < 0) {
            this->genericTimer++;

            if (this->genericTimer == 0) {
                gSaveContext.respawn[RESPAWN_MODE_TOP].data = 1;
                Play_SetupRespawnPoint(play, RESPAWN_MODE_TOP, 0x6FF);
                gSaveContext.fw.set = 1;
                gSaveContext.fw.pos.x = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.x;
                gSaveContext.fw.pos.y = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.y;
                gSaveContext.fw.pos.z = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.z;
                gSaveContext.fw.yaw = gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw;
                gSaveContext.fw.playerParams = 0x6FF;
                gSaveContext.fw.entranceIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex;
                gSaveContext.fw.roomIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex;
                gSaveContext.fw.tempSwchFlags = gSaveContext.respawn[RESPAWN_MODE_DOWN].tempSwchFlags;
                gSaveContext.fw.tempCollectFlags = gSaveContext.respawn[RESPAWN_MODE_DOWN].tempCollectFlags;
                this->genericTimer = 2;
            }
        } else if (this->magicSpellType >= 0) {
            if (this->genericTimer == 0) {
                Player_PlayAnimSfx(this, sMagicSpellCastAnimSfx);
            } else if (this->genericTimer == 1) {
                Player_PlayAnimSfx(this, sMagicSpellAnimSfx[this->magicSpellType]);
                if ((this->magicSpellType == 2) && LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                    this->stateFlags1 &= ~(PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE);
                }
            } else if (sMagicSpellTimeLimits[this->magicSpellType] < this->genericTimer++) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sEndCastMagicSpellAnims[this->magicSpellType],
                                               0.83f);
                this->currentYaw = this->actor.shape.rot.y;
                this->magicSpellType = -1;
            }
        }
    }

    Player_StepLinearVelocityToZero(this);
}

void Player_MoveAlongHookshotPath(Player* this, PlayState* play) {
    f32 distToFloor;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_hook_fly_wait);
    }

    Math_Vec3f_Sum(&this->actor.world.pos, &this->actor.velocity, &this->actor.world.pos);

    if (Player_EndHookshotMove(this)) {
        Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.world.pos);
        Player_UpdateBgcheck(play, this);

        distToFloor = this->actor.world.pos.y - this->actor.floorHeight;
        if (distToFloor > 20.0f) {
            distToFloor = 20.0f;
        }

        this->actor.world.rot.x = this->actor.shape.rot.x = 0;
        this->actor.world.pos.y -= distToFloor;
        this->linearVelocity = 1.0f;
        this->actor.velocity.y = 0.0f;
        Player_SetupFallFromLedge(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
        this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;
        this->stateFlags1 |= PLAYER_STATE1_END_HOOKSHOT_MOVE;
        return;
    }

    if ((this->skelAnime.animation != &gPlayerAnim_link_hook_fly_start) || (4.0f <= this->skelAnime.curFrame)) {
        this->actor.gravity = 0.0f;
        Math_ScaledStepToS(&this->actor.shape.rot.x, this->actor.world.rot.x, 0x800);
        Player_RequestRumble(this, 100, 2, 100, 0);
    }
}

void Player_CastFishingRod(Player* this, PlayState* play) {
    if ((this->genericTimer != 0) && ((this->spinAttackTimer != 0.0f) || (this->unk_85C != 0.0f))) {
        f32 updateScale = R_UPDATE_RATE * 0.5f;

        this->skelAnime.curFrame += this->skelAnime.playSpeed * updateScale;
        if (this->skelAnime.curFrame >= this->skelAnime.animLength) {
            this->skelAnime.curFrame -= this->skelAnime.animLength;
        }

        LinkAnimation_BlendToJoint(play, &this->skelAnime, &gPlayerAnim_link_fishing_wait, this->skelAnime.curFrame,
                                   (this->spinAttackTimer < 0.0f) ? &gPlayerAnim_link_fishing_reel_left
                                                                  : &gPlayerAnim_link_fishing_reel_right,
                                   5.0f, fabsf(this->spinAttackTimer), this->blendTable);
        LinkAnimation_BlendToMorph(play, &this->skelAnime, &gPlayerAnim_link_fishing_wait, this->skelAnime.curFrame,
                                   (this->unk_85C < 0.0f) ? &gPlayerAnim_link_fishing_reel_up
                                                          : &gPlayerAnim_link_fishing_reel_down,
                                   5.0f, fabsf(this->unk_85C), sFishingBlendTable);
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 0.5f);
    } else if (LinkAnimation_Update(play, &this->skelAnime)) {
        this->fishingState = 2;
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_fishing_wait);
        this->genericTimer = 1;
    }

    Player_StepLinearVelocityToZero(this);

    if (this->fishingState == 0) {
        Player_SetupStandingStillMorph(this, play);
    } else if (this->fishingState == 3) {
        Player_SetActionFunc(play, this, Player_ReleaseCaughtFish, 0);
        Player_ChangeAnimMorphToLastFrame(play, this, &gPlayerAnim_link_fishing_fish_catch);
    }
}

void Player_ReleaseCaughtFish(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->fishingState == 0)) {
        Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_fishing_fish_catch_end, play);
    }
}

static void (*csModePlaybackFuncs[])(PlayState*, Player*, void*) = {
    NULL,
    Player_AnimPlaybackType0,
    Player_AnimPlaybackType1,
    Player_AnimPlaybackType2,
    Player_AnimPlaybackType3,
    Player_AnimPlaybackType4,
    Player_AnimPlaybackType5,
    Player_AnimPlaybackType6,
    Player_AnimPlaybackType7,
    Player_AnimPlaybackType8,
    Player_AnimPlaybackType9,
    Player_AnimPlaybackType10,
    Player_AnimPlaybackType11,
    Player_AnimPlaybackType12,
    Player_AnimPlaybackType13,
    Player_AnimPlaybackType14,
    Player_AnimPlaybackType15,
    Player_AnimPlaybackType16,
    Player_AnimPlaybackType17,
};

static PlayerAnimSfxEntry sGetUpFromDekuTreeStoryAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x2822 },
    { NA_SE_PL_CALM_HIT, 0x82D },
    { NA_SE_PL_CALM_HIT, 0x833 },
    { NA_SE_PL_CALM_HIT, -0x840 },
};

static PlayerAnimSfxEntry sSurprisedStumbleBackFallAnimSfx[] = {
    { NA_SE_VO_LI_SURPRISE, 0x2003 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x300F },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3018 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x301E },
    { NA_SE_VO_LI_FALL_L, -0x201F },
};

static PlayerAnimSfxEntry sFallToKneeAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x300A },
};

static CutsceneModeEntry sCutsceneModeInitFuncs[] = {
    { 0, NULL },                                         // PLAYER_CSMODE_NONE
    { -1, Player_CutsceneSetupIdle },                    // PLAYER_CSMODE_IDLE
    { 2, &gPlayerAnim_link_demo_goma_furimuki },         // PLAYER_CSMODE_TURN_AROUND_SURPRISED_SHORT
    { 0, NULL },                                         // PLAYER_CSMODE_UNK_3
    { 0, NULL },                                         // PLAYER_CSMODE_UNK_4
    { 3, &gPlayerAnim_link_demo_bikkuri },               // PLAYER_CSMODE_SURPRISED
    { 0, NULL },                                         // PLAYER_CSMODE_UNK_6
    { 0, NULL },                                         // PLAYER_CSMODE_END
    { -1, Player_CutsceneSetupIdle },                    // PLAYER_CSMODE_WAIT
    { 2, &gPlayerAnim_link_demo_furimuki },              // PLAYER_CSMODE_TURN_AROUND_SURPRISED_LONG
    { -1, Player_CutsceneSetupEnterWarp },               // PLAYER_CSMODE_ENTER_WARP
    { 3, &gPlayerAnim_link_demo_warp },                  // PLAYER_CSMODE_RAISED_BY_WARP
    { -1, Player_CutsceneSetupFightStance },             // PLAYER_CSMODE_FIGHT_STANCE
    { 7, &gPlayerAnim_clink_demo_get1 },                 // PLAYER_CSMODE_START_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_clink_demo_get2 },                 // PLAYER_CSMODE_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_clink_demo_get3 },                 // PLAYER_CSMODE_END_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_clink_demo_standup },              // PLAYER_CSMODE_GET_UP_FROM_DEKU_TREE_STORY
    { 7, &gPlayerAnim_clink_demo_standup_wait },         // PLAYER_CSMODE_SIT_LISTENING_TO_DEKU_TREE_STORY
    { -1, Player_CutsceneSetupSwordPedestal },           // PLAYER_CSMODE_SWORD_INTO_PEDESTAL
    { 2, &gPlayerAnim_link_demo_baru_op1 },              // PLAYER_CSMODE_REACT_TO_QUAKE
    { 2, &gPlayerAnim_link_demo_baru_op3 },              // PLAYER_CSMODE_END_REACT_TO_QUAKE
    { 0, NULL },                                         // PLAYER_CSMODE_UNK_21
    { -1, Player_CutsceneSetupWarpToSages },             // PLAYER_CSMODE_WARP_TO_SAGES
    { 3, &gPlayerAnim_link_demo_jibunmiru },             // PLAYER_CSMODE_LOOK_AT_SELF
    { 9, &gPlayerAnim_link_normal_back_downA },          // PLAYER_CSMODE_KNOCKED_TO_GROUND
    { 2, &gPlayerAnim_link_normal_back_down_wake },      // PLAYER_CSMODE_GET_UP_FROM_GROUND
    { -1, Player_CutsceneSetupStartPlayOcarina },        // PLAYER_CSMODE_START_PLAY_OCARINA
    { 2, &gPlayerAnim_link_normal_okarina_end },         // PLAYER_CSMODE_END_PLAY_OCARINA
    { 3, &gPlayerAnim_link_demo_get_itemA },             // PLAYER_CSMODE_GET_ITEM
    { -1, Player_CutsceneSetupIdle },                    // PLAYER_CSMODE_IDLE_2
    { 2, &gPlayerAnim_link_normal_normal2fighter_free }, // PLAYER_CSMODE_DRAW_AND_BRANDISH_SWORD
    { 0, NULL },                                         // PLAYER_CSMODE_CLOSE_EYES
    { 0, NULL },                                         // PLAYER_CSMODE_OPEN_EYES
    { 5, &gPlayerAnim_clink_demo_atozusari },            // PLAYER_CSMODE_SURPRIED_STUMBLE_BACK_AND_FALL
    { -1, Player_CutsceneSetupSwimIdle },                // PLAYER_CSMODE_SURFACE_FROM_DIVE
    { -1, Player_CutsceneSetupGetItemInWater },          // PLAYER_CSMODE_GET_ITEM_IN_WATER
    { 5, &gPlayerAnim_clink_demo_bashi },                // PLAYER_CSMODE_GENTLE_KNOCKBACK_INTO_SIT
    { 16, &gPlayerAnim_link_normal_hang_up_down },       // PLAYER_CSMODE_GRABBED_AND_CARRIED_BY_NECK
    { -1, Player_CutsceneSetupSleepingRestless },        // PLAYER_CSMODE_SLEEPING_RESTLESS
    { -1, Player_CutsceneSetupSleeping },                // PLAYER_CSMODE_SLEEPING
    { 6, &gPlayerAnim_clink_op3_okiagari },              // PLAYER_CSMODE_AWAKEN
    { 6, &gPlayerAnim_clink_op3_tatiagari },             // PLAYER_CSMODE_GET_OFF_BED
    { -1, Player_CutsceneSetupBlownBackward },           // PLAYER_CSMODE_BLOWN_BACKWARD
    { 5, &gPlayerAnim_clink_demo_miokuri },              // PLAYER_CSMODE_STAND_UP_AND_WATCH
    { -1, Player_CutsceneSetupIdle3 },                   // PLAYER_CSMODE_IDLE_3
    { -1, Player_CutsceneSetupStop },                    // PLAYER_CSMODE_STOP
    { -1, Player_CutsceneSetDraw },                      // PLAYER_CSMODE_STOP_2
    { 5, &gPlayerAnim_clink_demo_nozoki },               // PLAYER_CSMODE_LOOK_THROUGH_PEEPHOLE
    { 5, &gPlayerAnim_clink_demo_koutai },               // PLAYER_CSMODE_STEP_BACK_CAUTIOUSLY
    { -1, Player_CutsceneSetupIdle },                    // PLAYER_CSMODE_IDLE_4
    { 5, &gPlayerAnim_clink_demo_koutai_kennuki },       // PLAYER_CSMODE_DRAW_SWORD_CHILD
    { 5, &gPlayerAnim_link_demo_kakeyori },              // PLAYER_CSMODE_JUMP_TO_ZELDAS_CRYSTAL
    { 5, &gPlayerAnim_link_demo_kakeyori_mimawasi },     // PLAYER_CSMODE_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL
    { 5, &gPlayerAnim_link_demo_kakeyori_miokuri },      // PLAYER_CSMODE_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING
    { 3, &gPlayerAnim_link_demo_furimuki2 },             // PLAYER_CSMODE_TURN_AROUND_SLOWLY
    { 3, &gPlayerAnim_link_demo_kaoage },                // PLAYER_CSMODE_END_SHIELD_EYES_WITH_HAND
    { 4, &gPlayerAnim_link_demo_kaoage_wait },           // PLAYER_CSMODE_SHIELD_EYES_WITH_HAND
    { 3, &gPlayerAnim_clink_demo_mimawasi },             // PLAYER_CSMODE_LOOK_AROUND_SURPRISED
    { 3, &gPlayerAnim_link_demo_nozokikomi },            // PLAYER_CSMODE_INSPECT_GROUND_CAREFULLY
    { 6, &gPlayerAnim_kolink_odoroki_demo },             // PLAYER_CSMODE_STARTLED_BY_GORONS_FALLING
    { 6, &gPlayerAnim_link_shagamu_demo },               // PLAYER_CSMODE_FALL_TO_KNEE
    { 14, &gPlayerAnim_link_okiru_demo },                // PLAYER_CSMODE_FLAT_ON_BACK
    { 3, &gPlayerAnim_link_okiru_demo },                 // PLAYER_CSMODE_RAISE_FROM_FLAT_ON_BACK
    { 5, &gPlayerAnim_link_fighter_power_kiru_start },   // PLAYER_CSMODE_START_SPIN_ATTACK
    { 16, &gPlayerAnim_demo_link_nwait },                // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_IDLE
    { 15, &gPlayerAnim_demo_link_tewatashi },            // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA
    { 15, &gPlayerAnim_demo_link_orosuu },               // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA
    { 3, &gPlayerAnim_d_link_orooro },                   // PLAYER_CSMODE_START_LOOK_AROUND_AFTER_SWORD_WARP
    { 3, &gPlayerAnim_d_link_imanodare },                // PLAYER_CSMODE_END_LOOK_AROUND_AFTER_SWORD_WARP
    { 3, &gPlayerAnim_link_hatto_demo },                 // PLAYER_CSMODE_LOOK_AROUND_AND_AT_SELF_QUICKLY
    { 6, &gPlayerAnim_o_get_mae },                       // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_ADULT
    { 6, &gPlayerAnim_o_get_ato },                       // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_ADULT
    { 6, &gPlayerAnim_om_get_mae },                      // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_CHILD
    { 6, &gPlayerAnim_nw_modoru },                       // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_CHILD
    { 3, &gPlayerAnim_link_demo_gurad },                 // PLAYER_CSMODE_RESIST_DARK_MAGIC
    { 3, &gPlayerAnim_link_demo_look_hand },             // PLAYER_CSMODE_TRIFORCE_HAND_RESONATES
    { 4, &gPlayerAnim_link_demo_sita_wait },             // PLAYER_CSMODE_STARE_DOWN_STARTLED
    { 3, &gPlayerAnim_link_demo_ue },                    // PLAYER_CSMODE_LOOK_UP_STARTLED
    { 3, &gPlayerAnim_Link_muku },                       // PLAYER_CSMODE_LOOK_TO_CHARACTER_AT_SIDE_SMILING
    { 3, &gPlayerAnim_Link_miageru },                    // PLAYER_CSMODE_LOOK_TO_CHARACTER_ABOVE_SMILING
    { 6, &gPlayerAnim_Link_ha },                         // PLAYER_CSMODE_SURPRISED_DEFENSE
    { 3, &gPlayerAnim_L_1kyoro },                        // PLAYER_CSMODE_START_HALF_TURN_SURPRISED
    { 3, &gPlayerAnim_L_2kyoro },                        // PLAYER_CSMODE_END_HALF_TURN_SURPRISED
    { 3, &gPlayerAnim_L_sagaru },                        // PLAYER_CSMODE_START_LOOK_UP_DEFENSE
    { 3, &gPlayerAnim_L_bouzen },                        // PLAYER_CSMODE_LOOK_UP_DEFENSE_IDLE
    { 3, &gPlayerAnim_L_kamaeru },                       // PLAYER_CSMODE_END_LOOK_UP_DEFENSE
    { 3, &gPlayerAnim_L_hajikareru },                    // PLAYER_CSMODE_START_SWORD_KNOCKED_FROM_HAND
    { 3, &gPlayerAnim_L_ken_miru },                      // PLAYER_CSMODE_SWORD_KNOCKED_FROM_HAND_IDLE
    { 3, &gPlayerAnim_L_mukinaoru },                     // PLAYER_CSMODE_END_SWORD_KNOCKED_FROM_HAND
    { -1, Player_CutsceneSetupSpinAttackIdle },          // PLAYER_CSMODE_SPIN_ATTACK_IDLE
    { 3, &gPlayerAnim_link_wait_itemD1_20f },            // PLAYER_CSMODE_INSPECT_WEAPON
    { -1, Player_SetupDoNothing4 },                      // PLAYER_CSMODE_UNK_91
    { -1, Player_CutsceneSetupKnockedToGroundDamaged },  // PLAYER_CSMODE_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT
    { 3, &gPlayerAnim_link_normal_wait_typeB_20f },      // PLAYER_CSMODE_REACT_TO_HEAT
    { -1, Player_CutsceneSetupGetSwordBack },            // PLAYER_CSMODE_GET_SWORD_BACK
    { 3, &gPlayerAnim_link_demo_kousan },                // PLAYER_CSMODE_CAUGHT_BY_GUARD
    { 3, &gPlayerAnim_link_demo_return_to_past },        // PLAYER_CSMODE_GET_SWORD_BACK_2
    { 3, &gPlayerAnim_link_last_hit_motion1 },           // PLAYER_CSMODE_START_GANON_KILL_COMBO
    { 3, &gPlayerAnim_link_last_hit_motion2 },           // PLAYER_CSMODE_END_GANON_KILL_COMBO
    { 3, &gPlayerAnim_link_demo_zeldamiru },             // PLAYER_CSMODE_WATCH_ZELDA_STUN_GANON
    { 3, &gPlayerAnim_link_demo_kenmiru1 },              // PLAYER_CSMODE_START_LOOK_AT_SWORD_GLOW
    { 3, &gPlayerAnim_link_demo_kenmiru2 },              // PLAYER_CSMODE_LOOK_AT_SWORD_GLOW_IDLE
    { 3, &gPlayerAnim_link_demo_kenmiru2_modori },       // PLAYER_CSMODE_END_LOOK_AT_SWORD_GLOW
};

static CutsceneModeEntry sCutsceneModeUpdateFuncs[] = {
    { 0, NULL },                                          // PLAYER_CSMODE_NONE
    { -1, Player_CutsceneIdle },                          // PLAYER_CSMODE_IDLE
    { -1, Player_CutsceneTurnAroundSurprisedShort },      // PLAYER_CSMODE_TURN_AROUND_SURPRISED_SHORT
    { -1, Player_CutsceneUnk3Update },                    // PLAYER_CSMODE_UNK_3
    { -1, Player_CutsceneUnk4Update },                    // PLAYER_CSMODE_UNK_4
    { 11, NULL },                                         // PLAYER_CSMODE_SURPRISED
    { -1, Player_CutsceneUnk6Update },                    // PLAYER_CSMODE_UNK_6
    { -1, Player_CutsceneEnd },                           // PLAYER_CSMODE_END
    { -1, Player_CutsceneWait },                          // PLAYER_CSMODE_WAIT
    { -1, Player_CutsceneTurnAroundSurprisedLong },       // PLAYER_CSMODE_TURN_AROUND_SURPRISED_LONG
    { -1, Player_CutsceneEnterWarp },                     // PLAYER_CSMODE_ENTER_WARP
    { -1, Player_CutsceneRaisedByWarp },                  // PLAYER_CSMODE_RAISED_BY_WARP
    { -1, Player_CutsceneFightStance },                   // PLAYER_CSMODE_FIGHT_STANCE
    { 11, NULL },                                         // PLAYER_CSMODE_START_GET_SPIRITUAL_STONE
    { 11, NULL },                                         // PLAYER_CSMODE_GET_SPIRITUAL_STONE
    { 11, NULL },                                         // PLAYER_CSMODE_END_GET_SPIRITUAL_STONE
    { 18, sGetUpFromDekuTreeStoryAnimSfx },               // PLAYER_CSMODE_GET_UP_FROM_DEKU_TREE_STORY
    { 11, NULL },                                         // PLAYER_CSMODE_SIT_LISTENING_TO_DEKU_TREE_STORY
    { -1, Player_CutsceneSwordPedestal },                 // PLAYER_CSMODE_SWORD_INTO_PEDESTAL
    { 12, &gPlayerAnim_link_demo_baru_op2 },              // PLAYER_CSMODE_REACT_TO_QUAKE
    { 11, NULL },                                         // PLAYER_CSMODE_END_REACT_TO_QUAKE
    { 0, NULL },                                          // PLAYER_CSMODE_UNK_21
    { -1, Player_CutsceneWarpToSages },                   // PLAYER_CSMODE_WARP_TO_SAGES
    { 11, NULL },                                         // PLAYER_CSMODE_LOOK_AT_SELF
    { -1, Player_CutsceneKnockedToGround },               // PLAYER_CSMODE_KNOCKED_TO_GROUND
    { 11, NULL },                                         // PLAYER_CSMODE_GET_UP_FROM_GROUND
    { 17, &gPlayerAnim_link_normal_okarina_swing },       // PLAYER_CSMODE_START_PLAY_OCARINA
    { 11, NULL },                                         // PLAYER_CSMODE_END_PLAY_OCARINA
    { 11, NULL },                                         // PLAYER_CSMODE_GET_ITEM
    { 11, NULL },                                         // PLAYER_CSMODE_IDLE_2
    { -1, Player_CutsceneDrawAndBrandishSword },          // PLAYER_CSMODE_DRAW_AND_BRANDISH_SWORD
    { -1, Player_CutsceneCloseEyes },                     // PLAYER_CSMODE_CLOSE_EYES
    { -1, Player_CutsceneOpenEyes },                      // PLAYER_CSMODE_OPEN_EYES
    { 18, sSurprisedStumbleBackFallAnimSfx },             // PLAYER_CSMODE_SURPRIED_STUMBLE_BACK_AND_FALL
    { -1, Player_CutsceneSurfaceFromDive },               // PLAYER_CSMODE_SURFACE_FROM_DIVE
    { 11, NULL },                                         // PLAYER_CSMODE_GET_ITEM_IN_WATER
    { 11, NULL },                                         // PLAYER_CSMODE_GENTLE_KNOCKBACK_INTO_SIT
    { 11, NULL },                                         // PLAYER_CSMODE_GRABBED_AND_CARRIED_BY_NECK
    { 11, NULL },                                         // PLAYER_CSMODE_SLEEPING_RESTLESS
    { -1, Player_CutsceneSleeping },                      // PLAYER_CSMODE_SLEEPING
    { -1, Player_CutsceneAwaken },                        // PLAYER_CSMODE_AWAKEN
    { -1, Player_CutsceneGetOffBed },                     // PLAYER_CSMODE_GET_OFF_BED
    { -1, Player_CutsceneBlownBackward },                 // PLAYER_CSMODE_BLOWN_BACKWARD
    { 13, &gPlayerAnim_clink_demo_miokuri_wait },         // PLAYER_CSMODE_STAND_UP_AND_WATCH
    { -1, Player_CutsceneIdle3 },                         // PLAYER_CSMODE_IDLE_3
    { 0, NULL },                                          // PLAYER_CSMODE_STOP
    { 0, NULL },                                          // PLAYER_CSMODE_STOP_2
    { 11, NULL },                                         // PLAYER_CSMODE_LOOK_THROUGH_PEEPHOLE
    { -1, Player_CutsceneStepBackCautiously },            // PLAYER_CSMODE_STEP_BACK_CAUTIOUSLY
    { -1, Player_CutsceneWait },                          // PLAYER_CSMODE_IDLE_4
    { -1, Player_CutsceneDrawSwordChild },                // PLAYER_CSMODE_DRAW_SWORD_CHILD
    { 13, &gPlayerAnim_link_demo_kakeyori_wait },         // PLAYER_CSMODE_JUMP_TO_ZELDAS_CRYSTAL
    { -1, Player_CutsceneDesperateLookAtZeldasCrystal },  // PLAYER_CSMODE_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL
    { 13, &gPlayerAnim_link_demo_kakeyori_miokuri_wait }, // PLAYER_CSMODE_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING
    { -1, Player_CutsceneTurnAroundSlowly },              // PLAYER_CSMODE_TURN_AROUND_SLOWLY
    { 11, NULL },                                         // PLAYER_CSMODE_END_SHIELD_EYES_WITH_HAND
    { 11, NULL },                                         // PLAYER_CSMODE_SHIELD_EYES_WITH_HAND
    { 12, &gPlayerAnim_clink_demo_mimawasi_wait },        // PLAYER_CSMODE_LOOK_AROUND_SURPRISED
    { -1, Player_CutsceneInspectGroundCarefully },        // PLAYER_CSMODE_INSPECT_GROUND_CAREFULLY
    { 11, NULL },                                         // PLAYER_CSMODE_STARTLED_BY_GORONS_FALLING
    { 18, sFallToKneeAnimSfx },                           // PLAYER_CSMODE_FALL_TO_KNEE
    { 11, NULL },                                         // PLAYER_CSMODE_FLAT_ON_BACK
    { 11, NULL },                                         // PLAYER_CSMODE_RAISE_FROM_FLAT_ON_BACK
    { 11, NULL },                                         // PLAYER_CSMODE_START_SPIN_ATTACK
    { 11, NULL },                                         // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_IDLE
    { -1, Player_CutsceneStartPassOcarina },              // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA
    { 17, &gPlayerAnim_demo_link_nwait },                 // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA
    { 12, &gPlayerAnim_d_link_orowait },                  // PLAYER_CSMODE_START_LOOK_AROUND_AFTER_SWORD_WARP
    { 12, &gPlayerAnim_demo_link_nwait },                 // PLAYER_CSMODE_END_LOOK_AROUND_AFTER_SWORD_WARP
    { 11, NULL },                                         // PLAYER_CSMODE_LOOK_AROUND_AND_AT_SELF_QUICKLY
    { -1, Player_LearnOcarinaSong },                      // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_ADULT
    { 17, &gPlayerAnim_sude_nwait },                      // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_ADULT
    { -1, Player_LearnOcarinaSong },                      // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_CHILD
    { 17, &gPlayerAnim_sude_nwait },                      // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_CHILD
    { 12, &gPlayerAnim_link_demo_gurad_wait },            // PLAYER_CSMODE_RESIST_DARK_MAGIC
    { 12, &gPlayerAnim_link_demo_look_hand_wait },        // PLAYER_CSMODE_TRIFORCE_HAND_RESONATES
    { 11, NULL },                                         // PLAYER_CSMODE_STARE_DOWN_STARTLED
    { 12, &gPlayerAnim_link_demo_ue_wait },               // PLAYER_CSMODE_LOOK_UP_STARTLED
    { 12, &gPlayerAnim_Link_m_wait },                     // PLAYER_CSMODE_LOOK_TO_CHARACTER_AT_SIDE_SMILING
    { 13, &gPlayerAnim_Link_ue_wait },                    // PLAYER_CSMODE_LOOK_TO_CHARACTER_ABOVE_SMILING
    { 12, &gPlayerAnim_Link_otituku_w },                  // PLAYER_CSMODE_SURPRISED_DEFENSE
    { 12, &gPlayerAnim_L_kw },                            // PLAYER_CSMODE_START_HALF_TURN_SURPRISED
    { 11, NULL },                                         // PLAYER_CSMODE_END_HALF_TURN_SURPRISED
    { 11, NULL },                                         // PLAYER_CSMODE_START_LOOK_UP_DEFENSE
    { 11, NULL },                                         // PLAYER_CSMODE_LOOK_UP_DEFENSE_IDLE
    { 11, NULL },                                         // PLAYER_CSMODE_END_LOOK_UP_DEFENSE
    { -1, Player_CutsceneSwordKnockedFromHand },          // PLAYER_CSMODE_START_SWORD_KNOCKED_FROM_HAND
    { 11, NULL },                                         // PLAYER_CSMODE_SWORD_KNOCKED_FROM_HAND_IDLE
    { 12, &gPlayerAnim_L_kennasi_w },                     // PLAYER_CSMODE_END_SWORD_KNOCKED_FROM_HAND
    { -1, Player_CutsceneSpinAttackIdle },                // PLAYER_CSMODE_SPIN_ATTACK_IDLE
    { -1, Player_CutsceneInspectWeapon },                 // PLAYER_CSMODE_INSPECT_WEAPON
    { -1, Player_DoNothing5 },                            // PLAYER_CSMODE_UNK_91
    { -1, Player_CutsceneKnockedToGroundDamaged },        // PLAYER_CSMODE_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT
    { 11, NULL },                                         // PLAYER_CSMODE_REACT_TO_HEAT
    { 11, NULL },                                         // PLAYER_CSMODE_GET_SWORD_BACK
    { 11, NULL },                                         // PLAYER_CSMODE_CAUGHT_BY_GUARD
    { -1, Player_CutsceneGetSwordBack },                  // PLAYER_CSMODE_GET_SWORD_BACK_2
    { -1, Player_CutsceneGanonKillCombo },                // PLAYER_CSMODE_START_GANON_KILL_COMBO
    { -1, Player_CutsceneGanonKillCombo },                // PLAYER_CSMODE_END_GANON_KILL_COMBO
    { 12, &gPlayerAnim_link_demo_zeldamiru_wait },        // PLAYER_CSMODE_WATCH_ZELDA_STUN_GANON
    { 12, &gPlayerAnim_link_demo_kenmiru1_wait },         // PLAYER_CSMODE_START_LOOK_AT_SWORD_GLOW
    { 12, &gPlayerAnim_link_demo_kenmiru2_wait },         // PLAYER_CSMODE_LOOK_AT_SWORD_GLOW_IDLE
    { 12, &gPlayerAnim_demo_link_nwait },                 // PLAYER_CSMODE_END_LOOK_AT_SWORD_GLOW
};

void Player_StopAnimAndMovement(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ClearRootLimbPosY(this);
    Player_ChangeAnimMorphToLastFrame(play, this, anim);
    Player_StopMovement(this);
}

void Player_StopAnimAndMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ClearRootLimbPosY(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f), 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE,
                         -8.0f);
    Player_StopMovement(this);
}

void Player_LoopAnimAndStopMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ClearRootLimbPosY(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f), 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f);
    Player_StopMovement(this);
}

void Player_AnimPlaybackType0(PlayState* play, Player* this, void* anim) {
    Player_StopMovement(this);
}

void Player_AnimPlaybackType1(PlayState* play, Player* this, void* anim) {
    Player_StopAnimAndMovement(play, this, anim);
}

void Player_AnimPlaybackType13(PlayState* play, Player* this, void* anim) {
    Player_ClearRootLimbPosY(this);
    Player_ChangeAnimOnce(play, this, anim);
    Player_StopMovement(this);
}

void Player_AnimPlaybackType2(PlayState* play, Player* this, void* anim) {
    Player_StopAnimAndMovementSlowed(play, this, anim);
}

void Player_AnimPlaybackType3(PlayState* play, Player* this, void* anim) {
    Player_LoopAnimAndStopMovementSlowed(play, this, anim);
}

void Player_AnimPlaybackType4(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnceWithMovementPresetFlagsSlowed(play, this, anim);
}

void Player_AnimPlaybackType5(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnceWithMovement(play, this, anim,
                                    PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                        PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                        PLAYER_ANIMMOVEFLAGS_7);
}

void Player_AnimPlaybackType6(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, anim);
}

void Player_AnimPlaybackType7(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoopWithMovement(play, this, anim,
                                    PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                        PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                        PLAYER_ANIMMOVEFLAGS_7);
}

void Player_AnimPlaybackType8(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnce(play, this, anim);
}

void Player_AnimPlaybackType9(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoop(play, this, anim);
}

void Player_AnimPlaybackType14(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnceSlowed(play, this, anim);
}

void Player_AnimPlaybackType15(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoopSlowed(play, this, anim);
}

void Player_AnimPlaybackType10(PlayState* play, Player* this, void* anim) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_AnimPlaybackType11(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_LoopAnimAndStopMovementSlowed(play, this, anim);
        this->genericTimer = 1;
    }
}

void Player_AnimPlaybackType16(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        Player_PlayAnimLoopSlowed(play, this, anim);
    }
}

void Player_AnimPlaybackType12(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, anim);
        this->genericTimer = 1;
    }
}

void Player_AnimPlaybackType17(PlayState* play, Player* this, void* animSfxEntry) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, animSfxEntry);
}

void Player_LookAtCutsceneTargetActor(Player* this) {
    if ((this->csTargetActor == NULL) || (this->csTargetActor->update == NULL)) {
        this->csTargetActor = NULL;
    }

    this->targetActor = this->csTargetActor;

    if (this->targetActor != NULL) {
        this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_CutsceneSetupSwimIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->stateFlags1 |= PLAYER_STATE1_SWIMMING;
    this->stateFlags2 |= PLAYER_STATE2_DIVING;
    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);

    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_CutsceneSurfaceFromDive(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->actor.gravity = 0.0f;

    if (this->genericVar == 0) {
        if (Player_SetupDive(play, this, NULL)) {
            this->genericVar = 1;
        } else {
            Player_PlaySwimAnim(play, this, NULL, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->shapePitchOffset, -DEG_TO_BINANG(54.932f), DEG_TO_BINANG(4.395f));
            Player_UpdateSwimMovement(this, &this->actor.velocity.y, 4.0f, this->currentYaw);
        }
        return;
    }

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericVar == 1) {
            Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
        } else {
            Player_PlayAnimLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
        }
    }

    Player_SetVerticalWaterVelocity(this);
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_CutsceneIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_LookAtCutsceneTargetActor(this);

    if (Player_IsSwimming(this)) {
        Player_CutsceneSurfaceFromDive(play, this, 0);
        return;
    }

    LinkAnimation_Update(play, &this->skelAnime);

    if (Player_IsShootingHookshot(this) || (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
        Player_SetupCurrentUpperAction(this, play);
        return;
    }

    if ((this->interactRangeActor != NULL) && (this->interactRangeActor->textId == 0xFFFF)) {
        Player_SetupGetItemOrHoldBehavior(this, play);
    }
}

void Player_CutsceneTurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_CutsceneSetupIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimationHeader* anim;

    if (Player_IsSwimming(this)) {
        Player_CutsceneSetupSwimIdle(play, this, 0);
        return;
    }

    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_nwait, this->modelAnimType);

    if ((this->csAction == 6) || (this->csAction == 0x2E)) {
        Player_PlayAnimOnce(play, this, anim);
    } else {
        Player_ClearRootLimbPosY(this);
        LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f), 0.0f, Animation_GetLastFrame(anim),
                             ANIMMODE_LOOP, -4.0f);
    }

    Player_StopMovement(this);
}

void Player_CutsceneWait(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (Player_SetupShootingGalleryPlay(play, this) == 0) {
        if ((this->csMode == PLAYER_CSMODE_IDLE_4) && (play->csCtx.state == CS_STATE_IDLE)) {
            Actor_SetPlayerCutscene(play, NULL, PLAYER_CSMODE_END);
            return;
        }

        if (Player_IsSwimming(this) != 0) {
            Player_CutsceneSurfaceFromDive(play, this, 0);
            return;
        }

        LinkAnimation_Update(play, &this->skelAnime);

        if (Player_IsShootingHookshot(this) || (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            Player_SetupCurrentUpperAction(this, play);
        }
    }
}

static PlayerAnimSfxEntry sTurnAroundSurprisedAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x302A },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3030 },
};

void Player_CutsceneTurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sTurnAroundSurprisedAnimSfx);
}

void Player_CutsceneSetupEnterWarp(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->stateFlags1 &= ~PLAYER_STATE1_AWAITING_THROWN_BOOMERANG;

    this->currentYaw = this->actor.shape.rot.y = this->actor.world.rot.y =
        Math_Vec3f_Yaw(&this->actor.world.pos, &this->csStartPos);

    if (this->linearVelocity <= 0.0f) {
        this->linearVelocity = 0.1f;
    } else if (this->linearVelocity > 2.5f) {
        this->linearVelocity = 2.5f;
    }
}

void Player_CutsceneEnterWarp(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    f32 targetVelocity = 2.5f;

    Player_CutsceneMoveUsingActionPosIntoRange(play, this, &targetVelocity, 10);

    if (play->sceneId == SCENE_JABU_JABU_BOSS) {
        if (this->genericTimer == 0) {
            if (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) {
                return;
            }
        } else {
            if (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) {
                return;
            }
        }
    }

    this->genericTimer++;
    if (this->genericTimer > 20) {
        this->csMode = PLAYER_CSMODE_RAISED_BY_WARP;
    }
}

void Player_CutsceneSetupFightStance(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_SetupUnfriendlyZTarget(this, play);
}

void Player_CutsceneFightStance(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_LookAtCutsceneTargetActor(this);

    if (this->genericTimer != 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_PlayAnimLoop(play, this, Player_GetFightingRightAnim(this));
            this->genericTimer = 0;
        }

        Player_ResetLeftRightBlendWeight(this);
    } else {
        func_808401B0(play, this);
    }
}

void Player_CutsceneUnk3Update(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_CutsceneMoveUsingActionPos(play, this, linkCsAction, 0.0f, 0, 0);
}

void Player_CutsceneUnk4Update(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_CutsceneMoveUsingActionPos(play, this, linkCsAction, 0.0f, 0, 1);
}

// unused
static LinkAnimationHeader* sTimeTravelStartAnims[] = {
    &gPlayerAnim_link_demo_back_to_past,
    &gPlayerAnim_clink_demo_goto_future,
};

static Vec3f sStartTimeTravelPos = { -1.0f, 70.0f, 20.0f };

void Player_CutsceneSetupSwordPedestal(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Math_Vec3f_Copy(&this->actor.world.pos, &sStartTimeTravelPos);
    this->actor.shape.rot.y = -0x8000;
    Player_PlayAnimOnceSlowed(play, this, this->ageProperties->timeTravelStartAnim);
    Player_SetupAnimMovement(play, this,
                             PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y |
                                 PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                 PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_7 |
                                 PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
}

static SwordPedestalSfx sSwordPedestalSfx[] = {
    { NA_SE_IT_SWORD_PUTAWAY_STN, 0 },
    { NA_SE_IT_SWORD_STICK_STN, NA_SE_VO_LI_SWORD_N },
};

static PlayerAnimSfxEntry sStepOntoPedestalAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x401D },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4027 },
};

void Player_CutsceneSwordPedestal(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    SwordPedestalSfx* swordPedestalSfx;
    Gfx** dLists;

    LinkAnimation_Update(play, &this->skelAnime);

    if ((LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 70.0f)) ||
        (!LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 87.0f))) {
        swordPedestalSfx = &sSwordPedestalSfx[gSaveContext.linkAge];
        this->interactRangeActor->parent = &this->actor;

        if (!LINK_IS_ADULT) {
            dLists = gPlayerLeftHandBgsDLs;
        } else {
            dLists = gPlayerLeftHandClosedDLs;
        }
        this->leftHandDLists = dLists + gSaveContext.linkAge;

        func_8002F7DC(&this->actor, swordPedestalSfx->swordSfx);
        if (!LINK_IS_ADULT) {
            Player_PlayVoiceSfxForAge(this, swordPedestalSfx->voiceSfx);
        }
    } else if (LINK_IS_ADULT) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 66.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
        }
    } else {
        Player_PlayAnimSfx(this, sStepOntoPedestalAnimSfx);
    }
}

void Player_CutsceneSetupWarpToSages(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_demo_warp, -(2.0f / 3.0f), 12.0f, 12.0f,
                         ANIMMODE_ONCE, 0.0f);
}

static PlayerAnimSfxEntry sWarpToSagesAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x281E },
};

void Player_CutsceneWarpToSages(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);

    this->genericTimer++;

    if (this->genericTimer >= 180) {
        if (this->genericTimer == 180) {
            LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_okarina_warp_goal, (2.0f / 3.0f), 10.0f,
                                 Animation_GetLastFrame(&gPlayerAnim_link_okarina_warp_goal), ANIMMODE_ONCE, -8.0f);
        }
        Player_PlayAnimSfx(this, sWarpToSagesAnimSfx);
    }
}

void Player_CutsceneKnockedToGround(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->genericTimer == 0) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_back_downB);
        this->genericTimer = 1;
    }

    if (this->genericTimer != 0) {
        Player_StepLinearVelocityToZero(this);
    }
}

void Player_CutsceneSetupStartPlayOcarina(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_StopAnimAndMovementSlowed(play, this, &gPlayerAnim_link_normal_okarina_start);
    Player_SetOcarinaItemAction(this);
    Player_SetModels(this, Player_ActionToModelGroup(this, this->itemAction));
}

static PlayerAnimSfxEntry sDrawAndBrandishSwordAnimSfx[] = {
    { NA_SE_IT_SWORD_PICKOUT, -0x80C },
};

void Player_CutsceneDrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Player_CutsceneDrawSword(play, this, 0);
    } else {
        Player_PlayAnimSfx(this, sDrawAndBrandishSwordAnimSfx);
    }
}

void Player_CutsceneCloseEyes(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 0, 1);
}

void Player_CutsceneOpenEyes(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 2, 1);
}

void Player_CutsceneSetupGetItemInWater(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, &gPlayerAnim_link_swimer_swim_get,
                                          PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                              PLAYER_ANIMMOVEFLAGS_7);
}

void Player_CutsceneSetupSleeping(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_PlayAnimOnceWithMovement(play, this, &gPlayerAnim_clink_op3_negaeri,
                                    PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                        PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                        PLAYER_ANIMMOVEFLAGS_7);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_GROAN);
}

void Player_CutsceneSleeping(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovement(play, this, &gPlayerAnim_clink_op3_wait2,
                                        PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                            PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                            PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
    }
}

void Player_PlaySlowLoopedCutsceneAnimWithSfx(PlayState* play, Player* this, LinkAnimationHeader* anim,
                                              PlayerAnimSfxEntry* animSfx) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, anim);
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        Player_PlayAnimSfx(this, animSfx);
    }
}

void Player_CutsceneSetupSleepingRestless(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->actor.shape.shadowDraw = NULL;
    Player_AnimPlaybackType7(play, this, &gPlayerAnim_clink_op3_wait1);
}

static PlayerAnimSfxEntry sAwakenAnimSfx[] = {
    { NA_SE_VO_LI_RELAX, 0x2023 },
    { NA_SE_PL_SLIPDOWN, 0x8EC },
    { NA_SE_PL_SLIPDOWN, -0x900 },
};

void Player_CutsceneAwaken(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovement(play, this, &gPlayerAnim_clink_op3_wait3, 0x9C);
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        Player_PlayAnimSfx(this, sAwakenAnimSfx);
        if (LinkAnimation_OnFrame(&this->skelAnime, 240.0f)) {
            this->actor.shape.shadowDraw = ActorShadow_DrawFeet;
        }
    }
}

static PlayerAnimSfxEntry sGetOffBedAnimSfx[] = {
    { NA_SE_PL_LAND + SURFACE_SFX_OFFSET_WOOD, 0x843 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4854 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x485A },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4860 },
};

void Player_CutsceneGetOffBed(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sGetOffBedAnimSfx);
}

void Player_CutsceneSetupBlownBackward(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, &gPlayerAnim_clink_demo_futtobi, 0x9D);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void Player_GetCsPositionByActionLength(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    f32 startX = linkCsAction->startPos.x;
    f32 startY = linkCsAction->startPos.y;
    f32 startZ = linkCsAction->startPos.z;
    f32 distX = (linkCsAction->endPos.x - startX);
    f32 distY = (linkCsAction->endPos.y - startY);
    f32 distZ = (linkCsAction->endPos.z - startZ);
    f32 percentCsActionDone =
        (f32)(play->csCtx.frames - linkCsAction->startFrame) / (f32)(linkCsAction->endFrame - linkCsAction->startFrame);

    this->actor.world.pos.x = distX * percentCsActionDone + startX;
    this->actor.world.pos.y = distY * percentCsActionDone + startY;
    this->actor.world.pos.z = distZ * percentCsActionDone + startZ;
}

static PlayerAnimSfxEntry sBlownBackwardAnimSfx[] = {
    { NA_SE_PL_BOUND, 0x1014 },
    { NA_SE_PL_BOUND, -0x101E },
};

void Player_CutsceneBlownBackward(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_GetCsPositionByActionLength(play, this, linkCsAction);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sBlownBackwardAnimSfx);
}

void Player_CutsceneRaisedByWarp(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (linkCsAction != NULL) {
        Player_GetCsPositionByActionLength(play, this, linkCsAction);
    }
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_CutsceneSetupIdle3(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_nwait, this->modelAnimType));
    Player_StopMovement(this);
}

void Player_CutsceneIdle3(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_CutsceneSetupStop(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_SetupAnimMovement(play, this,
                             PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                 PLAYER_ANIMMOVEFLAGS_7);
}

void Player_CutsceneSetDraw(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->actor.draw = Player_Draw;
}

void Player_CutsceneDrawSwordChild(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, &gPlayerAnim_clink_demo_koutai_wait);
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
            Player_CutsceneDrawSword(play, this, 1);
        }
    }
}

static PlayerAnimSfxEntry sTurnAroundSlowlyAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x300A },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3018 },
};

void Player_CutsceneTurnAroundSlowly(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_PlaySlowLoopedCutsceneAnimWithSfx(play, this, &gPlayerAnim_link_demo_furimuki2_wait,
                                             sTurnAroundSlowlyAnimSfx);
}

static PlayerAnimSfxEntry sInspectGroundCarefullyAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x400F },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4023 },
};

void Player_CutsceneInspectGroundCarefully(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_PlaySlowLoopedCutsceneAnimWithSfx(play, this, &gPlayerAnim_link_demo_nozokikomi_wait,
                                             sInspectGroundCarefullyAnimSfx);
}

void Player_CutsceneStartPassOcarina(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_demo_link_twait);
        this->genericTimer = 1;
    }

    if ((this->genericTimer != 0) && (play->csCtx.frames >= 900)) {
        this->rightHandType = PLAYER_MODELTYPE_LH_OPEN;
    } else {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
    }
}

void Player_AnimPlaybackType12PlaySfx(PlayState* play, Player* this, LinkAnimationHeader* anim,
                                      PlayerAnimSfxEntry* arg3) {
    Player_AnimPlaybackType12(play, this, anim);
    if (this->genericTimer == 0) {
        Player_PlayAnimSfx(this, arg3);
    }
}

static PlayerAnimSfxEntry sStepBackCautiouslyAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x300F },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3021 },
};

void Player_CutsceneStepBackCautiously(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_AnimPlaybackType12PlaySfx(play, this, &gPlayerAnim_clink_demo_koutai_wait, sStepBackCautiouslyAnimSfx);
}

static PlayerAnimSfxEntry sDesperateLookAtZeldasCrystalAnimSfx[] = {
    { NA_SE_PL_KNOCK, -0x84E },
};

void Player_CutsceneDesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_AnimPlaybackType12PlaySfx(play, this, &gPlayerAnim_link_demo_kakeyori_wait,
                                     sDesperateLookAtZeldasCrystalAnimSfx);
}

void Player_CutsceneSetupSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_SetupSpinAttackAnims(play, this);
}

void Player_CutsceneSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    sControlInput->press.button |= BTN_B;

    Player_ChargeSpinAttack(this, play);
}

void Player_CutsceneInspectWeapon(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_ChargeSpinAttack(this, play);
}

void Player_SetupDoNothing4(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
}

void Player_DoNothing5(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
}

void Player_CutsceneSetupKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->stateFlags3 |= PLAYER_STATE3_MIDAIR;
    this->linearVelocity = 2.0f;
    this->actor.velocity.y = -1.0f;

    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_back_downA);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

static void (*sCsKnockedToGroundDamagedFuncs[])(Player* this, PlayState* play) = {
    Player_StartKnockback,
    Player_DownFromKnockback,
    Player_GetUpFromKnockback,
};

void Player_CutsceneKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    sCsKnockedToGroundDamagedFuncs[this->genericTimer](this, play);
}

void Player_CutsceneSetupGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    Player_CutsceneDrawSword(play, this, 0);
    Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_return_to_past);
}

void Player_CutsceneSwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
        this->heldItemAction = this->itemAction = PLAYER_IA_NONE;
        this->heldItemId = ITEM_NONE;
        this->modelGroup = this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_IA_NONE);
        this->leftHandDLists = gPlayerLeftHandOpenDLs;
        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        gSaveContext.equips.buttonItems[0] = ITEM_SWORD_MASTER;
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SWORD);
    }
}

static LinkAnimationHeader* sLearnOcarinaSongAnims[] = {
    &gPlayerAnim_L_okarina_get,
    &gPlayerAnim_om_get,
};

static Vec3s sBaseSparklePos[2][2] = {
    { { -200, 700, 100 }, { 800, 600, 800 } },
    { { -200, 500, 0 }, { 600, 400, 600 } },
};

void Player_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    static Color_RGBA8 primColor = { 255, 255, 255, 0 };
    static Color_RGBA8 envColor = { 0, 128, 128, 0 };
    s32 linkAge = gSaveContext.linkAge;
    Vec3f sparklePos;
    Vec3f randOffsetSparklePos;
    Vec3s* baseSparklePos;

    Player_AnimPlaybackType12(play, this, sLearnOcarinaSongAnims[linkAge]);

    if (this->rightHandType != PLAYER_MODELTYPE_RH_FF) {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
        return;
    }

    baseSparklePos = sBaseSparklePos[gSaveContext.linkAge];

    randOffsetSparklePos.x = baseSparklePos[0].x + Rand_CenteredFloat(baseSparklePos[1].x);
    randOffsetSparklePos.y = baseSparklePos[0].y + Rand_CenteredFloat(baseSparklePos[1].y);
    randOffsetSparklePos.z = baseSparklePos[0].z + Rand_CenteredFloat(baseSparklePos[1].z);

    SkinMatrix_Vec3fMtxFMultXYZ(&this->shieldMf, &randOffsetSparklePos, &sparklePos);

    EffectSsKiraKira_SpawnDispersed(play, &sparklePos, &zeroVec, &zeroVec, &primColor, &envColor, 600, -10);
}

void Player_CutsceneGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_CutsceneEnd(play, this, linkCsAction);
    } else if (this->genericTimer == 0) {
        Item_Give(play, ITEM_SWORD_MASTER);
        Player_CutsceneDrawSword(play, this, 0);
    } else {
        Player_PlaySwingSwordSfx(this);
    }
}

void Player_CutsceneGanonKillCombo(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupMeleeAttack(this, 0.0f, 99.0f, this->skelAnime.endFrame - 8.0f);
    }

    if (this->heldItemAction != PLAYER_IA_SWORD_MASTER) {
        Player_CutsceneDrawSword(play, this, 1);
    }
}

void Player_CutsceneEnd(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    if (Player_IsSwimming(this)) {
        Player_SetupSwimIdle(play, this);
        Player_ResetSubCam(play, this);
    } else {
        Player_ClearLookAndAttention(this, play);
        if (!Player_SetupSpeakOrCheck(this, play)) {
            Player_SetupGetItemOrHoldBehavior(this, play);
        }
    }

    this->csMode = PLAYER_CSMODE_NONE;
    this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
}

void Player_CutsceneSetPosAndYaw(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    this->actor.world.pos.x = linkCsAction->startPos.x;
    this->actor.world.pos.y = linkCsAction->startPos.y;
    if ((play->sceneId == SCENE_KOKIRI_FOREST) && !LINK_IS_ADULT) {
        this->actor.world.pos.y -= 1.0f;
    }
    this->actor.world.pos.z = linkCsAction->startPos.z;
    this->currentYaw = this->actor.shape.rot.y = linkCsAction->rot.y;
}

void Player_CutsceneSetPosAndYawIfOutsideStartRange(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    f32 distToStartX = linkCsAction->startPos.x - (s32)this->actor.world.pos.x;
    f32 distToStartY = linkCsAction->startPos.y - (s32)this->actor.world.pos.y;
    f32 distToStartZ = linkCsAction->startPos.z - (s32)this->actor.world.pos.z;
    f32 distToStartXYZ = sqrtf(SQ(distToStartX) + SQ(distToStartY) + SQ(distToStartZ));
    s16 yawDiff = linkCsAction->rot.y - this->actor.shape.rot.y;

    if ((this->linearVelocity == 0.0f) && ((distToStartXYZ > 50.0f) || (ABS(yawDiff) > DEG_TO_BINANG(90.0f)))) {
        Player_CutsceneSetPosAndYaw(play, this, linkCsAction);
    }

    this->skelAnime.moveFlags = 0;
    Player_ClearRootLimbPosY(this);
}

void Player_CsModePlayback(PlayState* play, Player* this, CsCmdActorAction* linkCsAction, CutsceneModeEntry* csMode) {
    if (csMode->playbackFuncID > 0) {
        csModePlaybackFuncs[csMode->playbackFuncID](play, this, csMode->ptr);
    } else if (csMode->playbackFuncID < 0) {
        csMode->func(play, this, linkCsAction);
    }

    if ((sPrevSkelAnimeMoveFlags & PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE) &&
        !(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE)) {
        this->skelAnime.morphTable[0].y /= this->ageProperties->translationScale;
        sPrevSkelAnimeMoveFlags = 0;
    }
}

void Player_CutsceneDetatchHeldActor(PlayState* play, Player* this, s32 csMode) {
    if ((csMode != PLAYER_CSMODE_IDLE) && (csMode != PLAYER_CSMODE_WAIT) && (csMode != PLAYER_CSMODE_IDLE_4) &&
        (csMode != PLAYER_CSMODE_END)) {
        Player_DetatchHeldActor(play, this);
    }
}

void Player_CutsceneUnk6Update(PlayState* play, Player* this, CsCmdActorAction* linkCsAction) {
    CsCmdActorAction* curlinkCsAction = play->csCtx.linkAction;
    s32 pad;
    s32 currentCsAction;

    if (play->csCtx.state == CS_STATE_UNSKIPPABLE_INIT) {
        Actor_SetPlayerCutscene(play, NULL, PLAYER_CSMODE_END);
        this->csAction = 0;
        Player_StopMovement(this);
        return;
    }

    if (curlinkCsAction == NULL) {
        this->actor.flags &= ~ACTOR_FLAG_6;
        return;
    }

    if (this->csAction != curlinkCsAction->action) {
        currentCsAction = slinkCsActions[curlinkCsAction->action];
        if (currentCsAction >= 0) {
            if ((currentCsAction == 3) || (currentCsAction == 4)) {
                Player_CutsceneSetPosAndYawIfOutsideStartRange(play, this, curlinkCsAction);
            } else {
                Player_CutsceneSetPosAndYaw(play, this, curlinkCsAction);
            }
        }

        sPrevSkelAnimeMoveFlags = this->skelAnime.moveFlags;

        Player_EndAnimMovement(this);
        osSyncPrintf("TOOL MODE=%d\n", currentCsAction);
        Player_CutsceneDetatchHeldActor(play, this, ABS(currentCsAction));
        Player_CsModePlayback(play, this, curlinkCsAction, &sCutsceneModeInitFuncs[ABS(currentCsAction)]);

        this->genericTimer = 0;
        this->genericVar = 0;
        this->csAction = curlinkCsAction->action;
    }

    currentCsAction = slinkCsActions[this->csAction];
    Player_CsModePlayback(play, this, curlinkCsAction, &sCutsceneModeUpdateFuncs[ABS(currentCsAction)]);
}

void Player_StartCutscene(Player* this, PlayState* play) {
    if (this->csMode != this->prevCsMode) {
        sPrevSkelAnimeMoveFlags = this->skelAnime.moveFlags;

        Player_EndAnimMovement(this);
        this->prevCsMode = this->csMode;
        osSyncPrintf("DEMO MODE=%d\n", this->csMode);
        Player_CutsceneDetatchHeldActor(play, this, this->csMode);
        Player_CsModePlayback(play, this, NULL, &sCutsceneModeInitFuncs[this->csMode]);
    }

    Player_CsModePlayback(play, this, NULL, &sCutsceneModeUpdateFuncs[this->csMode]);
}

s32 Player_IsDroppingFish(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (Player_DropItemFromBottle == this->actionFunc) && (this->itemAction == PLAYER_IA_BOTTLE_FISH);
}

s32 Player_StartFishing(PlayState* play) {
    Player* this = GET_PLAYER(play);

    Player_ResetAttributesAndHeldActor(play, this);
    Player_UseItem(play, this, ITEM_FISHING_POLE);
    return 1;
}

s32 Player_SetupRestrainedByEnemy(PlayState* play, Player* this) {
    if (!Player_InBlockingCsMode(play, this) && (this->invincibilityTimer >= 0) && !Player_IsShootingHookshot(this) &&
        !(this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetActionFunc(play, this, Player_RestrainedByEnemy, 0);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_re_dead_attack);
        this->stateFlags2 |= PLAYER_STATE2_RESTRAINED_BY_ENEMY;
        Player_ClearAttentionModeAndStopMoving(this);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HELD);
        return true;
    }

    return false;
}

s32 Player_SetupPlayerCutscene(PlayState* play, Actor* actor, s32 csMode) {
    Player* this = GET_PLAYER(play);

    if (!Player_InBlockingCsMode(play, this)) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        this->csMode = csMode;
        this->csTargetActor = actor;
        Player_ClearAttentionModeAndStopMoving(this);
        return 1;
    }

    return 0;
}

void Player_SetupStandingStillMorph(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_StandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, Player_GetStandingStillAnim(this));
    this->currentYaw = this->actor.shape.rot.y;
}

// Returns true if player is out of health and not in blocking cutscene mode
s32 Player_InflictDamageAndCheckForDeath(PlayState* play, s32 damage) {
    Player* this = GET_PLAYER(play);

    if (!Player_InBlockingCsMode(play, this) && !Player_InflictDamage(play, this, damage)) {
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
        return 1;
    }

    return 0;
}

// Start talking with the given actor
void Player_StartTalkingWithActor(PlayState* play, Actor* actor) {
    Player* this = GET_PLAYER(play);
    s32 pad;

    if ((this->talkActor != NULL) || (actor == this->naviActor) ||
        CHECK_FLAG_ALL(actor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18)) {
        actor->flags |= ACTOR_FLAG_8;
    }

    this->talkActor = actor;
    this->exchangeItemId = EXCH_ITEM_NONE;

    if (actor->textId == 0xFFFF) {
        Actor_SetPlayerCutscene(play, actor, PLAYER_CSMODE_IDLE);
        actor->flags |= ACTOR_FLAG_8;
        Player_UnequipItem(play, this);
    } else {
        if (this->actor.flags & ACTOR_FLAG_8) {
            this->actor.textId = 0;
        } else {
            this->actor.flags |= ACTOR_FLAG_8;
            this->actor.textId = actor->textId;
        }

        if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
            s32 timerPreserved = this->genericTimer;

            Player_UnequipItem(play, this);
            Player_SetupTalkWithActor(play, this);

            this->genericTimer = timerPreserved;
        } else {
            if (Player_IsSwimming(this)) {
                Player_SetupMiniCsFunc(play, this, Player_SetupTalkWithActor);
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
            } else if ((actor->category != ACTORCAT_NPC) || (this->heldItemAction == PLAYER_IA_FISHING_POLE)) {
                Player_SetupTalkWithActor(play, this);

                if (!Player_IsUnfriendlyZTargeting(this)) {
                    if ((actor != this->naviActor) && (actor->xzDistToPlayer < 40.0f)) {
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_normal_backspace);
                    } else {
                        Player_PlayAnimLoop(play, this, Player_GetStandingStillAnim(this));
                    }
                }
            } else {
                Player_SetupMiniCsFunc(play, this, Player_SetupTalkWithActor);
                Player_PlayAnimOnceSlowed(play, this,
                                          (actor->xzDistToPlayer < 40.0f) ? &gPlayerAnim_link_normal_backspace
                                                                          : &gPlayerAnim_link_normal_talk_free);
            }

            if (this->skelAnime.animation == &gPlayerAnim_link_normal_backspace) {
                Player_SetupAnimMovement(play, this,
                                         PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                             PLAYER_ANIMMOVEFLAGS_NO_MOVE);
            }

            Player_ClearAttentionModeAndStopMoving(this);
        }

        this->stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE;
    }

    if ((this->naviActor == this->talkActor) && ((this->talkActor->textId & 0xFF00) != 0x200)) {
        this->naviActor->flags |= ACTOR_FLAG_8;
        Player_SetCameraTurnAround(play, 0xB);
    }
}

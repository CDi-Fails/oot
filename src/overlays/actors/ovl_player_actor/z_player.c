/*
 * File: z_player.c
 * Overlay: ovl_player_actor
 * Description: Link
 */

#include "ultra64.h"
#include "global.h"

#include "overlays/actors/ovl_Bg_Heavy_Block/z_bg_heavy_block.h"
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

typedef enum {
    /* 0x00 */ KNOB_ANIM_ADULT_L,
    /* 0x01 */ KNOB_ANIM_CHILD_L,
    /* 0x02 */ KNOB_ANIM_ADULT_R,
    /* 0x03 */ KNOB_ANIM_CHILD_R
} KnobDoorAnim;

typedef struct {
    /* 0x00 */ u8 itemId;
    /* 0x02 */ s16 actorId;
} ExplosiveInfo; // size = 0x04

typedef struct {
    /* 0x00 */ s16 actorId;
    /* 0x02 */ u8 itemId;
    /* 0x03 */ u8 actionParam;
    /* 0x04 */ u8 textId;
} BottleCatchInfo; // size = 0x06

typedef struct {
    /* 0x00 */ s16 actorId;
    /* 0x02 */ s16 actorParams;
} BottleDropInfo; // size = 0x04

typedef struct {
    /* 0x00 */ s8 damage;
    /* 0x01 */ u8 unk_01;
    /* 0x02 */ u8 unk_02;
    /* 0x03 */ u8 unk_03;
    /* 0x04 */ u16 sfxId;
} FallImpactInfo; // size = 0x06

typedef struct {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ s16 yaw;
} SpecialRespawnInfo; // size = 0x10

typedef struct {
    /* 0x00 */ u16 sfxId;
    /* 0x02 */ s16 field; // contains frame to play anim on, sign determines whether to stop processing entries
} PlayerAnimSfxEntry; // size = 0x04

typedef struct {
    /* 0x00 */ u16 unk_00;
    /* 0x02 */ s16 unk_02;
} struct_808551A4; // size = 0x04

typedef struct {
    /* 0x00 */ LinkAnimationHeader* anim;
    /* 0x04 */ u8 itemChangeFrame;
} ItemChangeInfo; // size = 0x08

typedef struct {
    /* 0x00 */ LinkAnimationHeader* unk_00;
    /* 0x04 */ LinkAnimationHeader* unk_04;
    /* 0x08 */ u8 unk_08;
    /* 0x09 */ u8 unk_09;
} struct_80854554; // size = 0x0C

typedef struct {
    /* 0x00 */ LinkAnimationHeader* startAnim;
    /* 0x04 */ LinkAnimationHeader* endAnim;
    /* 0x08 */ LinkAnimationHeader* fightEndAnim;
    /* 0x0C */ u8 startFrame;
    /* 0x0D */ u8 endFrame;
} struct_80854190; // size = 0x10

typedef struct {
    /* 0x00 */ LinkAnimationHeader* anim;
    /* 0x04 */ f32 unk_04;
    /* 0x04 */ f32 unk_08;
} struct_80854578; // size = 0x0C

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
void Player_ChangeItem(PlayState* play, Player* this, s8 actionParam);
s32 Player_SetupStartZTargetingDefend(Player* this, PlayState* play);
s32 Player_SetupStartZTargetingDefend2(Player* this, PlayState* play);
s32 Player_StartChangeItem(Player* this, PlayState* play);
s32 func_80834B5C(Player* this, PlayState* play);
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
s32 Player_SetupItemCutsceneOrCUp(Player* this, PlayState* play);
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
s32 Player_SetupStartGrabPushPullWall(Player* this, PlayState* play);
void Player_UnfriendlyZTargetStandingStill(Player* this, PlayState* play);
void Player_FriendlyZTargetStandingStill(Player* this, PlayState* play);
void Player_StandingStill(Player* this, PlayState* play);
void Player_EndSidewalk(Player* this, PlayState* play);
void Player_FriendlyBackwalk(Player* this, PlayState* play);
void func_8084170C(Player* this, PlayState* play);
void func_808417FC(Player* this, PlayState* play);
void Player_Sidewalk(Player* this, PlayState* play);
void Player_Turn(Player* this, PlayState* play);
void Player_Run(Player* this, PlayState* play);
void Player_ZTargetingRun(Player* this, PlayState* play);
void func_8084279C(Player* this, PlayState* play);
void Player_UnfriendlyBackwalk(Player* this, PlayState* play);
void Player_EndUnfriendlyBackwalk(Player* this, PlayState* play);
void func_80843188(Player* this, PlayState* play);
void func_808435C4(Player* this, PlayState* play);
void func_8084370C(Player* this, PlayState* play);
void func_8084377C(Player* this, PlayState* play);
void func_80843954(Player* this, PlayState* play);
void func_80843A38(Player* this, PlayState* play);
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
void func_80845CA4(Player* this, PlayState* play);
void func_80845EF8(Player* this, PlayState* play);
void Player_LiftActor(Player* this, PlayState* play);
void Player_ThrowStonePillar(Player* this, PlayState* play);
void Player_LiftSilverBoulder(Player* this, PlayState* play);
void Player_ThrowSilverBoulder(Player* this, PlayState* play);
void Player_FailToLiftActor(Player* this, PlayState* play);
void Player_SetupPutDownActor(Player* this, PlayState* play);
void Player_StartThrowActor(Player* this, PlayState* play);
void Player_SpawnNoUpdateOrDraw(PlayState* play, Player* this);
void Player_SpawnFromBlueWarp(PlayState* play, Player* this);
void Player_SpawnFromTimeTravel(PlayState* play, Player* this);
void Player_SpawnOpeningDoor(PlayState* play, Player* this);
void Player_SpawnExitingGrotto(PlayState* play, Player* this);
void Player_SpawnWithKnockback(PlayState* play, Player* this);
void Player_SpawnFromWarpSong(PlayState* play, Player* this);
void Player_SetupSpawnFromFaroresWind(PlayState* play, Player* this);
void func_8084B1D8(Player* this, PlayState* play);
void Player_TalkWithActor(Player* this, PlayState* play);
void Player_GrabPushPullWall(Player* this, PlayState* play);
void Player_PushWall(Player* this, PlayState* play);
void Player_PullWall(Player* this, PlayState* play);
void Player_GrabLedge(Player* this, PlayState* play);
void Player_ClimbOntoLedge(Player* this, PlayState* play);
void Player_ClimbDownFromLedge(Player* this, PlayState* play);
void Player_UpdateCommon(Player* this, PlayState* play, Input* input);
void func_8084C5F8(Player* this, PlayState* play);
void func_8084C760(Player* this, PlayState* play);
void func_8084C81C(Player* this, PlayState* play);
void Player_RideHorse(Player* this, PlayState* play);
void func_8084D3E4(Player* this, PlayState* play);
void Player_UpdateSwimIdle(Player* this, PlayState* play);
void Player_SpawnSwimming(Player* this, PlayState* play);
void Player_Swim(Player* this, PlayState* play);
void func_8084DAB4(Player* this, PlayState* play);
void func_8084DC48(Player* this, PlayState* play);
void func_8084E1EC(Player* this, PlayState* play);
void func_8084E30C(Player* this, PlayState* play);
void Player_SetupDrown(Player* this, PlayState* play);
void func_8084E3C4(Player* this, PlayState* play);
void Player_ThrowDekuNut(Player* this, PlayState* play);
void func_8084E6D4(Player* this, PlayState* play);
void func_8084E9AC(Player* this, PlayState* play);
void func_8084EAC0(Player* this, PlayState* play);
void Player_SwingBottle(Player* this, PlayState* play);
void func_8084EED8(Player* this, PlayState* play);
void Player_DropItemFromBottle(Player* this, PlayState* play);
void func_8084F104(Player* this, PlayState* play);
void func_8084F390(Player* this, PlayState* play);
void func_8084F608(Player* this, PlayState* play);
void func_8084F698(Player* this, PlayState* play);
void func_8084F710(Player* this, PlayState* play);
void func_8084F88C(Player* this, PlayState* play);
void Player_SetupOpenDoorFromSpawn(Player* this, PlayState* play);
void func_8084F9C0(Player* this, PlayState* play);
void Player_ShootingGalleryPlay(Player* this, PlayState* play);
void Player_FrozenInIce(Player* this, PlayState* play);
void Player_SetupElectricShock(Player* this, PlayState* play);
s32 func_8084FCAC(Player* this, PlayState* play);
void Player_BowStringMoveAfterShot(Player* this);
void func_8085002C(Player* this);
s32 Player_SetupMeleeWeaponAttack(Player* this, PlayState* play);
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
void Player_CutsceneSetupSwimIdle(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSurfaceFromDive(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneIdle(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneTurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupIdle(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneWait(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneTurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupEnterWarp(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneEnterWarp(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupFightStance(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneFightStance(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851998(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808519C0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupSwordIntoPedestal(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSwordIntoPedestal(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupWarpToSages(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneWarpToSages(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneKnockedToGround(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupStartPlayOcarina(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneDrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneCloseEyes(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneOpenEyes(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupGetItemInWater(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupSleeping(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSleeping(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupSleepingRestless(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneAwaken(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneGetOffBed(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupBlownBackward(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneBlownBackward(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneRaisedByWarp(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808521F4(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneIdle3(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_8085225C(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852280(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneInspectGroundCarefully(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneStartPassOcarina(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneDrawSwordChild(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneTurnAroundSlowly(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneDesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneStepBackCautiously(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneInspectWeapon(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_SetupDoNothing4(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_DoNothing5(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSetupGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneSwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneGanonKillCombo(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_CutsceneEnd(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808529D0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852C50(PlayState* play, Player* this, CsCmdActorAction* arg2);
void Player_StartCutscene(Player* this, PlayState* play);
s32 Player_IsDroppingFish(PlayState* play);
s32 Player_StartFishing(PlayState* play);
s32 Player_SetupRestrainedByEnemy(PlayState* play, Player* this);
s32 Player_SetupPlayerCutscene(PlayState* play, Actor* actor, s32 csMode);
void Player_SetupStandingStillMorph(Player* this, PlayState* play);
s32 Player_SetupInflictDamage(PlayState* play, s32 damage);
void Player_StartTalkingWithActor(PlayState* play, Actor* actor);

// .bss part 1
static s32 D_80858AA0;
static s32 D_80858AA4;
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
        0,                   // ageProperties->ageVoiceSfxOffset
        0x80,                // ageProperties->ageMoveSfxOffset
        &gPlayerAnim_002718, // ageProperties->chestOpenAnim
        &gPlayerAnim_002720, // ageProperties->timeTravelStartAnim
        &gPlayerAnim_002838, // ageProperties->timeTravelEndAnim
        &gPlayerAnim_002E70, // ageProperties->startClimb1Anim
        &gPlayerAnim_002E78, // ageProperties->startClimb2Anim
        {
            &gPlayerAnim_002E80, // ageProperties->verticalClimbAnim[0]
            &gPlayerAnim_002E88, // ageProperties->verticalClimbAnim[1]
            &gPlayerAnim_002D90, // ageProperties->verticalClimbAnim[2]
            &gPlayerAnim_002D98  // ageProperties->verticalClimbAnim[3]
        },
        {
            &gPlayerAnim_002D70, // ageProperties->horizontalClimbAnim[0]
            &gPlayerAnim_002D78  // ageProperties->horizontalClimbAnim[1]
        },
        {
            &gPlayerAnim_002E50, // ageProperties->endClimb1Anim[0]
            &gPlayerAnim_002E58  // ageProperties->endClimb1Anim[1]
        },
        {
            &gPlayerAnim_002E68, // ageProperties->endClimb2Anim[0]
            &gPlayerAnim_002E60  // ageProperties->endClimb2Anim[1]
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
        0x20,                // ageProperties->ageVoiceSfxOffset
        0,                   // ageProperties->ageMoveSfxOffset
        &gPlayerAnim_002318, // ageProperties->chestOpenAnim
        &gPlayerAnim_002360, // ageProperties->timeTravelStartAnim
        &gPlayerAnim_0023A8, // ageProperties->timeTravelEndAnim
        &gPlayerAnim_0023E0, // ageProperties->startClimb1Anim
        &gPlayerAnim_0023E8, // ageProperties->startClimb2Anim
        {
            &gPlayerAnim_0023F0, // ageProperties->verticalClimbAnim[0]
            &gPlayerAnim_0023F8, // ageProperties->verticalClimbAnim[1]
            &gPlayerAnim_002D90, // ageProperties->verticalClimbAnim[2]
            &gPlayerAnim_002D98  // ageProperties->verticalClimbAnim[3]
        },
        {
            &gPlayerAnim_002D70, // ageProperties->horizontalClimbAnim[0]
            &gPlayerAnim_002D78  // ageProperties->horizontalClimbAnim[1]
        },
        {
            &gPlayerAnim_0023C0, // ageProperties->endClimb1Anim[0]
            &gPlayerAnim_0023C8  // ageProperties->endClimb1Anim[1]
        },
        {
            &gPlayerAnim_0023D8, // ageProperties->endClimb2Anim[0]
            &gPlayerAnim_0023D0  // ageProperties->endClimb2Anim[1]
        },
    },
};

static u32 sDebugModeFlag = false;
static f32 sAnalogStickDistance = 0.0f;
static s16 sAnalogStickAngle = 0;
static s16 sCameraOffsetAnalogStickAngle = 0;
static s32 D_808535E0 = 0;
static s32 sFloorSpecialProperty = BGCHECK_FLOORSPECIALPROPERTY_NONE;
static f32 sWaterSpeedScale = 1.0f;
static f32 sInvertedWaterSpeedScale = 1.0f;
static u32 sTouchedWallFlags = 0;
static u32 sConveyorSpeedIndex = 0;
static s16 sIsFloorConveyor = false;
static s16 sConveyorYaw = 0;
static f32 sPlayerYDistToFloor = 0.0f;
static s32 sFloorProperty = BGCHECK_FLOORPROPERTY_NONE;
static s32 sYawToTouchedWall = 0;
static s32 sYawToTouchedWall2 = 0;
static s16 D_80853610 = 0;
static s32 sUsingItemAlreadyInHand = 0;
static s32 sUsingItemAlreadyInHand2 = 0;

static u16 sInterruptableSfx[] = {
    NA_SE_VO_LI_SWEAT,
    NA_SE_VO_LI_SNEEZE,
    NA_SE_VO_LI_RELAX,
    NA_SE_VO_LI_FALL_L,
};

static GetItemEntry sGetItemTable[] = {
    GET_ITEM(ITEM_BOMBS_5, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUTS_5, OBJECT_GI_NUTS, GID_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBCHU, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOW, OBJECT_GI_BOW, GID_BOW, 0x31, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SLINGSHOT, OBJECT_GI_PACHINKO, GID_SLINGSHOT, 0x30, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOOMERANG, OBJECT_GI_BOOMERANG, GID_BOOMERANG, 0x35, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STICK, OBJECT_GI_STICK, GID_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HOOKSHOT, OBJECT_GI_HOOKSHOT, GID_HOOKSHOT, 0x36, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LONGSHOT, OBJECT_GI_HOOKSHOT, GID_LONGSHOT, 0x4F, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LENS, OBJECT_GI_GLASSES, GID_LENS, 0x39, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LETTER_ZELDA, OBJECT_GI_LETTER, GID_LETTER_ZELDA, 0x69, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_OCARINA_TIME, OBJECT_GI_OCARINA, GID_OCARINA_TIME, 0x3A, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_HAMMER, OBJECT_GI_HAMMER, GID_HAMMER, 0x38, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_COJIRO, OBJECT_GI_NIWATORI, GID_COJIRO, 0x02, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOTTLE, OBJECT_GI_BOTTLE, GID_BOTTLE, 0x42, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POTION_RED, OBJECT_GI_LIQUID, GID_POTION_RED, 0x43, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POTION_GREEN, OBJECT_GI_LIQUID, GID_POTION_GREEN, 0x44, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POTION_BLUE, OBJECT_GI_LIQUID, GID_POTION_BLUE, 0x45, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_FAIRY, OBJECT_GI_BOTTLE, GID_BOTTLE, 0x46, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MILK_BOTTLE, OBJECT_GI_MILK, GID_MILK, 0x98, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LETTER_RUTO, OBJECT_GI_BOTTLE_LETTER, GID_LETTER_RUTO, 0x99, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BEAN, OBJECT_GI_BEAN, GID_BEAN, 0x48, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_MASK_SKULL, OBJECT_GI_SKJ_MASK, GID_MASK_SKULL, 0x10, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_SPOOKY, OBJECT_GI_REDEAD_MASK, GID_MASK_SPOOKY, 0x11, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_CHICKEN, OBJECT_GI_NIWATORI, GID_CHICKEN, 0x48, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_KEATON, OBJECT_GI_KI_TAN_MASK, GID_MASK_KEATON, 0x12, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_BUNNY, OBJECT_GI_RABIT_MASK, GID_MASK_BUNNY, 0x13, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_TRUTH, OBJECT_GI_TRUTH_MASK, GID_MASK_TRUTH, 0x17, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POCKET_EGG, OBJECT_GI_EGG, GID_EGG, 0x01, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POCKET_CUCCO, OBJECT_GI_NIWATORI, GID_CHICKEN, 0x48, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ODD_MUSHROOM, OBJECT_GI_MUSHROOM, GID_ODD_MUSHROOM, 0x03, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ODD_POTION, OBJECT_GI_POWDER, GID_ODD_POTION, 0x04, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SAW, OBJECT_GI_SAW, GID_SAW, 0x05, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SWORD_BROKEN, OBJECT_GI_BROKENSWORD, GID_SWORD_BROKEN, 0x08, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_PRESCRIPTION, OBJECT_GI_PRESCRIPTION, GID_PRESCRIPTION, 0x09, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_FROG, OBJECT_GI_FROG, GID_FROG, 0x0D, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_EYEDROPS, OBJECT_GI_EYE_LOTION, GID_EYEDROPS, 0x0E, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_CLAIM_CHECK, OBJECT_GI_TICKETSTONE, GID_CLAIM_CHECK, 0x0A, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SWORD_KOKIRI, OBJECT_GI_SWORD_1, GID_SWORD_KOKIRI, 0xA4, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SWORD_BGS, OBJECT_GI_LONGSWORD, GID_SWORD_BGS, 0x4B, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SHIELD_DEKU, OBJECT_GI_SHIELD_1, GID_SHIELD_DEKU, 0x4C, 0xA0, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SHIELD_HYLIAN, OBJECT_GI_SHIELD_2, GID_SHIELD_HYLIAN, 0x4D, 0xA0, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SHIELD_MIRROR, OBJECT_GI_SHIELD_3, GID_SHIELD_MIRROR, 0x4E, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_TUNIC_GORON, OBJECT_GI_CLOTHES, GID_TUNIC_GORON, 0x50, 0xA0, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_TUNIC_ZORA, OBJECT_GI_CLOTHES, GID_TUNIC_ZORA, 0x51, 0xA0, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOOTS_IRON, OBJECT_GI_BOOTS_2, GID_BOOTS_IRON, 0x53, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOOTS_HOVER, OBJECT_GI_HOVERBOOTS, GID_BOOTS_HOVER, 0x54, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_QUIVER_40, OBJECT_GI_ARROWCASE, GID_QUIVER_40, 0x56, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_QUIVER_50, OBJECT_GI_ARROWCASE, GID_QUIVER_50, 0x57, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOMB_BAG_20, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_20, 0x58, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOMB_BAG_30, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_30, 0x59, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOMB_BAG_40, OBJECT_GI_BOMBPOUCH, GID_BOMB_BAG_40, 0x5A, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_GAUNTLETS_SILVER, OBJECT_GI_GLOVES, GID_GAUNTLETS_SILVER, 0x5B, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_GAUNTLETS_GOLD, OBJECT_GI_GLOVES, GID_GAUNTLETS_GOLD, 0x5C, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SCALE_SILVER, OBJECT_GI_SCALE, GID_SCALE_SILVER, 0xCD, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SCALE_GOLDEN, OBJECT_GI_SCALE, GID_SCALE_GOLDEN, 0xCE, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STONE_OF_AGONY, OBJECT_GI_MAP, GID_STONE_OF_AGONY, 0x68, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_GERUDO_CARD, OBJECT_GI_GERUDO, GID_GERUDO_CARD, 0x7B, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_OCARINA_FAIRY, OBJECT_GI_OCARINA_0, GID_OCARINA_FAIRY, 0x3A, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SEEDS, OBJECT_GI_SEED, GID_SEEDS, 0xDC, 0x50, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HEART_CONTAINER, OBJECT_GI_HEARTS, GID_HEART_CONTAINER, 0xC6, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_HEART_PIECE_2, OBJECT_GI_HEARTS, GID_HEART_PIECE, 0xC2, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_KEY_BOSS, OBJECT_GI_BOSSKEY, GID_KEY_BOSS, 0xC7, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_COMPASS, OBJECT_GI_COMPASS, GID_COMPASS, 0x67, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_DUNGEON_MAP, OBJECT_GI_MAP, GID_DUNGEON_MAP, 0x66, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_KEY_SMALL, OBJECT_GI_KEY, GID_KEY_SMALL, 0x60, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_MAGIC_SMALL, OBJECT_GI_MAGICPOT, GID_MAGIC_SMALL, 0x52, 0x6F, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_MAGIC_LARGE, OBJECT_GI_MAGICPOT, GID_MAGIC_LARGE, 0x52, 0x6E, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_WALLET_ADULT, OBJECT_GI_PURSE, GID_WALLET_ADULT, 0x5E, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_WALLET_GIANT, OBJECT_GI_PURSE, GID_WALLET_GIANT, 0x5F, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_WEIRD_EGG, OBJECT_GI_EGG, GID_EGG, 0x9A, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_HEART, OBJECT_GI_HEART, GID_HEART, 0x55, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROWS_SMALL, OBJECT_GI_ARROW, GID_ARROWS_SMALL, 0xE6, 0x48, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_ARROWS_MEDIUM, OBJECT_GI_ARROW, GID_ARROWS_MEDIUM, 0xE6, 0x49, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_ARROWS_LARGE, OBJECT_GI_ARROW, GID_ARROWS_LARGE, 0xE6, 0x4A, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_GREEN, OBJECT_GI_RUPY, GID_RUPEE_GREEN, 0x6F, 0x00, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_BLUE, OBJECT_GI_RUPY, GID_RUPEE_BLUE, 0xCC, 0x01, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_RED, OBJECT_GI_RUPY, GID_RUPEE_RED, 0xF0, 0x02, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HEART_CONTAINER, OBJECT_GI_HEARTS, GID_HEART_CONTAINER, 0xC6, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MILK, OBJECT_GI_MILK, GID_MILK, 0x98, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_GORON, OBJECT_GI_GOLONMASK, GID_MASK_GORON, 0x14, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_ZORA, OBJECT_GI_ZORAMASK, GID_MASK_ZORA, 0x15, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_GERUDO, OBJECT_GI_GERUDOMASK, GID_MASK_GERUDO, 0x16, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BRACELET, OBJECT_GI_BRACELET, GID_BRACELET, 0x79, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_RUPEE_PURPLE, OBJECT_GI_RUPY, GID_RUPEE_PURPLE, 0xF1, 0x14, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_GOLD, OBJECT_GI_RUPY, GID_RUPEE_GOLD, 0xF2, 0x13, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SWORD_BGS, OBJECT_GI_LONGSWORD, GID_SWORD_BGS, 0x0C, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROW_FIRE, OBJECT_GI_M_ARROW, GID_ARROW_FIRE, 0x70, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROW_ICE, OBJECT_GI_M_ARROW, GID_ARROW_ICE, 0x71, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROW_LIGHT, OBJECT_GI_M_ARROW, GID_ARROW_LIGHT, 0x72, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SKULL_TOKEN, OBJECT_GI_SUTARU, GID_SKULL_TOKEN, 0xB4, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_DINS_FIRE, OBJECT_GI_GODDESS, GID_DINS_FIRE, 0xAD, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_FARORES_WIND, OBJECT_GI_GODDESS, GID_FARORES_WIND, 0xAE, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_NAYRUS_LOVE, OBJECT_GI_GODDESS, GID_NAYRUS_LOVE, 0xAF, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BULLET_BAG_30, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG, 0x07, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BULLET_BAG_40, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG, 0x07, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STICKS_5, OBJECT_GI_STICK, GID_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_STICKS_10, OBJECT_GI_STICK, GID_STICK, 0x37, 0x0D, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUTS_5, OBJECT_GI_NUTS, GID_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUTS_10, OBJECT_GI_NUTS, GID_NUTS, 0x34, 0x0C, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMB, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBS_10, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBS_20, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBS_30, OBJECT_GI_BOMB_1, GID_BOMB, 0x32, 0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SEEDS_30, OBJECT_GI_SEED, GID_SEEDS, 0xDC, 0x50, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBCHUS_5, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBCHUS_20, OBJECT_GI_BOMB_2, GID_BOMBCHU, 0x33, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_FISH, OBJECT_GI_FISH, GID_FISH, 0x47, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BUG, OBJECT_GI_INSECT, GID_BUG, 0x7A, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BLUE_FIRE, OBJECT_GI_FIRE, GID_BLUE_FIRE, 0x5D, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POE, OBJECT_GI_GHOST, GID_POE, 0x97, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BIG_POE, OBJECT_GI_GHOST, GID_BIG_POE, 0xF9, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_KEY_SMALL, OBJECT_GI_KEY, GID_KEY_SMALL, 0xF3, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_GREEN, OBJECT_GI_RUPY, GID_RUPEE_GREEN, 0xF4, 0x00, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_BLUE, OBJECT_GI_RUPY, GID_RUPEE_BLUE, 0xF5, 0x01, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_RED, OBJECT_GI_RUPY, GID_RUPEE_RED, 0xF6, 0x02, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_PURPLE, OBJECT_GI_RUPY, GID_RUPEE_PURPLE, 0xF7, 0x14, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HEART_PIECE_2, OBJECT_GI_HEARTS, GID_HEART_PIECE, 0xFA, 0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STICK_UPGRADE_20, OBJECT_GI_STICK, GID_STICK, 0x90, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_STICK_UPGRADE_30, OBJECT_GI_STICK, GID_STICK, 0x91, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUT_UPGRADE_30, OBJECT_GI_NUTS, GID_NUTS, 0xA7, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUT_UPGRADE_40, OBJECT_GI_NUTS, GID_NUTS, 0xA8, 0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BULLET_BAG_50, OBJECT_GI_DEKUPOUCH, GID_BULLET_BAG_50, 0x6C, 0x80, CHEST_ANIM_LONG),
    GET_ITEM_NONE,
    GET_ITEM_NONE,
};

#define GET_PLAYER_ANIM(group, type) sPlayerAnimations[group * PLAYER_ANIMTYPE_MAX + type]

static LinkAnimationHeader* sPlayerAnimations[PLAYER_ANIMGROUP_MAX * PLAYER_ANIMTYPE_MAX] = {
    /* PLAYER_ANIMGROUP_STANDING_STILL */
    &gPlayerAnim_003240,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003238,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003238,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BE0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003240,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003240,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_WALKING */
    &gPlayerAnim_003290,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003268,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003268,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BF8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003290,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003290,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_RUNNING */
    &gPlayerAnim_003140,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002B38,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003138,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002B40,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003140,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003140,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_RUNNING_DAMAGED */
    &gPlayerAnim_002E98,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0029E8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002E98,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_0029F0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002E98,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002E98,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_IRON_BOOTS */
    &gPlayerAnim_002FB0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002FA8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002FB0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002A40,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002FB0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002FB0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_FIGHTING_LEFT_OF_ENEMY */
    &gPlayerAnim_003220,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002590,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002590,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BC0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003220,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003220,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_FIGHTING_RIGHT_OF_ENEMY */
    &gPlayerAnim_003230,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0025D0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0025D0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BD0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003230,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003230,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_START_FIGHTING */
    &gPlayerAnim_002BB0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0031F8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0031F8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BB0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002BB0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002BB0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_8 */
    &gPlayerAnim_003088,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002A70,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002A70,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003088,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003088,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003088,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_LEFT */
    &gPlayerAnim_002750,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002748,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002748,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002750,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002750,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002750,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_LEFT */
    &gPlayerAnim_002330,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002330,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002330,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002330,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002330,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002330,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_RIGHT */
    &gPlayerAnim_002760,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002758,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002758,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002760,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002760,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002760,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_RIGHT */
    &gPlayerAnim_002338,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002338,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002338,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002338,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002338,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002338,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_HOLDING_OBJECT */
    &gPlayerAnim_002E08,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002E00,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002E00,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002E08,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002E08,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002E08,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_TALL_JUMP_LANDING */
    &gPlayerAnim_003028,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003020,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003020,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003028,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003028,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003028,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SHORT_JUMP_LANDING */
    &gPlayerAnim_003170,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003168,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003168,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003170,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003170,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003170,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_ROLLING */
    &gPlayerAnim_003038,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003030,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003030,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002A68,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003038,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003038,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_17 */
    &gPlayerAnim_002FC0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002FB8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002FB8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002FC8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002FC0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002FC0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_WALK_ON_LEFT_FOOT */
    &gPlayerAnim_003278,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003270,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003270,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BE8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003278,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003278,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_WALK_ON_RIGHT_FOOT */
    &gPlayerAnim_003288,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003280,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003280,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BF0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003288,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003288,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_START_DEFENDING */
    &gPlayerAnim_002EB8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002EA0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002EA0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002EB8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0026C8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002EB8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_DEFENDING */
    &gPlayerAnim_002ED8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002ED0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002ED0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002ED8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0026D0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002ED8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_DEFENDING */
    &gPlayerAnim_002EB0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002EA8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002EA8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002EB0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002EB0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002EB0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_SIDEWALKING */
    &gPlayerAnim_003190,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003188,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003188,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002B68,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003190,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003190,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SIDEWALKING_LEFT */
    &gPlayerAnim_003178,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002568,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002568,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002B58,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003178,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003178,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SIDEWALKING_RIGHT */
    &gPlayerAnim_003180,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002570,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002570,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002B60,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003180,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003180,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SHUFFLE_TURN */
    &gPlayerAnim_002D60,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002D58,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002D58,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002D60,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002D60,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002D60,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_FIGHTING_LEFT_OF_ENEMY */
    &gPlayerAnim_002BB8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003218,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003218,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BB8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002BB8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002BB8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_FIGHTING_RIGHT_OF_ENEMY */
    &gPlayerAnim_002BC8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003228,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003228,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002BC8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002BC8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002BC8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_THROWING_OBJECT */
    &gPlayerAnim_0031C8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0031C0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0031C0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_0031C8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0031C8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_0031C8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_PUTTING_DOWN_OBJECT */
    &gPlayerAnim_003118,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003110,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003110,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003118,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003118,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003118,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_BACKWALKING */
    &gPlayerAnim_002DE8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002DE8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002DE8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002DE8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002DE8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002DE8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_START_CHECKING_OR_SPEAKING */
    &gPlayerAnim_002E30,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002E18,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002E18,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002E30,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002E30,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002E30,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_CHECKING_OR_SPEAKING */
    &gPlayerAnim_002E40,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002E38,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002E38,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002E40,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002E40,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002E40,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_CHECKING_OR_SPEAKING */
    &gPlayerAnim_002E28,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002E20,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002E20,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002E28,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002E28,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002E28,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_PULL_OBJECT */
    &gPlayerAnim_0030C8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0030C0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0030C0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_0030C8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0030C8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_0030C8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_PULL_OBJECT */
    &gPlayerAnim_0030D8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0030D0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0030D0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_0030D8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0030D8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_0030D8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_PUSH_OBJECT */
    &gPlayerAnim_0030B8,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0030B0,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0030B0,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_0030B8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0030B8,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_0030B8,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_KNOCKED_FROM_CLIMBING */
    &gPlayerAnim_002F20,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002F18,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002F18,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002F20,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002F20,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002F20,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_HANGING_FROM_LEDGE */
    &gPlayerAnim_002FF0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002FE8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002FE8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002FF0,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002FF0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002FF0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_CLIMBING_IDLE */
    &gPlayerAnim_003010,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003008,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003008,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003010,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003010,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003010,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_CLIMBING */
    &gPlayerAnim_003000,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002FF8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002FF8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003000,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003000,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003000,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SLIDING_DOWN_SLOPE */
    &gPlayerAnim_002EF0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_002EE8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_002EE8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_002EF8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_002EF0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_002EF0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_SLIDING_DOWN_SLOPE */
    &gPlayerAnim_0031E0,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_0031D8,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_0031D8,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_0031E8,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_0031E0,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_0031E0,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_RELAX */
    &gPlayerAnim_003468,    // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_003438,    // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_003438,    // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_003468,    // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_003468,    // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_003468,    // PLAYER_ANIMTYPE_USED_EXPLOSIVE
};

static LinkAnimationHeader* sManualJumpAnims[][3] = {
    { &gPlayerAnim_002A28, &gPlayerAnim_002A38, &gPlayerAnim_002A30 },
    { &gPlayerAnim_002950, &gPlayerAnim_002960, &gPlayerAnim_002958 },
    { &gPlayerAnim_0029D0, &gPlayerAnim_0029E0, &gPlayerAnim_0029D8 },
    { &gPlayerAnim_002988, &gPlayerAnim_002998, &gPlayerAnim_002990 },
};

// First anim when holding one-handed sword, second anim all other times
static LinkAnimationHeader* sIdleAnims[][2] = {
    { &gPlayerAnim_003248, &gPlayerAnim_003200 }, // 
    { &gPlayerAnim_003258, &gPlayerAnim_003210 }, // 
    { &gPlayerAnim_003250, &gPlayerAnim_003208 }, // 
    { &gPlayerAnim_003250, &gPlayerAnim_003208 }, // 
    { &gPlayerAnim_003430, &gPlayerAnim_0033F0 }, // 
    { &gPlayerAnim_003430, &gPlayerAnim_0033F0 }, // 
    { &gPlayerAnim_003430, &gPlayerAnim_0033F0 }, // 
    { &gPlayerAnim_0033F8, &gPlayerAnim_0033D0 }, // 
    { &gPlayerAnim_003400, &gPlayerAnim_0033D8 }, // 
    { &gPlayerAnim_003420, &gPlayerAnim_003420 }, // 
    { &gPlayerAnim_003408, &gPlayerAnim_0033E0 }, // 
    { &gPlayerAnim_003410, &gPlayerAnim_0033E8 }, // 
    { &gPlayerAnim_003418, &gPlayerAnim_003418 }, // 
    { &gPlayerAnim_003428, &gPlayerAnim_003428 }  // 
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
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4019 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x401E },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x402C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4030 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4034 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4038 },
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
    D_80853E28, D_80853E34, D_80853E44, D_80853E4C, NULL,
};

static u8 sIdleAnimOffsetToAnimSfx[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 10, 10, 10, 10, 10, 10, 3, 3, 4, 4, 8, 8, 5, 5, 6, 6, 7, 7, 9, 9, 0,
};

// Used to map item IDs to action params
static s8 sItemActionParams[] = {
    PLAYER_AP_STICK,
    PLAYER_AP_NUT,
    PLAYER_AP_BOMB,
    PLAYER_AP_BOW,
    PLAYER_AP_BOW_FIRE,
    PLAYER_AP_DINS_FIRE,
    PLAYER_AP_SLINGSHOT,
    PLAYER_AP_OCARINA_FAIRY,
    PLAYER_AP_OCARINA_TIME,
    PLAYER_AP_BOMBCHU,
    PLAYER_AP_HOOKSHOT,
    PLAYER_AP_LONGSHOT,
    PLAYER_AP_BOW_ICE,
    PLAYER_AP_FARORES_WIND,
    PLAYER_AP_BOOMERANG,
    PLAYER_AP_LENS,
    PLAYER_AP_BEAN,
    PLAYER_AP_HAMMER,
    PLAYER_AP_BOW_LIGHT,
    PLAYER_AP_NAYRUS_LOVE,
    PLAYER_AP_BOTTLE,
    PLAYER_AP_BOTTLE_POTION_RED,
    PLAYER_AP_BOTTLE_POTION_GREEN,
    PLAYER_AP_BOTTLE_POTION_BLUE,
    PLAYER_AP_BOTTLE_FAIRY,
    PLAYER_AP_BOTTLE_FISH,
    PLAYER_AP_BOTTLE_MILK,
    PLAYER_AP_BOTTLE_LETTER,
    PLAYER_AP_BOTTLE_FIRE,
    PLAYER_AP_BOTTLE_BUG,
    PLAYER_AP_BOTTLE_BIG_POE,
    PLAYER_AP_BOTTLE_MILK_HALF,
    PLAYER_AP_BOTTLE_POE,
    PLAYER_AP_WEIRD_EGG,
    PLAYER_AP_CHICKEN,
    PLAYER_AP_LETTER_ZELDA,
    PLAYER_AP_MASK_KEATON,
    PLAYER_AP_MASK_SKULL,
    PLAYER_AP_MASK_SPOOKY,
    PLAYER_AP_MASK_BUNNY,
    PLAYER_AP_MASK_GORON,
    PLAYER_AP_MASK_ZORA,
    PLAYER_AP_MASK_GERUDO,
    PLAYER_AP_MASK_TRUTH,
    PLAYER_AP_SWORD_MASTER,
    PLAYER_AP_POCKET_EGG,
    PLAYER_AP_POCKET_CUCCO,
    PLAYER_AP_COJIRO,
    PLAYER_AP_ODD_MUSHROOM,
    PLAYER_AP_ODD_POTION,
    PLAYER_AP_SAW,
    PLAYER_AP_SWORD_BROKEN,
    PLAYER_AP_PRESCRIPTION,
    PLAYER_AP_FROG,
    PLAYER_AP_EYEDROPS,
    PLAYER_AP_CLAIM_CHECK,
    PLAYER_AP_BOW_FIRE,
    PLAYER_AP_BOW_ICE,
    PLAYER_AP_BOW_LIGHT,
    PLAYER_AP_SWORD_KOKIRI,
    PLAYER_AP_SWORD_MASTER,
    PLAYER_AP_SWORD_BGS,
};

static s32 (*sUpperBodyItemFuncs[])(Player* this, PlayState* play) = {
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend2, Player_SetupStartZTargetingDefend2, Player_SetupStartZTargetingDefend2, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldFpsItem,
    Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldFpsItem, Player_HoldActor, Player_HoldActor, Player_HoldBoomerang,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
    Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend, Player_SetupStartZTargetingDefend,
};

static void (*sItemChangeFuncs[])(PlayState* play, Player* this) = {
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_SetupDekuStick,
    Player_DoNothing2, Player_SetupBowOrSlingshot, Player_SetupBowOrSlingshot, Player_SetupBowOrSlingshot, Player_SetupBowOrSlingshot, Player_SetupBowOrSlingshot, Player_SetupBowOrSlingshot,
    Player_SetupBowOrSlingshot, Player_SetupBowOrSlingshot, Player_SetupHookshot, Player_SetupHookshot, Player_SetupExplosive, Player_SetupExplosive, Player_SetupBoomerang,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
    Player_DoNothing, Player_DoNothing, Player_DoNothing, Player_DoNothing,
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

static ItemChangeInfo sItemChangeAnims[PLAYER_ITEM_CHANGE_MAX] = {
    { &gPlayerAnim_002F50, 12 }, /* PLAYER_ITEM_CHANGE_DEFAULT */
    { &gPlayerAnim_003080, 6 },  /* PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND */
    { &gPlayerAnim_002C68, 8 },  /* PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND */
    { &gPlayerAnim_003090, 8 },  /* PLAYER_ITEM_CHANGE_SHIELD */
    { &gPlayerAnim_002A20, 8 },  /* PLAYER_ITEM_CHANGE_2HAND_TO_1HAND */
    { &gPlayerAnim_002F30, 10 }, /* PLAYER_ITEM_CHANGE_1HAND */
    { &gPlayerAnim_002C58, 7 },  /* PLAYER_ITEM_CHANGE_2HAND */
    { &gPlayerAnim_002C60, 11 }, /* PLAYER_ITEM_CHANGE_2HAND_TO_2HAND */
    { &gPlayerAnim_002F50, 12 }, /* PLAYER_ITEM_CHANGE_DEFAULT_2 */
    { &gPlayerAnim_003078, 4 },  /* PLAYER_ITEM_CHANGE_1HAND_TO_BOMB */
    { &gPlayerAnim_003058, 4 },  /* PLAYER_ITEM_CHANGE_2HAND_TO_BOMB */
    { &gPlayerAnim_002F38, 4 },  /* PLAYER_ITEM_CHANGE_BOMB */
    { &gPlayerAnim_0024E0, 5 },  /* PLAYER_ITEM_CHANGE_UNK_12 */
    { &gPlayerAnim_002F48, 13 }, /* PLAYER_ITEM_CHANGE_LEFT_HAND */
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

static struct_80854190 D_80854190[PLAYER_MWA_MAX] = {
    /* PLAYER_MWA_FORWARD_SLASH_1H */
    { &gPlayerAnim_002A80, &gPlayerAnim_002A90, &gPlayerAnim_002A88, 1, 4 },
    /* PLAYER_MWA_FORWARD_SLASH_2H */
    { &gPlayerAnim_0028C0, &gPlayerAnim_0028C8, &gPlayerAnim_002498, 1, 4 },
    /* PLAYER_MWA_FORWARD_COMBO_1H */
    { &gPlayerAnim_002A98, &gPlayerAnim_002AA0, &gPlayerAnim_002540, 0, 5 },
    /* PLAYER_MWA_FORWARD_COMBO_2H */
    { &gPlayerAnim_0028D0, &gPlayerAnim_0028D8, &gPlayerAnim_0024A0, 1, 7 },
    /* PLAYER_MWA_LEFT_SLASH_1H */
    { &gPlayerAnim_002968, &gPlayerAnim_002970, &gPlayerAnim_0024C0, 1, 4 },
    /* PLAYER_MWA_LEFT_SLASH_2H */
    { &gPlayerAnim_002880, &gPlayerAnim_002888, &gPlayerAnim_002478, 0, 5 },
    /* PLAYER_MWA_LEFT_COMBO_1H */
    { &gPlayerAnim_002978, &gPlayerAnim_002980, &gPlayerAnim_0024C8, 2, 8 },
    /* PLAYER_MWA_LEFT_COMBO_2H */
    { &gPlayerAnim_002890, &gPlayerAnim_002898, &gPlayerAnim_002480, 3, 8 },
    /* PLAYER_MWA_RIGHT_SLASH_1H */
    { &gPlayerAnim_0029A0, &gPlayerAnim_0029A8, &gPlayerAnim_0024D0, 0, 4 },
    /* PLAYER_MWA_RIGHT_SLASH_2H */
    { &gPlayerAnim_0028A0, &gPlayerAnim_0028A8, &gPlayerAnim_002488, 0, 5 },
    /* PLAYER_MWA_RIGHT_COMBO_1H */
    { &gPlayerAnim_0029B0, &gPlayerAnim_0029B8, &gPlayerAnim_0024D8, 0, 6 },
    /* PLAYER_MWA_RIGHT_COMBO_2H */
    { &gPlayerAnim_0028B0, &gPlayerAnim_0028B8, &gPlayerAnim_002490, 1, 5 },
    /* PLAYER_MWA_STAB_1H */
    { &gPlayerAnim_002AA8, &gPlayerAnim_002AB0, &gPlayerAnim_002548, 0, 3 },
    /* PLAYER_MWA_STAB_2H */
    { &gPlayerAnim_0028E0, &gPlayerAnim_0028E8, &gPlayerAnim_0024A8, 0, 3 },
    /* PLAYER_MWA_STAB_COMBO_1H */
    { &gPlayerAnim_002AB8, &gPlayerAnim_002AC0, &gPlayerAnim_002550, 1, 9 },
    /* PLAYER_MWA_STAB_COMBO_2H */
    { &gPlayerAnim_0028F0, &gPlayerAnim_0028F8, &gPlayerAnim_0024B0, 1, 8 },
    /* PLAYER_MWA_FLIPSLASH_START */
    { &gPlayerAnim_002A60, &gPlayerAnim_002A50, &gPlayerAnim_002A50, 1, 10 },
    /* PLAYER_MWA_JUMPSLASH_START */
    { &gPlayerAnim_002900, &gPlayerAnim_002910, &gPlayerAnim_002910, 1, 11 },
    /* PLAYER_MWA_FLIPSLASH_FINISH */
    { &gPlayerAnim_002A50, &gPlayerAnim_002A58, &gPlayerAnim_002A58, 1, 2 },
    /* PLAYER_MWA_JUMPSLASH_FINISH */
    { &gPlayerAnim_002910, &gPlayerAnim_002908, &gPlayerAnim_002908, 1, 2 },
    /* PLAYER_MWA_BACKSLASH_RIGHT */
    { &gPlayerAnim_002B80, &gPlayerAnim_002B88, &gPlayerAnim_002B88, 1, 5 },
    /* PLAYER_MWA_BACKSLASH_LEFT */
    { &gPlayerAnim_002B70, &gPlayerAnim_002B78, &gPlayerAnim_002B78, 1, 4 },
    /* PLAYER_MWA_HAMMER_FORWARD */
    { &gPlayerAnim_002C40, &gPlayerAnim_002C50, &gPlayerAnim_002C48, 3, 10 },
    /* PLAYER_MWA_HAMMER_SIDE */
    { &gPlayerAnim_002C70, &gPlayerAnim_002C80, &gPlayerAnim_002C78, 2, 11 },
    /* PLAYER_MWA_SPIN_ATTACK_1H */
    { &gPlayerAnim_002B28, &gPlayerAnim_002B30, &gPlayerAnim_002560, 0, 12 },
    /* PLAYER_MWA_SPIN_ATTACK_2H */
    { &gPlayerAnim_002940, &gPlayerAnim_002948, &gPlayerAnim_0024B8, 0, 15 },
    /* PLAYER_MWA_BIG_SPIN_1H */
    { &gPlayerAnim_0029C0, &gPlayerAnim_0029C8, &gPlayerAnim_002560, 0, 16 },
    /* PLAYER_MWA_BIG_SPIN_2H */
    { &gPlayerAnim_0029C0, &gPlayerAnim_0029C8, &gPlayerAnim_0024B8, 0, 16 },
};

static LinkAnimationHeader* sSpinAttackAnims2[] = {
    &gPlayerAnim_002AE8, // One-handed
    &gPlayerAnim_002920, // Two-handed
};

static LinkAnimationHeader* sSpinAttackAnims1[] = {
    &gPlayerAnim_002AE0, // One-handed
    &gPlayerAnim_002920, // Two-handed
};

static LinkAnimationHeader* sSpinAttackChargeAnims[] = {
    &gPlayerAnim_002AF0, // One-handed
    &gPlayerAnim_002928, // Two-handed
};

static LinkAnimationHeader* sCancelSpinAttackChargeAnims[] = {
    &gPlayerAnim_002AF8, // One-handed
    &gPlayerAnim_002930, // Two-handed
};

static LinkAnimationHeader* sSpinAttackChargeWalkAnims[] = {
    &gPlayerAnim_002B00, // One-handed
    &gPlayerAnim_002938, // Two-handed
};

static LinkAnimationHeader* sSpinAttackChargeSidewalkAnims[] = {
    &gPlayerAnim_002AD8, // One-handed
    &gPlayerAnim_002918, // Two-handed
};

static u8 D_80854380[2] = { PLAYER_MWA_SPIN_ATTACK_1H, PLAYER_MWA_SPIN_ATTACK_2H };
static u8 D_80854384[2] = { PLAYER_MWA_BIG_SPIN_1H, PLAYER_MWA_BIG_SPIN_2H };

static u16 sUseItemButtons[] = { BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };

static u8 sMagicSpellCosts[] = { 12, 24, 24, 12, 24, 12 };

static u16 sFpsItemReadySfx[] = { NA_SE_IT_BOW_DRAW, NA_SE_IT_SLING_DRAW, NA_SE_IT_HOOKSHOT_READY };

static u8 sMagicArrowCosts[] = { 4, 4, 8 };

static LinkAnimationHeader* sRightDefendAnims[] = {
    &gPlayerAnim_0025C0,
    &gPlayerAnim_0025C8,
};

static LinkAnimationHeader* sLeftDefendAnims[] = {
    &gPlayerAnim_002580,
    &gPlayerAnim_002588,
};

static LinkAnimationHeader* sLeftStandingDeflectWithShieldAnims[] = {
    &gPlayerAnim_002510,
    &gPlayerAnim_002518,
};

static LinkAnimationHeader* sRightStandingDeflectWithShieldAnims[] = {
    &gPlayerAnim_002510,
    &gPlayerAnim_002520,
};

static LinkAnimationHeader* sDeflectWithShieldAnims[] = {
    &gPlayerAnim_002EC0,
    &gPlayerAnim_002A08,
};

static LinkAnimationHeader* sReadyFpsItemWhileWalkingAnims[] = {
    &gPlayerAnim_0026F0,
    &gPlayerAnim_002CC8,
};

static LinkAnimationHeader* sReadyFpsItemAnims[] = {
    &gPlayerAnim_0026C0,
    &gPlayerAnim_002CC0,
};

// return type can't be void due to regalloc in func_8084FCAC
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
        Player_ChangeItem(play, this, PLAYER_AP_NONE);
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

    this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_IN_FIRST_PERSON_MODE | PLAYER_STATE1_CLIMBING);
    this->stateFlags2 &= ~(PLAYER_STATE2_MOVING_PUSH_PULL_WALL | PLAYER_STATE2_RESTRAINED_BY_ENEMY | PLAYER_STATE2_INSIDE_CRAWLSPACE);

    this->actor.shape.rot.x = 0;
    this->actor.shape.yOffset = 0.0f;

    this->slashCounter = this->comboTimer = 0;
}

s32 Player_UnequipItem(PlayState* play, Player* this) {
    if (this->heldItemActionParam >= PLAYER_AP_FISHING_POLE) {
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

// Requests for the rumble pak to start rumbling
void Player_RequestRumble(Player* this, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        func_800AA000(arg4, arg1, arg2, arg3);
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
    return sfxId + this->moveSfxType;
}

void Player_PlayMoveSfx(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, Player_GetMoveSfx(this, sfxId));
}

u16 Player_GetMoveSfxForAge(Player* this, u16 sfxId) {
    return sfxId + this->moveSfxType + this->ageProperties->ageMoveSfxOffset;
}

void Player_PlayMoveSfxForAge(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, Player_GetMoveSfxForAge(this, sfxId));
}

void Player_PlayWalkSfx(Player* this, f32 arg1) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_WALK_HEAVYBOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_WALK_GROUND);
    }

    func_800F4010(&this->actor.projectedPos, sfxId, arg1);
}

void Player_PlayJumpSfx(Player* this) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_JUMP_HEAVYBOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_JUMP);
    }

    func_8002F7DC(&this->actor, sfxId);
}

void Player_PlayLandingSfx(Player* this) {
    s32 sfxId;

    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_LAND_HEAVYBOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_LAND);
    }

    func_8002F7DC(&this->actor, sfxId);
}

void Player_PlayReactableSfx(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, sfxId);
    this->stateFlags2 |= PLAYER_STATE2_MAKING_REACTABLE_NOISE;
}

#define PLAYER_ANIMSFX_GET_FLAGS(data)         ((data) & 0x7800)
#define PLAYER_ANIMSFX_GET_PLAY_FRAME(data)    ((data) & 0x7FF)

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
                func_800F4010(&this->actor.projectedPos, this->ageProperties->ageMoveSfxOffset + NA_SE_PL_WALK_LADDER, 0.0f);
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

void Player_PlayAnimOnceWithMovementSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags, f32 playbackSpeed) {
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
        scaledCamOffsetStickAngle = (u16)((s16)(sCameraOffsetAnalogStickAngle - this->actor.shape.rot.y) + DEG_TO_BINANG(45.0f)) >> 14;
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
    return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_STANDING_STILL, this->modelAnimType);
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
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_RUNNING_DAMAGED, this->modelAnimType);
    } else if (!(this->stateFlags1 & (PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
               (this->currentBoots == PLAYER_BOOTS_IRON)) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_IRON_BOOTS, this->modelAnimType);
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_RUNNING, this->modelAnimType);
    }
}

s32 Player_IsAimingReadyBoomerang(Player* this) {
    return Player_IsAimingBoomerang(this) && (this->fpsItemTimer != 0);
}

LinkAnimationHeader* Player_GetFightingRightAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_002638;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_FIGHTING_RIGHT_OF_ENEMY, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetFightingLeftAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_002630;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_FIGHTING_LEFT_OF_ENEMY, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetEndSidewalkAnim(Player* this) {
    if (Actor_PlayerIsAimingReadyFpsItem(this)) {
        return &gPlayerAnim_0026E8;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_SIDEWALKING, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetSidewalkRightAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_002620;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SIDEWALKING_RIGHT, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetSidewalkLeftAnim(Player* this) {
    if (Player_IsAimingReadyBoomerang(this)) {
        return &gPlayerAnim_002618;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SIDEWALKING_LEFT, this->modelAnimType);
    }
}

void Player_SetUpperActionFunc(Player* this, PlayerUpperActionFunc arg1) {
    this->upperActionFunc = arg1;
    this->fpsItemShootState = 0;
    this->upperInterpWeight = 0.0f;
    Player_StopInterruptableSfx(this);
}

void Player_SetupChangeItemAnim(PlayState* play, Player* this, s8 actionParam) {
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

    Player_ChangeItem(play, this, actionParam);

    if (i < PLAYER_ANIMGROUP_MAX) {
        this->skelAnime.animation = GET_PLAYER_ANIM(i, this->modelAnimType);
    }
}

s8 Player_ItemToActionParam(s32 item) {
    if (item >= ITEM_NONE_FE) {
        return PLAYER_AP_NONE;
    } else if (item == ITEM_LAST_USED) {
        return PLAYER_AP_LAST_USED;
    } else if (item == ITEM_FISHING_POLE) {
        return PLAYER_AP_FISHING_POLE;
    } else {
        return sItemActionParams[item];
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

    if (this->heldItemActionParam != PLAYER_AP_SLINGSHOT) {
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

void Player_ChangeItem(PlayState* play, Player* this, s8 actionParam) {
    this->fpsItemType = PLAYER_FPSITEM_NONE;
    this->unk_85C = 0.0f;
    this->spinAttackTimer = 0.0f;

    this->heldItemActionParam = this->itemActionParam = actionParam;
    this->modelGroup = this->nextModelGroup;

    this->stateFlags1 &= ~(PLAYER_STATE1_AIMING_FPS_ITEM | PLAYER_STATE1_AIMING_BOOMERANG);

    sItemChangeFuncs[actionParam](play, this);

    Player_SetModelGroup(this, this->modelGroup);
}

void Player_MeleeAttack(Player* this, s32 attackFlag) {
    u16 itemSfx;
    u16 voiceSfx;

    if (this->isMeleeWeaponAttacking == false) {
        if ((this->heldItemActionParam == PLAYER_AP_SWORD_BGS) && (gSaveContext.swordHealth > 0.0f)) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else { 
            itemSfx = NA_SE_IT_SWORD_SWING;
        }

        voiceSfx = NA_SE_VO_LI_SWORD_N;
        if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else if (this->meleeWeaponAnimation >= PLAYER_MWA_SPIN_ATTACK_1H) {
            itemSfx = 0;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        } else if (this->slashCounter >= 3) {
            itemSfx = NA_SE_IT_SWORD_SWING_HARD;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        }

        if (itemSfx != 0) {
            Player_PlayReactableSfx(this, itemSfx);
        }

        if (!((this->meleeWeaponAnimation >= PLAYER_MWA_FLIPSLASH_START) &&
              (this->meleeWeaponAnimation <= PLAYER_MWA_JUMPSLASH_FINISH))) {
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
    if ((item < ITEM_NONE_FE) && (Player_ItemToActionParam(item) == this->itemActionParam)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_IsWearableMaskValid(s32 item1, s32 actionParam) {
    if ((item1 < ITEM_NONE_FE) && (Player_ItemToActionParam(item1) == actionParam)) {
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
    s32 maskActionParam;
    s32 item;
    s32 i;

    if (this->currentMask != PLAYER_MASK_NONE) {
        maskActionParam = this->currentMask - 1 + PLAYER_AP_MASK_KEATON;
        if (!Player_IsWearableMaskValid(C_BTN_ITEM(0), maskActionParam) && !Player_IsWearableMaskValid(C_BTN_ITEM(1), maskActionParam) &&
            !Player_IsWearableMaskValid(C_BTN_ITEM(2), maskActionParam)) {
            this->currentMask = PLAYER_MASK_NONE;
        }
    }

    if (!(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE)) && !Player_IsShootingHookshot(this)) {
        if (this->itemActionParam >= PLAYER_AP_FISHING_POLE) {
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
            if ((item < ITEM_NONE_FE) && (Player_ItemToActionParam(item) == this->heldItemActionParam)) {
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
    s8 actionParam;
    s32 nextAnimType;

    actionParam = Player_ItemToActionParam(this->heldItemId);
    Player_SetUpperActionFunc(this, Player_StartChangeItem);

    nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];
    itemChangeAnim = sAnimtypeToItemChangeAnims[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType];
    if ((actionParam == PLAYER_AP_BOTTLE) || (actionParam == PLAYER_AP_BOOMERANG) ||
        ((actionParam == PLAYER_AP_NONE) &&
         ((this->heldItemActionParam == PLAYER_AP_BOTTLE) || (this->heldItemActionParam == PLAYER_AP_BOOMERANG)))) {
        itemChangeAnim = (actionParam == PLAYER_AP_NONE) ? -PLAYER_ITEM_CHANGE_LEFT_HAND : PLAYER_ITEM_CHANGE_LEFT_HAND;
    }

    this->itemChangeAnim = ABS(itemChangeAnim);

    anim = sItemChangeAnims[this->itemChangeAnim].anim;
    if ((anim == &gPlayerAnim_002F30) && (this->currentShield == PLAYER_SHIELD_NONE)) {
        anim = &gPlayerAnim_002F40;
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

    if (actionParam != PLAYER_AP_NONE) {
        playSpeed *= 2.0f;
    }

    LinkAnimation_Change(play, &this->skelAnimeUpper, anim, playSpeed, startFrame, endFrame, ANIMMODE_ONCE, 0.0f);

    this->stateFlags1 &= ~PLAYER_STATE1_START_CHANGE_ITEM;
}

void Player_SetupItem(Player* this, PlayState* play) {
    if ((this->actor.category == ACTORCAT_PLAYER) && !(this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) &&
        ((this->heldItemActionParam == this->itemActionParam) || (this->stateFlags1 & PLAYER_STATE1_SHIELDING)) &&
        (gSaveContext.health != 0) && (play->csCtx.state == CS_STATE_IDLE) && (this->csMode == PLAYER_CSMODE_NONE) &&
        (play->shootingGalleryStatus == 0) && (play->activeCamId == CAM_ID_MAIN) &&
        (play->transitionTrigger != TRANS_TRIGGER_START) && (gSaveContext.timer1State != 10)) {
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
            *typePtr = ARROW_NORMAL + (this->heldItemActionParam - PLAYER_AP_BOW);
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

    if ((this->heldItemActionParam >= PLAYER_AP_BOW_FIRE) && (this->heldItemActionParam <= PLAYER_AP_BOW_0E) &&
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
    if (this->heldItemActionParam != PLAYER_AP_NONE) {
        if (Player_GetSwordItemAP(this, this->heldItemActionParam) >= 0) {
            Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PUTAWAY);
        } else {
            Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
        }
    }

    Player_UseItem(play, this, this->heldItemId);

    if (Player_GetSwordItemAP(this, this->heldItemActionParam) >= 0) {
        Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PICKOUT);
    } else if (this->heldItemActionParam != PLAYER_AP_NONE) {
        Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
    }
}

void Player_SetupHeldItemUpperActionFunc(PlayState* play, Player* this) {
    if (Player_StartChangeItem == this->upperActionFunc) {
        Player_ChangeItemWithSfx(play, this);
    }

    Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemActionParam]);
    this->fpsItemTimer = 0;
    this->idleCounter = 0;
    Player_DetatchHeldActor(play, this);
    this->stateFlags1 &= ~PLAYER_STATE1_START_CHANGE_ITEM;
}

LinkAnimationHeader* Player_GetDefendAnim(PlayState* play, Player* this) {
    Player_SetUpperActionFunc(this, func_80834B5C);
    Player_DetatchHeldActor(play, this);

    if (this->leftRightBlendWeight < 0.5f) {
        return sRightDefendAnims[Player_HoldsTwoHandedWeapon(this)];
    } else {
        return sLeftDefendAnims[Player_HoldsTwoHandedWeapon(this)];
    }
}

s32 Player_StartZTargetingDefend(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    f32 frame;

    if (!(this->stateFlags1 & (PLAYER_STATE1_SHIELDING | PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_IN_CUTSCENE)) &&
        (play->shootingGalleryStatus == 0) && (this->heldItemActionParam == this->itemActionParam) &&
        (this->currentShield != PLAYER_SHIELD_NONE) && !Player_IsChildWithHylianShield(this) && Player_IsZTargeting(this) &&
        CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {

        anim = Player_GetDefendAnim(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->skelAnimeUpper, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
        func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_POSTURE);

        return 1;
    } else {
        return 0;
    }
}

s32 Player_SetupStartZTargetingDefend(Player* this, PlayState* play) {
    if (Player_StartZTargetingDefend(play, this)) {
        return 1;
    } else {
        return 0;
    }
}

void Player_SetupEndDefend(Player* this) {
    Player_SetUpperActionFunc(this, Player_EndDefend);

    if (this->itemActionParam < 0) {
        Player_SetHeldItem(this);
    }

    Animation_Reverse(&this->skelAnimeUpper);
    func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_REMOVE);
}

void Player_SetupChangeItem(PlayState* play, Player* this) {
    ItemChangeInfo* itemChangeInfo = &sItemChangeAnims[this->itemChangeAnim];
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

s32 Player_SetupStartZTargetingDefend2(Player* this, PlayState* play) {
    if (Player_StartZTargetingDefend(play, this) || func_8083499C(this, play)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_StartChangeItem(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnimeUpper) ||
        ((Player_ItemToActionParam(this->heldItemId) == this->heldItemActionParam) &&
         (sUsingItemAlreadyInHand =
              (sUsingItemAlreadyInHand || ((this->modelAnimType != PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON) && (play->shootingGalleryStatus == 0)))))) {
        Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemActionParam]);
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

s32 func_80834B5C(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnimeUpper);

    if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
        Player_SetupEndDefend(this);
        return 1;
    } else {
        this->stateFlags1 |= PLAYER_STATE1_SHIELDING;
        Player_SetModelsForHoldingShield(this);
        return 1;
    }
}

s32 func_80834BD4(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;

    if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        anim = Player_GetDefendAnim(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->skelAnimeUpper, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
    }

    this->stateFlags1 |= PLAYER_STATE1_SHIELDING;
    Player_SetModelsForHoldingShield(this);

    return 1;
}

s32 Player_EndDefend(Player* this, PlayState* play) {
    sUsingItemAlreadyInHand = sUsingItemAlreadyInHand2;

    if (sUsingItemAlreadyInHand || LinkAnimation_Update(play, &this->skelAnimeUpper)) {
        Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemActionParam]);
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_STANDING_STILL, this->modelAnimType));
        this->idleCounter = 0;
        this->upperActionFunc(this, play);
        return 0;
    }

    return 1;
}

s32 Player_SetupUseFpsItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    if (this->heldItemActionParam != PLAYER_AP_BOOMERANG) {
        if (!Player_SetupReadyFpsItemToShoot(this, play)) {
            return 0;
        }

        if (!Player_HoldsHookshot(this)) {
            anim = &gPlayerAnim_0026A0;
        } else {
            anim = &gPlayerAnim_002CA0;
        }
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, anim);
    } else {
        Player_SetUpperActionFunc(this, Player_SetupAimBoomerang);
        this->fpsItemTimer = 10;
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_002628);
    }

    if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_003380);
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !Player_SetupStartUnfriendlyZTargeting(this)) {
        Player_PlayAnimLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_STANDING_STILL, this->modelAnimType));
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
        if (Player_IsZTargeting(this) || (Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_BOWARROW) == 0)) {
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

    if ((!Player_HoldsHookshot(this) || Player_EndHookshotMove(this)) && !Player_StartZTargetingDefend(play, this) &&
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

        this->unk_A73 = 4;
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

    if ((this->fpsItemShootState == 0) && (Player_IsPlayingIdleAnim(this) == 0) && (this->skelAnime.animation == &gPlayerAnim_0026E8)) {
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

    if ((this->fpsItemShootState > 0) && ((this->fpsItemType < PLAYER_FPSITEM_NONE) || (!sUsingItemAlreadyInHand2 && !func_80834E7C(play)))) {
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

    if (!Player_StartZTargetingDefend(play, this) && (sUsingItemAlreadyInHand || ((this->fpsItemType < PLAYER_FPSITEM_NONE) && sUsingItemAlreadyInHand2) || Player_CheckShootingGalleryShootInput(play))) {
        this->fpsItemType = ABS(this->fpsItemType);

        if (Player_SetupReadyFpsItemToShoot(this, play)) {
            if (Player_HoldsHookshot(this)) {
                this->fpsItemShootState = 1;
            } else {
                LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_0026B8);
            }
        }
    } else {
        if (this->fpsItemTimer != 0) {
            this->fpsItemTimer--;
        }

        if (Player_IsZTargeting(this) || (this->attentionMode != PLAYER_ATTENTIONMODE_NONE) || (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
            if (this->fpsItemTimer == 0) {
                this->fpsItemTimer++;
            }
            return 1;
        }

        if (Player_HoldsHookshot(this)) {
            Player_SetUpperActionFunc(this, Player_HoldFpsItem);
        } else {
            Player_SetUpperActionFunc(this, Player_EndAimFpsItem);
            LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_0026B0);
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

void func_808355DC(Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_Z_TARGETING_FRIENDLY;

    if (!(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_7) && (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        (sYawToTouchedWall < DEG_TO_BINANG(45.0f))) {
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
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, &gPlayerAnim_002E10);
    }
}

s32 Player_HoldActor(Player* this, PlayState* play) {
    Actor* heldActor = this->heldActor;

    if (heldActor == NULL) {
        Player_SetupHeldItemUpperActionFunc(play, this);
    }

    if (Player_StartZTargetingDefend(play, this)) {
        return 1;
    }

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
            LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, &gPlayerAnim_002E10);
        }

        if ((heldActor->id == ACTOR_EN_NIW) && (this->actor.velocity.y <= 0.0f)) {
            this->actor.minVelocityY = -2.0f;
            this->actor.gravity = -0.5f;
            this->fallStartHeight = this->actor.world.pos.y;
        }

        return 1;
    }

    return Player_SetupStartZTargetingDefend(this, play);
}

void Player_SetLeftHandDlists(Player* this, Gfx** dLists) {
    this->leftHandDLists = dLists + gSaveContext.linkAge;
}

s32 Player_HoldBoomerang(Player* this, PlayState* play) {
    if (Player_StartZTargetingDefend(play, this)) {
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
        LinkAnimation_PlayLoop(play, &this->skelAnimeUpper, &gPlayerAnim_002638);
    }

    Player_SetupAimAttention(this, play);

    return 1;
}

s32 Player_AimBoomerang(Player* this, PlayState* play) {
    LinkAnimationHeader* animSeg = this->skelAnime.animation;

    if ((Player_GetFightingRightAnim(this) == animSeg) || (Player_GetFightingLeftAnim(this) == animSeg) || (Player_GetSidewalkRightAnim(this) == animSeg) ||
        (Player_GetSidewalkLeftAnim(this) == animSeg)) {
        AnimationContext_SetCopyAll(play, this->skelAnime.limbCount, this->skelAnimeUpper.jointTable,
                                    this->skelAnime.jointTable);
    } else {
        LinkAnimation_Update(play, &this->skelAnimeUpper);
    }

    Player_SetupAimAttention(this, play);

    if (!sUsingItemAlreadyInHand2) {
        Player_SetUpperActionFunc(this, Player_ThrowBoomerang);
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper,
                               (this->leftRightBlendWeight < 0.5f) ? &gPlayerAnim_002608 : &gPlayerAnim_002600);
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
                func_808355DC(this);
            }
            this->unk_A73 = 4;
            func_8002F7DC(&this->actor, NA_SE_IT_BOOMERANG_THROW);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        }
    }

    return 1;
}

s32 Player_WaitForThrownBoomerang(Player* this, PlayState* play) {
    if (Player_StartZTargetingDefend(play, this)) {
        return 1;
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG)) {
        Player_SetUpperActionFunc(this, Player_CatchBoomerang);
        LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, &gPlayerAnim_0025F8);
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

    if (func_8084E3C4 == this->actionFunc) {
        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
        this->stateFlags2 &= ~(PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
    } else if (Player_UpdateMagicSpell == this->actionFunc) {
        Player_ResetSubCam(play, this);
    }

    this->actionFunc = func;

    if ((this->itemActionParam != this->heldItemActionParam) &&
        (!(flag & 1) || !(this->stateFlags1 & PLAYER_STATE1_SHIELDING))) {
        Player_SetHeldItem(this);
    }

    if (!(flag & 1) && !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
        Player_SetupHeldItemUpperActionFunc(play, this);
        this->stateFlags1 &= ~PLAYER_STATE1_SHIELDING;
    }

    Player_EndAnimMovement(this);
    this->stateFlags1 &= ~(PLAYER_STATE1_END_HOOKSHOT_MOVE | PLAYER_STATE1_TALKING | PLAYER_STATE1_TAKING_DAMAGE | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE |
                           PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID);
    this->stateFlags2 &= ~(PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING | PLAYER_STATE2_PLAYING_OCARINA_GENERAL | PLAYER_STATE2_IDLING);
    this->stateFlags3 &= ~(PLAYER_STATE3_MIDAIR | PLAYER_STATE3_ENDING_MELEE_ATTACK | PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH);
    this->unk_84F = 0;
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

    if (this->itemActionParam >= 0) {
        itemAPToRestore = this->itemActionParam;
        this->itemActionParam = this->heldItemActionParam;
        Player_SetActionFunc(play, this, func, flag);
        this->itemActionParam = itemAPToRestore;
        Player_SetModels(this, Player_ActionToModelGroup(this, this->itemActionParam));
    }
}

void Player_ChangeCameraSetting(PlayState* play, s16 camSetting) {
    if (!Play_CamIsNotFixed(play)) {
        if (camSetting == CAM_SET_SCENE_TRANSITION) {
            Interface_ChangeAlpha(2);
        }
    } else {
        Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), camSetting);
    }
}

void Player_SetCameraTurnAround(PlayState* play, s32 arg1) {
    Player_ChangeCameraSetting(play, CAM_SET_TURN_AROUND);
    Camera_SetCameraData(Play_GetCamera(play, CAM_ID_MAIN), 4, 0, 0, arg1, 0, 0);
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
    s8 actionParam;
    s32 temp;
    s32 nextAnimType;

    actionParam = Player_ItemToActionParam(item);

    if (((this->heldItemActionParam == this->itemActionParam) &&
         (!(this->stateFlags1 & PLAYER_STATE1_SHIELDING) || (Player_ActionToMeleeWeapon(actionParam) != PLAYER_MELEEWEAPON_NONE) ||
          (actionParam == PLAYER_AP_NONE))) ||
        ((this->itemActionParam < 0) &&
         ((Player_ActionToMeleeWeapon(actionParam) != PLAYER_MELEEWEAPON_NONE) || (actionParam == PLAYER_AP_NONE)))) {

        if ((actionParam == PLAYER_AP_NONE) || !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
            ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
             ((actionParam == PLAYER_AP_HOOKSHOT) || (actionParam == PLAYER_AP_LONGSHOT)))) {

            if ((play->bombchuBowlingStatus == 0) &&
                (((actionParam == PLAYER_AP_STICK) && (AMMO(ITEM_STICK) == 0)) ||
                 ((actionParam == PLAYER_AP_BEAN) && (AMMO(ITEM_BEAN) == 0)) ||
                 (temp = Player_ActionToExplosive(this, actionParam),
                  ((temp >= 0) && ((AMMO(sExplosiveInfos[temp].itemId) == 0) ||
                                   (play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].length >= 3)))))) {
                func_80078884(NA_SE_SY_ERROR);
            } else if (actionParam == PLAYER_AP_LENS) {
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
            } else if (actionParam == PLAYER_AP_NUT) {
                if (AMMO(ITEM_NUT) != 0) {
                    Player_SetupThrowDekuNut(play, this);
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if ((temp = Player_ActionToMagicSpell(this, actionParam)) >= 0) {
                if (((actionParam == PLAYER_AP_FARORES_WIND) && (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 0)) ||
                    ((gSaveContext.magicCapacity != 0) && (gSaveContext.magicState == MAGIC_STATE_IDLE) &&
                     (gSaveContext.magic >= sMagicSpellCosts[temp]))) {
                    this->itemActionParam = actionParam;
                    this->attentionMode = PLAYER_ATTENTIONMODE_ITEM_CUTSCENE;
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if (actionParam >= PLAYER_AP_MASK_KEATON) {
                if (this->currentMask != PLAYER_MASK_NONE) {
                    this->currentMask = PLAYER_MASK_NONE;
                } else {
                    this->currentMask = actionParam - PLAYER_AP_MASK_KEATON + 1;
                }

                Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
            } else if (((actionParam >= PLAYER_AP_OCARINA_FAIRY) && (actionParam <= PLAYER_AP_OCARINA_TIME)) ||
                       (actionParam >= PLAYER_AP_BOTTLE_FISH)) {
                if (!Player_IsUnfriendlyZTargeting(this) ||
                    ((actionParam >= PLAYER_AP_BOTTLE_POTION_RED) && (actionParam <= PLAYER_AP_BOTTLE_FAIRY))) {
                    TitleCard_Clear(play, &play->actorCtx.titleCtx);
                    this->attentionMode = PLAYER_ATTENTIONMODE_ITEM_CUTSCENE;
                    this->itemActionParam = actionParam;
                }
            } else if ((actionParam != this->heldItemActionParam) ||
                       ((this->heldActor == 0) && (Player_ActionToExplosive(this, actionParam) >= 0))) {
                this->nextModelGroup = Player_ActionToModelGroup(this, actionParam);
                nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];

                if ((this->heldItemActionParam >= 0) && (Player_ActionToMagicSpell(this, actionParam) < 0) &&
                    (item != this->heldItemId) &&
                    (sAnimtypeToItemChangeAnims[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType] !=
                     PLAYER_ITEM_CHANGE_DEFAULT)) {
                    this->heldItemId = item;
                    this->stateFlags1 |= PLAYER_STATE1_START_CHANGE_ITEM;
                } else {
                    Player_PutAwayHookshot(this);
                    Player_DetatchHeldActor(play, this);
                    Player_SetupChangeItemAnim(play, this, actionParam);
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

    Player_SetActionFunc(play, this, isSwimming ? Player_SetupDrown : Player_Die, 0);

    this->stateFlags1 |= PLAYER_STATE1_IN_DEATH_CUTSCENE;

    Player_PlayAnimOnce(play, this, anim);
    if (anim == &gPlayerAnim_002878) {
        this->skelAnime.endFrame = 84.0f;
    }

    Player_ClearAttentionModeAndStopMoving(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DOWN);

    if (this->actor.category == ACTORCAT_PLAYER) {
        func_800F47BC();

        if (Inventory_ConsumeFairy(play)) {
            play->gameOverCtx.state = GAMEOVER_REVIVE_START;
            this->unk_84F = 1;
        } else {
            play->gameOverCtx.state = GAMEOVER_DEATH_START;
            func_800F6AB0(0);
            Audio_PlayFanfare(NA_BGM_GAME_OVER);
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
        }

        OnePointCutscene_Init(play, 9806, isSwimming ? 120 : 60, &this->actor, CAM_ID_MAIN);
        ShrinkWindow_SetVal(0x20);
    }
}

s32 Player_CanUseItem(Player* this) {
    return (!(Player_RunMiniCutsceneFunc == this->actionFunc) ||
            ((this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) &&
             ((this->heldItemId == ITEM_LAST_USED) || (this->heldItemId == ITEM_NONE)))) &&
           (!(Player_StartChangeItem == this->upperActionFunc) ||
            (Player_ItemToActionParam(this->heldItemId) == this->heldItemActionParam));
}

s32 Player_SetupCurrentUpperAction(Player* this, PlayState* play) {
    if (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->actor.parent != NULL) && Player_HoldsHookshot(this)) {
        Player_SetActionFunc(play, this, Player_MoveAlongHookshotPath, 1);
        this->stateFlags3 |= PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH;
        Player_PlayAnimOnce(play, this, &gPlayerAnim_002C90);
        Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
        Player_ClearAttentionModeAndStopMoving(this);
        this->currentYaw = this->actor.shape.rot.y;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        this->hoverBootsTimer = 0;
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
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

    if (!(this->stateFlags2 & (PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION))) {
        if ((this->targetActor != NULL) &&
            ((play->actorCtx.targetCtx.unk_4B != 0) || (this->actor.category != ACTORCAT_PLAYER))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y,
                               Math_Vec3f_Yaw(&this->actor.world.pos, &this->targetActor->focus.pos), 4000);
        } else if ((this->stateFlags1 & PLAYER_STATE1_Z_TARGETING_FRIENDLY) &&
                   !(this->stateFlags2 & (PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, this->targetYaw, 4000);
        }
    } else if (!(this->stateFlags2 & PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION)) {
        Math_ScaledStepToS(&this->actor.shape.rot.y, this->currentYaw, 2000);
    }

    this->unk_87C = this->actor.shape.rot.y - previousYaw;
}

// Steps angle based on offset from referenceAngle, then returns any excess angle difference beyond angleMinMax
s32 Player_StepAngleWithOffset(s16* angle, s16 target, s16 step, s16 angleMinMax, s16 referenceAngle, s16 angleDiffMinMax) {
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
    s16 yaw;

    yaw = this->actor.shape.rot.y;
    if (syncUpperRotToFocusRot != false) {
        yaw = this->actor.focus.rot.y;
        this->upperBodyRot.x = this->actor.focus.rot.x;
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
    } else {
        Player_StepAngleWithOffset(&this->upperBodyRot.x,
                      Player_StepAngleWithOffset(&this->headRot.x, this->actor.focus.rot.x, DEG_TO_BINANG(3.3f), DEG_TO_BINANG(54.932f), this->actor.focus.rot.x, 0),
                      DEG_TO_BINANG(1.1f), DEG_TO_BINANG(21.973f), this->headRot.x, DEG_TO_BINANG(54.932f));
        yawDiff = this->actor.focus.rot.y - yaw;
        Player_StepAngleWithOffset(&yawDiff, 0, DEG_TO_BINANG(1.1f), DEG_TO_BINANG(131.84f), this->upperBodyRot.y, DEG_TO_BINANG(43.95f));
        yaw = this->actor.focus.rot.y - yawDiff;
        Player_StepAngleWithOffset(&this->headRot.y, yawDiff - this->upperBodyRot.y, DEG_TO_BINANG(1.1f), DEG_TO_BINANG(43.95f), yawDiff, DEG_TO_BINANG(43.95f));
        Player_StepAngleWithOffset(&this->upperBodyRot.y, yawDiff, DEG_TO_BINANG(1.1f), DEG_TO_BINANG(43.95f), this->headRot.y, DEG_TO_BINANG(43.95f));
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X |
                           PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_X |
                           PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
    }

    return yaw;
}

void func_80836BEC(Player* this, PlayState* play) {
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
        (this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_IN_CUTSCENE)) || (this->stateFlags3 & PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH)) {
        this->targetSwitchTimer = 0;
    } else if (zTrigPressed || (this->stateFlags2 & PLAYER_STATE2_USING_SWITCH_Z_TARGETING) || (this->forcedTargetActor != NULL)) {
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
    if (actorRequestingTalk || (this->targetSwitchTimer != 0) || (this->stateFlags1 & (PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_AWAITING_THROWN_BOOMERANG))) {
        if (!actorRequestingTalk) {
            if (!(this->stateFlags1 & PLAYER_STATE1_AWAITING_THROWN_BOOMERANG) &&
                ((this->heldItemActionParam != PLAYER_AP_FISHING_POLE) || (this->fpsItemType == PLAYER_FPSITEM_NONE)) &&
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
                        func_808355DC(this);
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

// If not in scene transition, or if lacking attention mode, checks for analog stick distance. If none, returns false with generic velocity and yaw set to analog stick angle.
s32 Player_SetVelocityAndYaw(PlayState* play, Player* this, f32* velocity, s16* yaw, f32 arg4) {
    f32 baseSpeedScale;
    f32 slope;
    f32 slopeSpeedScale;
    f32 speedLimit;

    if ((this->attentionMode != PLAYER_ATTENTIONMODE_NONE) || (play->transitionTrigger == TRANS_TRIGGER_START) ||
        (this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE)) {
        *velocity = 0.0f;
        *yaw = this->actor.shape.rot.y;
    } else {
        *velocity = sAnalogStickDistance;
        *yaw = sAnalogStickAngle;

        if (arg4 != 0.0f) {
            *velocity -= 20.0f;
            if (*velocity < 0.0f) {
                *velocity = 0.0f;
            } else {
                baseSpeedScale = 1.0f - Math_CosS(*velocity * 450.0f);
                *velocity = ((baseSpeedScale * baseSpeedScale) * 30.0f) + 7.0f;
            }
        } else {
            *velocity *= 0.8f;
        }

        if (sAnalogStickDistance != 0.0f) {
            slope = Math_SinS(this->angleToFloorX);
            speedLimit = this->speedLimit;
            slopeSpeedScale = CLAMP(slope, 0.0f, 0.6f);

            if (this->sinkingOffsetY != 0.0f) {
                speedLimit = speedLimit - (this->sinkingOffsetY * 0.008f);
                if (speedLimit < 2.0f) {
                    speedLimit = 2.0f;
                }
            }

            *velocity = (*velocity * 0.14f) - (8.0f * slopeSpeedScale * slopeSpeedScale);
            *velocity = CLAMP(*velocity, 0.0f, speedLimit);

            return 1;
        }
    }

    return 0;
}

s32 Player_StepLinearVelocityToZero(Player* this) {
    return Math_StepToF(&this->linearVelocity, 0.0f, REG(43) / 100.0f);
}

// If Player_SetVelocityAndYaw returns false, return a target velocity and yaw. If it returns true, add cam input dir yaw to target yaw.
s32 Player_SetOrGetVelocityAndYaw(Player* this, f32* velocity, s16* yaw, f32 arg3, PlayState* play) {
    if (!Player_SetVelocityAndYaw(play, this, velocity, yaw, arg3)) {
        *yaw = this->actor.shape.rot.y;

        if (this->targetActor != NULL) {
            if ((play->actorCtx.targetCtx.unk_4B != 0) && !(this->stateFlags2 & PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION)) {
                *yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->targetActor->focus.pos);
                return 0;
            }
        } else if (Player_IsFriendlyZTargeting(this)) {
            *yaw = this->targetYaw;
        }

        return 0;
    } else {
        *yaw += Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
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
    Player_SetupCUpBehavior,           // 0
    Player_SetupOpenDoor,              // 1
    Player_SetupGetItemOrHoldBehavior, // 2
    Player_SetupMountHorse,            // 3
    Player_SetupSpeakOrCheck,          // 4
    Player_SetupStartGrabPushPullWall, // 5
    Player_SetupRollOrPutAway,         // 6
    Player_SetupMeleeWeaponAttack,     // 7
    Player_SetupStartChargeSpinAttack, // 8
    Player_SetupPutDownOrThrowActor,   // 9
    Player_SetupJumpSlashOrRoll,       // 10
    Player_SetupDefend,                // 11
    Player_SetupWallJumpBehavior,      // 12
    Player_SetupItemCutsceneOrCUp,     // 13
};

// Checks if you're allowed to perform a sub-action based on a provided array of indices into sSubActions
s32 Player_SetupSubAction(PlayState* play, Player* this, s8* subActionIndex, s32 flag) {
    s32 i;

    if (!(this->stateFlags1 & (PLAYER_STATE1_EXITING_SCENE | PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_IN_CUTSCENE))) {
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

        if (!(this->stateFlags1 & PLAYER_STATE1_START_CHANGE_ITEM) && (Player_StartChangeItem != this->upperActionFunc)) {
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

// Checks if current anim is certain number of frames from the end, returns -1 if false
// If true, then returns 0 if sub-action can be performed, returns 1 if velocity is slowed down and target yaw is set
s32 Player_SetupInterruptAction(PlayState* play, Player* this, SkelAnime* skelAnime, f32 framesFromEnd) {
    f32 targetVelocity;
    s16 targetYaw;

    if ((skelAnime->endFrame - framesFromEnd) <= skelAnime->curFrame) {
        if (Player_SetupSubAction(play, this, sStandStillSubActions, 1)) {
            return 0;
        }

        if (Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play)) {
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

    if ((this->heldItemActionParam == PLAYER_AP_STICK) || Player_HoldsBrokenKnife(this)) {
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

    if ((this->meleeWeaponAnimation >= PLAYER_MWA_LEFT_SLASH_1H) &&
        (this->meleeWeaponAnimation <= PLAYER_MWA_LEFT_COMBO_2H)) {
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
    PLAYER_MWA_STAB_1H,
    PLAYER_MWA_LEFT_SLASH_1H,
    PLAYER_MWA_LEFT_SLASH_1H,
    PLAYER_MWA_RIGHT_SLASH_1H,
};
static s8 sHammerAttackDirections[] = {
    PLAYER_MWA_HAMMER_FORWARD,
    PLAYER_MWA_HAMMER_SIDE,
    PLAYER_MWA_HAMMER_FORWARD,
    PLAYER_MWA_HAMMER_SIDE,
};

s32 Player_GetMeleeAttackAnim(Player* this) {
    s32 relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];
    s32 attackAnim;

    if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
        if (relativeStickInput < PLAYER_RELATIVESTICKINPUT_FORWARD) {
            relativeStickInput = PLAYER_RELATIVESTICKINPUT_FORWARD;
        }
        attackAnim = sHammerAttackDirections[relativeStickInput];
        this->slashCounter = 0;
    } else {
        if (Player_CanQuickspin(this)) {
            attackAnim = PLAYER_MWA_SPIN_ATTACK_1H;
        } else {
            if (relativeStickInput < PLAYER_RELATIVESTICKINPUT_FORWARD) {
                if (Player_IsZTargeting(this)) {
                    attackAnim = PLAYER_MWA_FORWARD_SLASH_1H;
                } else {
                    attackAnim = PLAYER_MWA_LEFT_SLASH_1H;
                }
            } else {
                attackAnim = sMeleeWeaponAttackDirections[relativeStickInput];
                if (attackAnim == PLAYER_MWA_STAB_1H) {
                    this->stateFlags2 |= PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
                    if (!Player_IsZTargeting(this)) {
                        attackAnim = PLAYER_MWA_FORWARD_SLASH_1H;
                    }
                }
            }
            if (this->heldItemActionParam == PLAYER_AP_STICK) {
                attackAnim = PLAYER_MWA_FORWARD_SLASH_1H;
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

void Player_SetupMeleeWeaponAttackBehavior(PlayState* play, Player* this, s32 meleeWeaponAnim) {
    s32 pad;
    u32 dmgFlags;
    s32 meleeWeapon;

    Player_SetActionFunc(play, this, Player_MeleeWeaponAttack, 0);
    this->comboTimer = 8;
    if (!((meleeWeaponAnim >= PLAYER_MWA_FLIPSLASH_FINISH) && (meleeWeaponAnim <= PLAYER_MWA_JUMPSLASH_FINISH))) {
        Player_InactivateMeleeWeapon(this);
    }

    if ((meleeWeaponAnim != this->meleeWeaponAnimation) || !(this->slashCounter < 3)) {
        this->slashCounter = 0;
    }

    this->slashCounter++;
    if (this->slashCounter >= 3) {
        meleeWeaponAnim += 2;
    }

    this->meleeWeaponAnimation = meleeWeaponAnim;

    Player_PlayAnimOnceSlowed(play, this, D_80854190[meleeWeaponAnim].startAnim);
    if ((meleeWeaponAnim != PLAYER_MWA_FLIPSLASH_START) && (meleeWeaponAnim != PLAYER_MWA_JUMPSLASH_START)) {
        Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
    }

    this->currentYaw = this->actor.shape.rot.y;

    if (Player_HoldsBrokenKnife(this)) {
        meleeWeapon = PLAYER_MELEEWEAPON_SWORD_MASTER;
    } else {
        meleeWeapon = Player_GetMeleeWeaponHeld(this) - 1;
    }

    if ((meleeWeaponAnim >= PLAYER_MWA_FLIPSLASH_START) && (meleeWeaponAnim <= PLAYER_MWA_JUMPSLASH_FINISH)) {
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
    Player_PlayAnimLoop(play, this, &gPlayerAnim_003040);
    this->genericTimer = 1;
    if (this->attentionMode != PLAYER_ATTENTIONMODE_CUTSCENE) {
        this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
    }
}

static LinkAnimationHeader* sLinkDamageAnims[] = {
    &gPlayerAnim_002F80, &gPlayerAnim_002F78, &gPlayerAnim_002DE0, &gPlayerAnim_002DD8,
    &gPlayerAnim_002F70, &gPlayerAnim_002528, &gPlayerAnim_002DC8, &gPlayerAnim_0024F0,
};

void Player_SetupDamage(PlayState* play, Player* this, s32 damageReaction, f32 knockbackVelXZ, f32 knockbackVelY, s16 damageYaw, s32 invincibilityTimer) {
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

        anim = &gPlayerAnim_002FD0;

        Player_ClearAttentionModeAndStopMoving(this);
        Player_RequestRumble(this, 255, 10, 40, 0);

        func_8002F7DC(&this->actor, NA_SE_PL_FREEZE_S);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
    } else if (damageReaction == PLAYER_DMGREACTION_ELECTRIC_SHOCKED) {
        Player_SetActionFunc(play, this, Player_SetupElectricShock, 0);

        Player_RequestRumble(this, 255, 80, 150, 0);

        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_002F00);
        Player_ClearAttentionModeAndStopMoving(this);

        this->genericTimer = 20;
    } else {
        damageYaw -= this->actor.shape.rot.y;
        if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
            Player_SetActionFunc(play, this, func_8084E30C, 0);
            Player_RequestRumble(this, 180, 20, 50, 0);

            this->linearVelocity = 4.0f;
            this->actor.velocity.y = 0.0f;

            anim = &gPlayerAnim_003320;

            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        } else if ((damageReaction == PLAYER_DMGREACTION_KNOCKBACK) || (damageReaction == PLAYER_DMGREACTION_HOP) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                   (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING))) {
            Player_SetActionFunc(play, this, func_8084377C, 0);

            this->stateFlags3 |= PLAYER_STATE3_MIDAIR;

            Player_RequestRumble(this, 255, 20, 150, 0);
            Player_ClearAttentionModeAndStopMoving(this);

            if (damageReaction == PLAYER_DMGREACTION_HOP) {
                this->genericTimer = 4;

                this->actor.speedXZ = 3.0f;
                this->linearVelocity = 3.0f;
                this->actor.velocity.y = 6.0f;

                Player_ChangeAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_RUNNING_DAMAGED, this->modelAnimType));
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
            } else {
                this->actor.speedXZ = knockbackVelXZ;
                this->linearVelocity = knockbackVelXZ;
                this->actor.velocity.y = knockbackVelY;

                if (ABS(damageYaw) > DEG_TO_BINANG(90.0f)) {
                    anim = &gPlayerAnim_002F58;
                } else {
                    anim = &gPlayerAnim_002DB0;
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

s32 Player_GetHurtFloorType(s32 specialFloorProperty) {
    s32 hurtFloorType = specialFloorProperty - BGCHECK_FLOORSPECIALPROPERTY_HURT_FLOOR;

    if ((hurtFloorType >= PLAYER_HURTFLOORTYPE_DEFAULT) && (hurtFloorType < PLAYER_HURTFLOORTYPE_MAX)) {
        return hurtFloorType;
    } else {
        return PLAYER_HURTFLOORTYPE_NONE;
    }
}

s32 Player_IsFloorSinkingSand(s32 specialFloorProperty) {
    return (specialFloorProperty == BGCHECK_FLOORSPECIALPROPERTY_SHALLOW_SAND) ||
           (specialFloorProperty == BGCHECK_FLOORSPECIALPROPERTY_QUICKSAND_NO_HORSE) ||
           (specialFloorProperty == BGCHECK_FLOORSPECIALPROPERTY_QUICKSAND_HORSE_CAN_CROSS);
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
            Player_SetupInflictDamage(play, -16);
            this->voidRespawnCounter = 0;
        }
    } else {
        sinkingGroundVoidOut = ((Player_GetHeight(this) - 8.0f) < (this->sinkingOffsetY * this->actor.scale.y));

        if (sinkingGroundVoidOut || (this->actor.bgCheckFlags & BGCHECKFLAG_CRUSHED) || (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_VOID_ON_TOUCH) ||
            (this->stateFlags2 & PLAYER_STATE2_FORCE_VOID_OUT)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);

            if (sinkingGroundVoidOut) {
                Play_TriggerRespawn(play);
                Scene_SetTransitionForNextEntrance(play);
            } else {
                // Special case for getting crushed in Forest Temple's Checkboard Ceiling Hall or Shadow Temple's
                // Falling Spike Trap Room, to respawn the player in a specific place
                if (((play->sceneNum == SCENE_BMORI1) && (play->roomCtx.curRoom.num == 15)) ||
                    ((play->sceneNum == SCENE_HAKADAN) && (play->roomCtx.curRoom.num == 10))) {
                    static SpecialRespawnInfo checkboardCeilingRespawn = { { 1992.0f, 403.0f, -3432.0f }, 0 };
                    static SpecialRespawnInfo fallingSpikeTrapRespawn = { { 1200.0f, -1343.0f, 3850.0f }, 0 };
                    SpecialRespawnInfo* respawnInfo;

                    if (play->sceneNum == SCENE_BMORI1) {
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
        } else if ((this->damageEffect != PLAYER_DMGEFFECT_NONE) && ((this->damageEffect >= PLAYER_DMGEFFECT_KNOCKBACK) || (this->invincibilityTimer == 0))) {
            u8 damageReactions[] = { PLAYER_DMGREACTION_HOP, PLAYER_DMGREACTION_KNOCKBACK, PLAYER_DMGREACTION_KNOCKBACK };

            Player_PlayFallSfxAndCheckBurning(this);

            if (this->damageEffect == PLAYER_DMGEFFECT_ELECTRIC_KNOCKBACK) {
                this->shockTimer = 40;
            }

            this->actor.colChkInfo.damage += this->damageAmount;
            Player_SetupDamage(play, this, damageReactions[this->damageEffect - 1], this->knockbackVelXZ, this->knockbackVelY, this->damageYaw, 20);
        } else {
            attackHitShield = (this->shieldQuad.base.acFlags & AC_BOUNCED) != 0;

            //! @bug The second set of conditions here seems intended as a way for Link to "block" hits by rolling.
            // However, `Collider.atFlags` is a byte so the flag check at the end is incorrect and cannot work.
            // Additionally, `Collider.atHit` can never be set while already colliding as AC, so it's also bugged.
            // This behavior was later fixed in MM, most likely by removing both the `atHit` and `atFlags` checks.
            if (attackHitShield || ((this->invincibilityTimer < 0) && (this->cylinder.base.acFlags & AC_HIT) &&
                         (this->cylinder.info.atHit != NULL) && (this->cylinder.info.atHit->atFlags & 0x20000000))) {

                Player_RequestRumble(this, 180, 20, 100, 0);

                if (!Player_IsChildWithHylianShield(this)) {
                    if (this->invincibilityTimer >= 0) {
                        LinkAnimationHeader* anim;
                        s32 sp54 = func_80843188 == this->actionFunc;

                        if (!Player_IsSwimming(this)) {
                            Player_SetActionFunc(play, this, func_808435C4, 0);
                        }

                        if (!(this->unk_84F = sp54)) {
                            Player_SetUpperActionFunc(this, func_80834BD4);

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

                    if (!(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING))) {
                        this->linearVelocity = -18.0f;
                        this->currentYaw = this->actor.shape.rot.y;
                    }
                }

                if (attackHitShield && (this->shieldQuad.info.acHitInfo->toucher.effect == 1)) {
                    Player_BurnDekuShield(this, play);
                }

                return 0;
            }

            if ((this->unk_A87 != 0) || (this->invincibilityTimer > 0) || (this->stateFlags1 & PLAYER_STATE1_TAKING_DAMAGE) ||
                (this->csMode != PLAYER_CSMODE_NONE) || (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) ||
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

                Player_SetupDamage(play, this, damageReaction, 4.0f, 5.0f, Actor_WorldYawTowardActor(ac, &this->actor), 20);
            } else if (this->invincibilityTimer != 0) {
                return 0;
            } else {
                static u8 dmgStartFrame[] = { 120, 60 };
                s32 hurtFloorType = Player_GetHurtFloorType(sFloorSpecialProperty);

                if (((this->actor.wallPoly != NULL) &&
                     SurfaceType_IsWallDamage(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) ||
                    ((hurtFloorType >= PLAYER_HURTFLOORTYPE_DEFAULT) &&
                     SurfaceType_IsWallDamage(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) &&
                     (this->hurtFloorTimer >= dmgStartFrame[hurtFloorType])) ||
                    ((hurtFloorType >= PLAYER_HURTFLOORTYPE_DEFAULT) &&
                     ((this->currentTunic != PLAYER_TUNIC_GORON) || (this->hurtFloorTimer >= dmgStartFrame[hurtFloorType])))) {
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
    LinkAnimationHeader* sp38;
    f32 sp34;
    f32 temp;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;
    f32 sp24;

    if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->touchedWallJumpType >= PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP) &&
        (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->ageProperties->unk_14 > this->wallHeight))) {
        canJumpToLedge = false;

        if (Player_IsSwimming(this)) {
            if (this->actor.yDistToWater < 50.0f) {
                if ((this->touchedWallJumpType < PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP) || (this->wallHeight > this->ageProperties->unk_10)) {
                    return 0;
                }
            } else if ((this->currentBoots != PLAYER_BOOTS_IRON) || (this->touchedWallJumpType > PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP)) {
                return 0;
            }
        } else if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                   ((this->ageProperties->unk_14 <= this->wallHeight) && (this->stateFlags1 & PLAYER_STATE1_SWIMMING))) {
            return 0;
        }

        if ((this->actor.wallBgId != BGCHECK_SCENE) && (sTouchedWallFlags & 0x40)) {
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

            sp34 = this->wallHeight;

            if (this->ageProperties->unk_14 <= sp34) {
                sp38 = &gPlayerAnim_002D48;
                this->linearVelocity = 1.0f;
            } else {
                wallPolyNormalX = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.x);
                wallPolyNormalZ = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.z);
                sp24 = this->wallDistance + 0.5f;

                this->stateFlags1 |= PLAYER_STATE1_CLIMBING_ONTO_LEDGE;

                if (Player_IsSwimming(this)) {
                    sp38 = &gPlayerAnim_0032E8;
                    sp34 -= (60.0f * this->ageProperties->translationScale);
                    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;
                } else if (this->ageProperties->unk_18 <= sp34) {
                    sp38 = &gPlayerAnim_002D40;
                    sp34 -= (59.0f * this->ageProperties->translationScale);
                } else {
                    sp38 = &gPlayerAnim_002D38;
                    sp34 -= (41.0f * this->ageProperties->translationScale);
                }

                this->actor.shape.yOffset -= sp34 * 100.0f;

                this->actor.world.pos.x -= sp24 * wallPolyNormalX;
                this->actor.world.pos.y += this->wallHeight;
                this->actor.world.pos.z -= sp24 * wallPolyNormalZ;

                Player_ClearAttentionModeAndStopMoving(this);
            }

            this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;

            LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sp38, 1.3f);
            AnimationContext_DisableQueue(play);

            this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;

            return 1;
        }
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->touchedWallJumpType == PLAYER_WALLJUMPTYPE_HOP_UP) && (this->wallTouchTimer >= 3)) {
        temp = (this->wallHeight * 0.08f) + 5.5f;
        Player_SetupJump(this, &gPlayerAnim_002FE0, temp, play);
        this->linearVelocity = 2.5f;

        return 1;
    }

    return 0;
}

void Player_SetupMiniCsMovement(PlayState* play, Player* this, f32 xzOffset, s16 yaw) {
    Player_SetActionFunc(play, this, func_80845CA4, 0);
    Player_ResetAttributes(play, this);

    this->unk_84F = 1;
    this->genericTimer = 1;

    this->csStartPos.x = (Math_SinS(yaw) * xzOffset) + this->actor.world.pos.x;
    this->csStartPos.z = (Math_CosS(yaw) * xzOffset) + this->actor.world.pos.z;

    Player_PlayAnimOnce(play, this, Player_GetStandingStillAnim(this));
}

void Player_SetupSwimIdle(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_UpdateSwimIdle, 0);
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_003328);
}

void Player_SetupEnterGrotto(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, func_8084F88C, 0);

    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID;

    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_FREE0);
}

s32 Player_ShouldEnterGrotto(PlayState* play, Player* this) {
    if ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (this->stateFlags1 & PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID)) {
        Player_SetupEnterGrotto(play, this);
        Player_PlayAnimLoop(play, this, &gPlayerAnim_003040);
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
 * The spawn value (`PlayState.curSpawn`) is set to a different value depending on the entrance used to enter the
 * scene, which allows these dynamic "return entrances" to link back to the previous scene.
 *
 * Note: grottos and normal fairy fountains use `ENTR_RETURN_GROTTO`
 */
s16 sReturnEntranceGroupData[] = {
    // ENTR_RETURN_DAIYOUSEI_IZUMI
    /*  0 */ ENTR_SPOT16_4, // DMT from Magic Fairy Fountain
    /*  1 */ ENTR_SPOT17_3, // DMC from Double Defense Fairy Fountain
    /*  2 */ ENTR_SPOT15_2, // Hyrule Castle from Dins Fire Fairy Fountain

    // ENTR_RETURN_2
    /*  3 */ ENTR_SPOT01_9,     // Kakariko from Potion Shop
    /*  4 */ ENTR_MARKET_DAY_5, // Market (child day) from Potion Shop

    // ENTR_RETURN_SHOP1
    /*  5 */ ENTR_SPOT01_3,     // Kakariko from Bazaar
    /*  6 */ ENTR_MARKET_DAY_6, // Market (child day) from Bazaar

    // ENTR_RETURN_4
    /*  7 */ ENTR_SPOT01_11,      // Kakariko from House of Skulltulas
    /*  8 */ ENTR_MARKET_ALLEY_2, // Back Alley (day) from Bombchu Shop

    // ENTR_RETURN_SYATEKIJYOU
    /*  9 */ ENTR_SPOT01_10,    // Kakariko from Shooting Gallery
    /* 10 */ ENTR_MARKET_DAY_8, // Market (child day) from Shooting Gallery

    // ENTR_RETURN_YOUSEI_IZUMI_YOKO
    /* 11 */ ENTR_SPOT08_5, // Zoras Fountain from Farores Wind Fairy Fountain
    /* 12 */ ENTR_SPOT15_2, // Hyrule Castle from Dins Fire Fairy Fountain
    /* 13 */ ENTR_SPOT11_7, // Desert Colossus from Nayrus Love Fairy Fountain
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
            (((poly != NULL) &&
              (exitIndex = SurfaceType_GetSceneExitIndex(&play->colCtx, poly, bgId), exitIndex != 0)) ||
             (Player_IsFloorSinkingSand(sFloorSpecialProperty) && (this->floorProperty == BGCHECK_FLOORPROPERTY_VOID_PIT_LARGE)))) {

            yDistToExit = this->sceneExitPosY - (s32)this->actor.world.pos.y;

            if (!(this->stateFlags1 & (PLAYER_STATE1_RIDING_HORSE | PLAYER_STATE1_SWIMMING | PLAYER_STATE1_IN_CUTSCENE)) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (yDistToExit < 100) && (sPlayerYDistToFloor > 100.0f)) {
                return 0;
            }

            if (exitIndex == 0) {
                Play_TriggerVoidOut(play);
                Scene_SetTransitionForNextEntrance(play);
            } else {
                play->nextEntranceIndex = play->setupExitList[exitIndex - 1];

                if (play->nextEntranceIndex == ENTR_RETURN_GROTTO) {
                    gSaveContext.respawnFlag = 2;
                    play->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex;
                    play->transitionType = TRANS_TYPE_FADE_WHITE;
                    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
                } else if (play->nextEntranceIndex >= ENTR_RETURN_YOUSEI_IZUMI_YOKO) {
                    play->nextEntranceIndex =
                        sReturnEntranceGroupData[sReturnEntranceGroupIndices[play->nextEntranceIndex -
                                                                             ENTR_RETURN_YOUSEI_IZUMI_YOKO] +
                                                 play->curSpawn];
                    Scene_SetTransitionForNextEntrance(play);
                } else {
                    if (SurfaceType_GetSlope(&play->colCtx, poly, bgId) == 2) {
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
                !(this->stateFlags2 & PLAYER_STATE2_INSIDE_CRAWLSPACE) && !Player_IsSwimming(this) &&
                (floorSpecialProperty = func_80041D4C(&play->colCtx, poly, bgId), (floorSpecialProperty != BGCHECK_FLOORSPECIALPROPERTY_10)) &&
                ((yDistToExit < 100) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {

                if (floorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_LOOK_UP) {
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

                    if (sConveyorSpeedIndex != 0) {
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
                    (((this->floorProperty == BGCHECK_FLOORPROPERTY_VOID_PIT) || (this->floorProperty == BGCHECK_FLOORPROPERTY_VOID_PIT_LARGE)) &&
                     ((sPlayerYDistToFloor < 100.0f) || (this->fallDistance > 400.0f) ||
                      ((play->sceneNum != SCENE_HAKADAN) && (this->fallDistance > 200.0f)))) ||
                    ((play->sceneNum == SCENE_GANON_FINAL) && (this->fallDistance > 320.0f))) {

                    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                        if (this->floorProperty == BGCHECK_FLOORPROPERTY_VOID_PIT) {
                            Play_TriggerRespawn(play);
                        } else {
                            Play_TriggerVoidOut(play);
                        }
                        play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                        func_80078884(NA_SE_OC_ABYSS);
                    } else {
                        Player_SetupEnterGrotto(play, this);
                        this->genericTimer = 9999;
                        if (this->floorProperty == BGCHECK_FLOORPROPERTY_VOID_PIT) {
                            this->unk_84F = -1;
                        } else {
                            this->unk_84F = 1;
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
f32 Player_RaycastFloorWithOffset(PlayState* play, Player* this, Vec3f* raycastPosOffset, Vec3f* raycastPos, CollisionPoly** colPoly, s32* bgId) {
    Player_GetWorldPosRelativeToPlayer(this, &this->actor.world.pos, raycastPosOffset, raycastPos);

    return BgCheck_EntityRaycastFloor3(&play->colCtx, colPoly, bgId, raycastPos);
}

// Returns floor Y, or BGCHECK_MIN_Y if no floor detected
f32 Player_RaycastFloorWithOffset2(PlayState* play, Player* this, Vec3f* raycastPosOffset, Vec3f* raycastPos) {
    CollisionPoly* colPoly;
    s32 polyBgId;

    return Player_RaycastFloorWithOffset(play, this, raycastPosOffset, raycastPos, &colPoly, &polyBgId);
}

s32 Player_WallLineTestWithOffset(PlayState* play, Player* this, Vec3f* posOffset, CollisionPoly** wallPoly, s32* bgId, Vec3f* posResult) {
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
    EnDoor* door; // Can also be DoorKiller*
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

                this->unk_84F = 0;
                this->unk_447 = this->doorType;
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
                    this->doorBgCamIndex = play->transiActorCtx.list[(u16)doorShutter->dyna.actor.params >> 10]
                                               .sides[(doorDirection > 0) ? 0 : 1]
                                               .bgCamIndex;

                    Actor_DisableLens(play);
                }
            } else {
                // This actor can be either EnDoor or DoorKiller.
                // Don't try to access any struct vars other than `animStyle` and `playerIsOpening`! These two variables
                // are common across the two actors' structs however most other variables are not!
                door = (EnDoor*)doorActor;

                door->animStyle = (doorDirection < 0.0f) ? (LINK_IS_ADULT ? KNOB_ANIM_ADULT_L : KNOB_ANIM_CHILD_L)
                                                         : (LINK_IS_ADULT ? KNOB_ANIM_ADULT_R : KNOB_ANIM_CHILD_R);

                if (door->animStyle == KNOB_ANIM_ADULT_L) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_LEFT, this->modelAnimType);
                } else if (door->animStyle == KNOB_ANIM_CHILD_L) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_LEFT, this->modelAnimType);
                } else if (door->animStyle == KNOB_ANIM_ADULT_R) {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_RIGHT, this->modelAnimType);
                } else {
                    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_RIGHT, this->modelAnimType);
                }

                Player_SetActionFunc(play, this, func_80845EF8, 0);
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
                              PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                  PLAYER_ANIMMOVEFLAGS_7 | PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);

                if (doorActor->parent != NULL) {
                    doorDirection = -doorDirection;
                }

                door->playerIsOpening = 1;

                if (this->doorType != PLAYER_DOORTYPE_FAKE) {
                    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                    Actor_DisableLens(play);

                    if (((doorActor->params >> 7) & 7) == 3) {
                        raycastPos.x = doorActor->world.pos.x - (doorOpeningPosOffset * sin);
                        raycastPos.y = doorActor->world.pos.y + 10.0f;
                        raycastPos.z = doorActor->world.pos.z - (doorOpeningPosOffset * cos);

                        BgCheck_EntityRaycastFloor1(&play->colCtx, &floorPoly, &raycastPos);

                        if (Player_SetupExit(play, this, floorPoly, BGCHECK_SCENE)) {
                            gSaveContext.entranceSpeed = 2.0f;
                            gSaveContext.entranceSound = NA_SE_OC_DOOR_OPEN;
                        }
                    } else {
                        Camera_ChangeDoorCam(Play_GetCamera(play, CAM_ID_MAIN), doorActor,
                                             play->transiActorCtx.list[(u16)doorActor->params >> 10]
                                                 .sides[(doorDirection > 0) ? 0 : 1]
                                                 .bgCamIndex,
                                             0, 38.0f * sInvertedWaterSpeedScale, 26.0f * sInvertedWaterSpeedScale, 10.0f * sInvertedWaterSpeedScale);
                    }
                }
            }

            if ((this->doorType != PLAYER_DOORTYPE_FAKE) && (doorActor->category == ACTORCAT_DOOR)) {
                frontRoom =
                    play->transiActorCtx.list[(u16)doorActor->params >> 10].sides[(doorDirection > 0) ? 0 : 1].room;

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
            Player_SetActionFunc(play, this, func_8084F608, 0);
            this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        } else {
            LinkAnimationHeader* anim;

            if (interactActorId == ACTOR_BG_HEAVY_BLOCK) {
                Player_SetActionFunc(play, this, Player_ThrowStonePillar, 0);
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                anim = &gPlayerAnim_002F98;
            } else if ((interactActorId == ACTOR_EN_ISHI) && ((interactRangeActor->params & 0xF) == 1)) {
                Player_SetActionFunc(play, this, Player_LiftSilverBoulder, 0);
                anim = &gPlayerAnim_0032B0;
            } else if (((interactActorId == ACTOR_EN_BOMBF) || (interactActorId == ACTOR_EN_KUSA)) &&
                       (Player_GetStrength() <= PLAYER_STR_NONE)) {
                Player_SetActionFunc(play, this, Player_FailToLiftActor, 0);
                this->actor.world.pos.x =
                    (Math_SinS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.x;
                this->actor.world.pos.z =
                    (Math_CosS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.z;
                this->currentYaw = this->actor.shape.rot.y = interactRangeActor->yawTowardsPlayer + 0x8000;
                anim = &gPlayerAnim_003060;
            } else {
                Player_SetActionFunc(play, this, Player_LiftActor, 0);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_HOLDING_OBJECT, this->modelAnimType);
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

void Player_SetupClimbDownFromLedge(PlayState* play, Player* this) {
    s32 sp1C = this->genericTimer;
    s32 sp18 = this->unk_84F;

    Player_SetActionFuncPreserveMoveFlags(play, this, Player_ClimbDownFromLedge, 0);
    this->actor.velocity.y = 0.0f;

    this->genericTimer = sp1C;
    this->unk_84F = sp18;
}

void func_8083A40C(PlayState* play, Player* this) {
    Player_SetActionFuncPreserveMoveFlags(play, this, func_8084C760, 0);
}

void func_8083A434(PlayState* play, Player* this) {
    Player_SetActionFuncPreserveMoveFlags(play, this, func_8084E6D4, 0);

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
        anim = &gPlayerAnim_003148;
    } else {
        anim = &gPlayerAnim_002FE0;
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

void Player_SetupGrabLedge(PlayState* play, Player* this, CollisionPoly* colPoly, f32 distToPoly, LinkAnimationHeader* anim) {
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

    if ((this->actor.yDistToWater < -80.0f) && (ABS(this->angleToFloorX) < DEG_TO_BINANG(15.0f)) && (ABS(this->angleToFloorY) < DEG_TO_BINANG(15.0f))) {
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

        if (BgCheck_EntityLineTest1(&play->colCtx, &this->actor.world.pos, &pos, &colPolyPos, &colPoly, true, false, false,
                                    true, &polyBgId) &&
            (ABS(colPoly->normal.y) < 600)) {
            f32 nx = COLPOLY_GET_NORMAL(colPoly->normal.x);
            f32 ny = COLPOLY_GET_NORMAL(colPoly->normal.y);
            f32 nz = COLPOLY_GET_NORMAL(colPoly->normal.z);
            f32 distToPoly;
            s32 shouldClimbDownAdjacentWall;

            distToPoly = Math3D_UDistPlaneToPos(nx, ny, nz, colPoly->dist, &this->actor.world.pos);

            shouldClimbDownAdjacentWall = sFloorProperty == BGCHECK_FLOORPROPERTY_CLIMB_DOWN_ADJACENT_WALL;
            if (!shouldClimbDownAdjacentWall && (func_80041DB8(&play->colCtx, colPoly, polyBgId) & 8)) {
                shouldClimbDownAdjacentWall = true;
            }

            Player_SetupGrabLedge(play, this, colPoly, distToPoly, shouldClimbDownAdjacentWall ? &gPlayerAnim_002D88 : &gPlayerAnim_002F10);

            if (shouldClimbDownAdjacentWall) {
                Player_SetupMiniCsFunc(play, this, Player_SetupClimbDownFromLedge);

                this->currentYaw += 0x8000;
                this->actor.shape.rot.y = this->currentYaw;

                this->stateFlags1 |= PLAYER_STATE1_CLIMBING;
                Player_SetupAnimMovement(play, this,
                              PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                  PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);

                this->genericTimer = -1;
                this->unk_84F = shouldClimbDownAdjacentWall;
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
            if (sFloorProperty == BGCHECK_FLOORPROPERTY_STOP_XZ_MOMENTUM) {
                this->actor.world.pos.x = this->actor.prevPos.x;
                this->actor.world.pos.z = this->actor.prevPos.z;
                return;
            }

            if (!(this->stateFlags3 & PLAYER_STATE3_MIDAIR) && !(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_7) &&
                (Player_UpdateMidair != this->actionFunc) && (Player_FallingDive != this->actionFunc)) {

                if ((sFloorProperty == BGCHECK_FLOORPROPERTY_STOP_ALL_MOMENTUM) || (this->isMeleeWeaponAttacking != 0)) {
                    Math_Vec3f_Copy(&this->actor.world.pos, &this->actor.prevPos);
                    Player_StopMovement(this);
                    return;
                }

                if (this->hoverBootsTimer != 0) {
                    this->actor.velocity.y = 1.0f;
                    sFloorProperty = BGCHECK_FLOORPROPERTY_NO_JUMPING;
                    return;
                }

                yawDiff = (s16)(this->currentYaw - this->actor.shape.rot.y);

                Player_SetActionFunc(play, this, Player_UpdateMidair, 1);
                Player_ResetAttributes(play, this);

                this->moveSfxType = this->prevMoveSfxType;

                if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE) && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) &&
                    (sFloorProperty != BGCHECK_FLOORPROPERTY_CLIMB_DOWN_ADJACENT_WALL) && (sFloorProperty != BGCHECK_FLOORPROPERTY_NO_JUMPING) && (sPlayerYDistToFloor > 20.0f) && (this->isMeleeWeaponAttacking == 0) &&
                    (ABS(yawDiff) < DEG_TO_BINANG(45.0f)) && (this->linearVelocity > 3.0f)) {

                    if ((sFloorProperty == BGCHECK_FLOORPROPERTY_FALLING_DIVE) && !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {

                        floorPosY = Player_RaycastFloorWithOffset(play, this, &sWaterRaycastOffset, &raycastPos, &floorPoly, &floorBgId);
                        waterPosY = this->actor.world.pos.y;

                        if (WaterBox_GetSurface1(play, &play->colCtx, raycastPos.x, raycastPos.z, &waterPosY, &waterbox) &&
                            ((waterPosY - floorPosY) > 50.0f)) {
                            Player_SetupJump(this, &gPlayerAnim_003158, 6.0f, play);
                            Player_SetActionFunc(play, this, Player_FallingDive, 0);
                            return;
                        }
                    }

                    Player_StartJump(this, play);
                    return;
                }

                if ((sFloorProperty == BGCHECK_FLOORPROPERTY_NO_JUMPING) || (sPlayerYDistToFloor <= this->ageProperties->unk_34) || !Player_SetupGrabLedgeInsteadOfFalling(this, play)) {
                    Player_PlayAnimLoop(play, this, &gPlayerAnim_003040);
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

        DmaMgr_SendRequest2(&this->giObjectDmaRequest, this->giObjectSegment, gObjectTable[objectId].vromStart, size, 0,
                            &this->giObjectLoadQueue, NULL, "../z_player.c", 9099);
    }
}

void Player_SetupMagicSpell(PlayState* play, Player* this, s32 magicSpell) {
    Player_SetActionFuncPreserveItemAP(play, this, Player_UpdateMagicSpell, 0);

    this->magicSpellType = magicSpell - 3;
    Magic_RequestChange(play, sMagicSpellCosts[magicSpell], MAGIC_CONSUME_WAIT_PREVIEW);

    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_002D28, 0.83f);

    if (magicSpell == 5) {
        this->subCamId = OnePointCutscene_Init(play, 1100, -101, NULL, CAM_ID_MAIN);
    } else {
        Player_SetCameraTurnAround(play, 10);
    }
}

void Player_ResetLookAngles(Player* this) {
    this->actor.focus.rot.x = this->actor.focus.rot.z = this->headRot.x = this->headRot.y = this->headRot.z = this->upperBodyRot.x =
        this->upperBodyRot.y = this->upperBodyRot.z = 0;

    this->actor.focus.rot.y = this->actor.shape.rot.y;
}

static u8 sExchangeGetItemIDs[] = {
    GI_LETTER_ZELDA, GI_WEIRD_EGG,    GI_CHICKEN,     GI_BEAN,        GI_POCKET_EGG,   GI_POCKET_CUCCO,
    GI_COJIRO,       GI_ODD_MUSHROOM, GI_ODD_POTION,  GI_SAW,         GI_SWORD_BROKEN, GI_PRESCRIPTION,
    GI_FROG,         GI_EYEDROPS,     GI_CLAIM_CHECK, GI_MASK_SKULL,  GI_MASK_SPOOKY,  GI_MASK_KEATON,
    GI_MASK_BUNNY,   GI_MASK_TRUTH,   GI_MASK_GORON,  GI_MASK_ZORA,   GI_MASK_GERUDO,  GI_LETTER_RUTO,
    GI_LETTER_RUTO,  GI_LETTER_RUTO,  GI_LETTER_RUTO, GI_LETTER_RUTO, GI_LETTER_RUTO,
};

static LinkAnimationHeader* sExchangeItemAnims[] = {
    &gPlayerAnim_002F88,
    &gPlayerAnim_002690,
    &gPlayerAnim_003198,
};

s32 Player_SetupItemCutsceneOrCUp(Player* this, PlayState* play) {
    s32 item;
    s32 sp28;
    GetItemEntry* giEntry;
    Actor* talkActor;

    if ((this->attentionMode != PLAYER_ATTENTIONMODE_NONE) && (Player_IsSwimming(this) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
                                 (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE))) {

        if (!Player_SetupCutscene(play, this)) {
            if (this->attentionMode == PLAYER_ATTENTIONMODE_ITEM_CUTSCENE) {
                item = Player_ActionToMagicSpell(this, this->itemActionParam);
                if (item >= PLAYER_MAGICSPELL_UNUSED_15) {
                    if ((item != PLAYER_MAGICSPELL_FARORES_WIND) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
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

                item = this->itemActionParam - PLAYER_AP_LETTER_ZELDA;
                if ((item >= 0) ||
                    (sp28 = Player_ActionToBottle(this, this->itemActionParam) - 1,
                     ((sp28 >= 0) && (sp28 < 6) &&
                      ((this->itemActionParam > PLAYER_AP_BOTTLE_POE) ||
                       ((this->talkActor != NULL) &&
                        (((this->itemActionParam == PLAYER_AP_BOTTLE_POE) && (this->exchangeItemId == EXCH_ITEM_POE)) ||
                         (this->exchangeItemId == EXCH_ITEM_BLUE_FIRE))))))) {

                    if ((play->actorCtx.titleCtx.delayTimer == 0) && (play->actorCtx.titleCtx.alpha == 0)) {
                        Player_SetActionFuncPreserveItemAP(play, this, func_8084F104, 0);

                        if (item >= 0) {
                            giEntry = &sGetItemTable[sExchangeGetItemIDs[item] - 1];
                            Player_LoadGetItemObject(this, giEntry->objectId);
                        }

                        this->stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;

                        if (item >= 0) {
                            item = item + 1;
                        } else {
                            item = sp28 + 0x18;
                        }

                        talkActor = this->talkActor;

                        if ((talkActor != NULL) &&
                            ((this->exchangeItemId == item) || (this->exchangeItemId == EXCH_ITEM_BLUE_FIRE) ||
                             ((this->exchangeItemId == EXCH_ITEM_POE) &&
                              (this->itemActionParam == PLAYER_AP_BOTTLE_BIG_POE)) ||
                             ((this->exchangeItemId == EXCH_ITEM_BEAN) &&
                              (this->itemActionParam == PLAYER_AP_BOTTLE_BUG))) &&
                            ((this->exchangeItemId != EXCH_ITEM_BEAN) || (this->itemActionParam == PLAYER_AP_BEAN))) {
                            if (this->exchangeItemId == EXCH_ITEM_BEAN) {
                                Inventory_ChangeAmmo(ITEM_BEAN, -1);
                                Player_SetActionFuncPreserveItemAP(play, this, func_8084279C, 0);
                                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
                                this->genericTimer = 0x50;
                                this->unk_84F = -1;
                            }
                            talkActor->flags |= ACTOR_FLAG_8;
                            this->targetActor = this->talkActor;
                        } else if (item == EXCH_ITEM_LETTER_RUTO) {
                            this->unk_84F = 1;
                            this->actor.textId = 0x4005;
                            Player_SetCameraTurnAround(play, 1);
                        } else {
                            this->unk_84F = 2;
                            this->actor.textId = 0xCF;
                            Player_SetCameraTurnAround(play, 4);
                        }

                        this->actor.flags |= ACTOR_FLAG_8;
                        this->exchangeItemId = item;

                        if (this->unk_84F < 0) {
                            Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_START_CHECKING_OR_SPEAKING, this->modelAnimType));
                        } else {
                            Player_PlayAnimOnce(play, this, sExchangeItemAnims[this->unk_84F]);
                        }

                        Player_ClearAttentionModeAndStopMoving(this);
                    }
                    return 1;
                }

                item = Player_ActionToBottle(this, this->itemActionParam);
                if (item >= PLAYER_BOTTLECONTENTS_NONE) {
                    if (item == PLAYER_BOTTLECONTENTS_FAIRY) {
                        Player_SetActionFuncPreserveItemAP(play, this, func_8084EED8, 0);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002650);
                        Player_SetCameraTurnAround(play, 3);
                    } else if ((item > PLAYER_BOTTLECONTENTS_NONE) && (item < PLAYER_BOTTLECONTENTS_POE)) {
                        Player_SetActionFuncPreserveItemAP(play, this, Player_DropItemFromBottle, 0);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002688);
                        Player_SetCameraTurnAround(play, (item == 1) ? 1 : 5);
                    } else {
                        Player_SetActionFuncPreserveItemAP(play, this, func_8084EAC0, 0);
                        Player_ChangeAnimSlowedMorphToLastFrame(play, this, &gPlayerAnim_002668);
                        Player_SetCameraTurnAround(play, 2);
                    }
                } else {
                    Player_SetActionFuncPreserveItemAP(play, this, func_8084E3C4, 0);
                    Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_0030A0);
                    this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_GENERAL;
                    Player_SetCameraTurnAround(play, (this->ocarinaActor != NULL) ? 0x5B : 0x5A);
                    if (this->ocarinaActor != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR;
                        Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->ocarinaActor);
                    }
                }
            } else if (Player_SetupCameraMode(play, this)) {
                if (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
                    Player_SetActionFunc(play, this, func_8084B1D8, 1);
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

    targetActorHasText = (targetActor != NULL) &&
           (CHECK_FLAG_ALL(targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) || (targetActor->naviEnemyId != NAVI_ENEMY_NONE));

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
                ((this->heldActor != NULL) && (naviHasText || (talkActor == this->heldActor) || (naviActor == this->heldActor) ||
                                               ((talkActor != NULL) && (talkActor->flags & ACTOR_FLAG_16))))) {
                if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) ||
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

                    this->currentMask = D_80858AA4;
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
        Player_SetupItemCutsceneOrCUp(this, play);
        return 1;
    }

    if ((this->targetActor != NULL) && (CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) ||
                                    (this->targetActor->naviEnemyId != NAVI_ENEMY_NONE))) {
        this->stateFlags2 |= PLAYER_STATE2_NAVI_REQUESTING_TALK;
    } else if ((this->naviTextId == 0) && !Player_IsUnfriendlyZTargeting(this) && CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) &&
               (R_SCENE_CAM_TYPE != SCENE_CAM_TYPE_FIXED_SHOP_VIEWPOINT) &&
               (R_SCENE_CAM_TYPE != SCENE_CAM_TYPE_FIXED_TOGGLE_VIEWPOINT) && !Player_ForceFirstPerson(this, play)) {
        func_80078884(NA_SE_SY_ERROR);
    }

    return 0;
}

void Player_SetupJumpSlash(PlayState* play, Player* this, s32 arg2, f32 xzVelocity, f32 yVelocity) {
    Player_SetupMeleeWeaponAttackBehavior(play, this, arg2);
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
    if (!(this->stateFlags1 & PLAYER_STATE1_SHIELDING) && (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE)) {
        if (sUsingItemAlreadyInHand ||
            ((this->actor.category != ACTORCAT_PLAYER) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B))) {
            return 1;
        }
    }

    return 0;
}

s32 Player_SetupMidairJumpSlash(Player* this, PlayState* play) {
    if (Player_CanJumpSlash(this) && (sFloorSpecialProperty != BGCHECK_FLOORSPECIALPROPERTY_QUICKSAND_NO_HORSE)) {
        Player_SetupJumpSlash(play, this, PLAYER_MWA_JUMPSLASH_START, 3.0f, 4.5f);
        return 1;
    }

    return 0;
}

void Player_SetupRolling(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_Rolling, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_ROLLING, this->modelAnimType),
                                   1.25f * sWaterSpeedScale);
}

s32 Player_CanRoll(Player* this, PlayState* play) {
    if ((this->relativeAnalogStickInputs[this->inputFrameCounter] == PLAYER_RELATIVESTICKINPUT_FORWARD) && (sFloorSpecialProperty != 7)) {
        Player_SetupRolling(this, play);
        return 1;
    }

    return 0;
}

void Player_SetupBackflipOrSidehop(Player* this, PlayState* play, s32 relativeStickInput) {
    Player_SetupJumpWithSfx(this, sManualJumpAnims[relativeStickInput][0], !(relativeStickInput & PLAYER_RELATIVESTICKINPUT_LEFT) ? 5.8f : 3.5f, play, NA_SE_VO_LI_SWORD_N);

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
        (play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) && (sFloorSpecialProperty != BGCHECK_FLOORSPECIALPROPERTY_QUICKSAND_NO_HORSE) &&
        (SurfaceType_GetSlope(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) != 1)) {
        relativeStickInput = this->relativeAnalogStickInputs[this->inputFrameCounter];

        if (relativeStickInput <= PLAYER_RELATIVESTICKINPUT_FORWARD) {
            if (Player_IsZTargeting(this)) {
                if (this->actor.category != ACTORCAT_PLAYER) {
                    if (relativeStickInput < 0) {
                        Player_SetupJump(this, &gPlayerAnim_002FE0, REG(69) / 100.0f, play);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                } else {
                    if ((Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) && Player_CanUseItem(this)) {
                        Player_SetupJumpSlash(play, this, PLAYER_MWA_JUMPSLASH_START, 5.0f, 5.0f);
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
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_WALK_ON_LEFT_FOOT, this->modelAnimType);
        frame = 11.0f - frame;
        if (frame < 0.0f) {
            frame = 1.375f * -frame;
        }
        frame /= 11.0f;
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_WALK_ON_RIGHT_FOOT, this->modelAnimType);
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

    this->stateFlags1 &= ~(PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_IN_FIRST_PERSON_MODE);
}

s32 Player_SetupRollOrPutAway(Player* this, PlayState* play) {
    if (!Player_SetupStartUnfriendlyZTargeting(this) && (D_808535E0 == 0) && !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
        CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        if (Player_CanRoll(this, play)) {
            return 1;
        }
        if ((this->unk_837 == 0) && (this->heldItemActionParam >= PLAYER_AP_SWORD_MASTER)) {
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

        if (Player_SetActionFunc(play, this, func_80843188, 0)) {
            this->stateFlags1 |= PLAYER_STATE1_SHIELDING;

            if (!Player_IsChildWithHylianShield(this)) {
                Player_SetModelsForHoldingShield(this);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_START_DEFENDING, this->modelAnimType);
            } else {
                anim = &gPlayerAnim_002400;
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

s32 func_8083C484(Player* this, f32* arg1, s16* arg2) {
    s16 yaw = this->currentYaw - *arg2;

    if (ABS(yaw) > 0x6000) {
        if (Player_StepLinearVelocityToZero(this)) {
            *arg1 = 0.0f;
            *arg2 = this->currentYaw;
        } else {
            return 1;
        }
    }

    return 0;
}

void func_8083C50C(Player* this) {
    if ((this->comboTimer > 0) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        this->comboTimer = -this->comboTimer;
    }
}

s32 Player_SetupStartChargeSpinAttack(Player* this, PlayState* play) {
    if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_SHIELDING) && (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) && (this->comboTimer == 1) &&
            (this->heldItemActionParam != PLAYER_AP_STICK)) {
            if ((this->heldItemActionParam != PLAYER_AP_SWORD_BGS) || (gSaveContext.swordHealth > 0.0f)) {
                Player_StartChargeSpinAttack(play, this);
                return 1;
            }
        }
    } else {
        func_8083C50C(this);
    }

    return 0;
}

s32 Player_SetupThrowDekuNut(PlayState* play, Player* this) {
    if ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (AMMO(ITEM_NUT) != 0)) {
        Player_SetActionFunc(play, this, Player_ThrowDekuNut, 0);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_003048);
        this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
        return 1;
    }

    return 0;
}

static struct_80854554 sBottleSwingAnims[] = {
    { &gPlayerAnim_002648, &gPlayerAnim_002640, 2, 3 },
    { &gPlayerAnim_002680, &gPlayerAnim_002678, 5, 3 },
};

s32 Player_CanSwingBottleOrCastFishingRod(PlayState* play, Player* this) {
    Vec3f checkPos;

    if (sUsingItemAlreadyInHand) {
        if (Player_GetBottleHeld(this) >= 0) {
            Player_SetActionFunc(play, this, Player_SwingBottle, 0);

            if (this->actor.yDistToWater > 12.0f) {
                this->genericTimer = 1;
            }

            Player_PlayAnimOnceSlowed(play, this, sBottleSwingAnims[this->genericTimer].unk_00);

            func_8002F7DC(&this->actor, NA_SE_IT_SWORD_SWING);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
            return 1;
        }

        if (this->heldItemActionParam == PLAYER_AP_FISHING_POLE) {
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
            Player_PlayAnimOnce(play, this, &gPlayerAnim_002C30);
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
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_RUNNING, this->modelAnimType));

    this->unk_89C = 0;
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
            Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_0032F0);
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
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_WALKING, this->modelAnimType));
}

void Player_SetupUnfriendlyBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UnfriendlyBackwalk, 1);
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_0024F8, 2.2f, 0.0f,
                         Animation_GetLastFrame(&gPlayerAnim_0024F8), ANIMMODE_ONCE, -6.0f);
    this->linearVelocity = 8.0f;
    this->currentYaw = yaw;
}

void Player_SetupSidewalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_Sidewalk, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SIDEWALKING_RIGHT, this->modelAnimType));
    this->walkFrame = 0.0f;
}

void Player_SetupEndUnfriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndUnfriendlyBackwalk, 1);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_0024E8, 2.0f);
}

void Player_SetupTurn(PlayState* play, Player* this, s16 yaw) {
    this->currentYaw = yaw;
    Player_SetActionFunc(play, this, Player_Turn, 1);
    this->unk_87E = 1200;
    this->unk_87E *= sWaterSpeedScale;
    LinkAnimation_Change(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SHUFFLE_TURN, this->modelAnimType), 1.0f, 0.0f,
                         0.0f, ANIMMODE_LOOP, -6.0f);
}

void Player_EndUnfriendlyZTarget(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;

    Player_SetActionFunc(play, this, Player_StandingStill, 1);

    if (this->leftRightBlendWeight < 0.5f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_FIGHTING_RIGHT_OF_ENEMY, this->modelAnimType);
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_FIGHTING_LEFT_OF_ENEMY, this->modelAnimType);
    }
    Player_PlayAnimOnce(play, this, anim);

    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupUnfriendlyZTarget(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_START_FIGHTING, this->modelAnimType));
    this->genericTimer = 1;
}

void Player_SetupEndUnfriendlyZTarget(Player* this, PlayState* play) {
    if (this->linearVelocity != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_EndUnfriendlyZTarget(this, play);
    }
}

void func_8083CF5C(Player* this, PlayState* play) {
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

s32 func_8083D12C(PlayState* play, Player* this, Input* arg2) {
    if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
        if ((arg2 == NULL) || (CHECK_BTN_ALL(arg2->press.button, BTN_A) && (ABS(this->unk_6C2) < 12000) &&
                               (this->currentBoots != PLAYER_BOOTS_IRON))) {

            Player_SetActionFunc(play, this, func_8084DC48, 0);
            Player_PlayAnimOnce(play, this, &gPlayerAnim_003308);

            this->unk_6C2 = 0;
            this->stateFlags2 |= PLAYER_STATE2_DIVING;
            this->actor.velocity.y = 0.0f;

            if (arg2 != NULL) {
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

                if (arg2 != NULL) {
                    Player_SetActionFunc(play, this, func_8084E1EC, 1);

                    if (this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) {
                        this->stateFlags1 |= PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                    }

                    this->genericTimer = 2;
                }

                Player_ResetSubCam(play, this);
                Player_ChangeAnimMorphToLastFrame(play, this,
                              (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ? &gPlayerAnim_003318 : &gPlayerAnim_003300);

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
    Player_PlayAnimLoop(play, this, &gPlayerAnim_0032F0);
    this->unk_6C2 = 16000;
    this->genericTimer = 1;
}

void func_8083D36C(PlayState* play, Player* this) {
    if ((this->currentBoots != PLAYER_BOOTS_IRON) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_ResetAttributesAndHeldActor(play, this);

        if ((this->currentBoots != PLAYER_BOOTS_IRON) && (this->stateFlags2 & PLAYER_STATE2_DIVING)) {
            this->stateFlags2 &= ~PLAYER_STATE2_DIVING;
            func_8083D12C(play, this, 0);
            this->unk_84F = 1;
        } else if (Player_FallingDive == this->actionFunc) {
            Player_SetActionFunc(play, this, func_8084DC48, 0);
            Player_RiseFromDive(play, this);
        } else {
            Player_SetActionFunc(play, this, Player_UpdateSwimIdle, 1);
            Player_ChangeAnimMorphToLastFrame(play, this,
                          (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ? &gPlayerAnim_003330 : &gPlayerAnim_0032E0);
        }
    }

    if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) || (this->actor.yDistToWater < this->ageProperties->diveWaterSurface)) {
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
        this->unk_840 = 0;
    } else {
        Audio_SetBaseFilter(0x20);
        if (this->unk_840 < 300) {
            this->unk_840++;
        }
    }

    if ((Player_JumpUpToLedge != this->actionFunc) && (Player_ClimbOntoLedge != this->actionFunc)) {
        if (this->ageProperties->diveWaterSurface < this->actor.yDistToWater) {
            if (!(this->stateFlags1 & PLAYER_STATE1_SWIMMING) ||
                (!((this->currentBoots == PLAYER_BOOTS_IRON) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) &&
                 (func_8084E30C != this->actionFunc) && (Player_SetupDrown != this->actionFunc) &&
                 (Player_UpdateSwimIdle != this->actionFunc) && (Player_Swim != this->actionFunc) &&
                 (func_8084DAB4 != this->actionFunc) && (func_8084DC48 != this->actionFunc) &&
                 (func_8084E1EC != this->actionFunc) && (Player_SpawnSwimming != this->actionFunc))) {
                func_8083D36C(play, this);
                return;
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_SWIMMING) && (this->actor.yDistToWater < this->ageProperties->unk_24)) {
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

    if (Player_IsFloorSinkingSand(sFloorSpecialProperty)) {
        unsinkSpeed = fabsf(this->linearVelocity) * 20.0f;
        sinkSpeed = 0.0f;

        if (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_SHALLOW_SAND) {
            if (this->sinkingOffsetY > 1300.0f) {
                maxSinkSpeed = this->sinkingOffsetY;
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
            } else if ((sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_QUICKSAND_NO_HORSE) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                unsinkSpeed = 0;
            }
        }

        if (this->currentBoots != PLAYER_BOOTS_HOVER) {
            sinkSpeed = (maxSinkSpeed - this->sinkingOffsetY) * 0.02f;
            sinkSpeed = CLAMP(sinkSpeed, 0.0f, 300.0f);
            if (this->currentBoots == PLAYER_BOOTS_IRON) {
                sinkSpeed += sinkSpeed;
            }
        }

        this->sinkingOffsetY += sinkSpeed - unsinkSpeed;
        this->sinkingOffsetY = CLAMP(this->sinkingOffsetY, 0.0f, maxSinkSpeed);

        this->actor.gravity -= this->sinkingOffsetY * 0.004f;
    } else {
        this->sinkingOffsetY = 0.0f;
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

    if (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_LOOK_UP) {
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

    if (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this) && (this->linearVelocity > 5.0f)) {
        targetPitch = this->linearVelocity * 200.0f;
        targetRoll = (s16)(this->currentYaw - this->actor.shape.rot.y) * this->linearVelocity * 0.1f;
        targetPitch = CLAMP(targetPitch, -4000, 4000);
        targetRoll = CLAMP(-targetRoll, -4000, 4000);
        Math_ScaledStepToS(&this->upperBodyRot.x, targetPitch, 900);
        this->headRot.x = -(f32)this->upperBodyRot.x * 0.5f;
        Math_ScaledStepToS(&this->headRot.z, targetRoll, 300);
        Math_ScaledStepToS(&this->upperBodyRot.z, targetRoll, 200);
        this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Z | PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Z | PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_X;
    } else {
        Player_SetLookAngle(this, play);
    }
}

void func_8083DF68(Player* this, f32 targetVelocity, s16 targetYaw) {
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

static struct_80854578 D_80854578[] = {
    { &gPlayerAnim_003398, 35.17f, 6.6099997f },
    { &gPlayerAnim_0033A8, -34.16f, 7.91f },
};

s32 Player_SetupMountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 unk_04;
    f32 unk_08;
    f32 sp38;
    f32 sp34;
    s32 temp;

    if ((rideActor != NULL) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        sp38 = Math_CosS(rideActor->actor.shape.rot.y);
        sp34 = Math_SinS(rideActor->actor.shape.rot.y);

        Player_SetupMiniCsFunc(play, this, Player_SetupRideHorse);

        this->stateFlags1 |= PLAYER_STATE1_RIDING_HORSE;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_WATER;

        if (this->mountSide < 0) {
            temp = 0;
        } else {
            temp = 1;
        }

        unk_04 = D_80854578[temp].unk_04;
        unk_08 = D_80854578[temp].unk_08;
        this->actor.world.pos.x =
            rideActor->actor.world.pos.x + rideActor->riderPos.x + ((unk_04 * sp38) + (unk_08 * sp34));
        this->actor.world.pos.z =
            rideActor->actor.world.pos.z + rideActor->riderPos.z + ((unk_08 * sp38) - (unk_04 * sp34));

        this->unk_878 = rideActor->actor.world.pos.y - this->actor.world.pos.y;
        this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

        Actor_MountHorse(play, this, &rideActor->actor);
        Player_PlayAnimOnce(play, this, D_80854578[temp].anim);
        Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
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
    &gPlayerAnim_002EE0,
    &gPlayerAnim_0031D0,
};

s32 func_8083E318(PlayState* play, Player* this, CollisionPoly* floorPoly) {
    s32 pad;
    s16 playerVelYaw;
    Vec3f slopeNormal;
    s16 downwardSlopeYaw;
    f32 slopeSlowdownSpeed;
    f32 slopeSlowdownSpeedStep;
    s16 velYawToDownwardSlope;

    if (!Player_InBlockingCsMode(play, this) && (func_8084F390 != this->actionFunc) &&
        (SurfaceType_GetSlope(&play->colCtx, floorPoly, this->actor.floorBgId) == 1)) {

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
            Player_SetActionFunc(play, this, func_8084F390, 0);
            Player_ResetAttributesAndHeldActor(play, this);
            if (D_80853610 >= 0) {
                this->unk_84F = 1;
            }
            Player_ChangeAnimShortMorphLoop(play, this, sSlopeSlipAnims[this->unk_84F]);
            this->linearVelocity = sqrtf(SQ(this->actor.velocity.x) + SQ(this->actor.velocity.z));
            this->currentYaw = playerVelYaw;
            return true;
        }
    }

    return false;
}

// unknown data (unused), possibly four individual arrays separated by padding
static s16 D_80854598[] = {
    0xFFDB, 0x0871, 0xF831, // Vec3s?
    0x0000,
    0x0094, 0x0470, 0xF398, // Vec3s?
    0x0000,
    0xFFB5, 0x04A9, 0x0C9F, // Vec3s?
    0x0000,
    0x0801, 0x0402,
};

void Player_GetItem(PlayState* play, Player* this, GetItemEntry* giEntry) {
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

                if ((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) || (play->sceneNum == SCENE_BOWLING)) {
                    Player_DetatchHeldActor(play, this);
                    Player_LoadGetItemObject(this, giEntry->objectId);

                    if (!(this->stateFlags2 & PLAYER_STATE2_DIVING) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                        Player_SetupMiniCsFunc(play, this, func_8083A434);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002788);
                        Player_SetCameraTurnAround(play, 9);
                    }

                    this->stateFlags1 |= PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
                    Player_ClearAttentionModeAndStopMoving(this);
                    return 1;
                }

                Player_GetItem(play, this, giEntry);
                this->getItemId = GI_NONE;
            }
        } else if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) && !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) &&
                   !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
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

                Player_SetupMiniCsFunc(play, this, func_8083A434);
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
                                  PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                      PLAYER_ANIMMOVEFLAGS_7 | PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
                    chest->unk_1F4 = 1;
                    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_SLOW_CHEST_CS);
                } else {
                    Player_PlayAnimOnce(play, this, &gPlayerAnim_002DF8);
                    chest->unk_1F4 = -1;
                }

                return 1;
            }

            if ((this->heldActor == NULL) || Player_HoldsHookshot(this)) {
                if ((interactedActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                    s32 actionParam = this->itemActionParam;

                    this->itemActionParam = PLAYER_AP_NONE;
                    this->modelAnimType = PLAYER_ANIMTYPE_DEFAULT;
                    this->heldItemActionParam = this->itemActionParam;
                    Player_SetupMiniCsFunc(play, this, Player_SetupHoldActor);

                    if (actionParam == PLAYER_AP_SWORD_MASTER) {
                        this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_AP_LAST_USED);
                        Player_ChangeItem(play, this, PLAYER_AP_LAST_USED);
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
    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_THROWING_OBJECT, this->modelAnimType));
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
                Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_PUTTING_DOWN_OBJECT, this->modelAnimType));
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

            if ((isClimbableWall != 0) || (wallFlags & 2) || func_80041E4C(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) {
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

                    Player_SetupMiniCsFunc(play, this, Player_SetupClimbDownFromLedge);
                    this->stateFlags1 |= PLAYER_STATE1_CLIMBING;
                    this->stateFlags1 &= ~PLAYER_STATE1_SWIMMING;

                    if ((isClimbableWall != 0) || (wallFlags & 2)) {
                        if ((this->unk_84F = isClimbableWall) != 0) {
                            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                                climbAnim = &gPlayerAnim_002D80;
                            } else {
                                climbAnim = &gPlayerAnim_002D68;
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
                                  PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                      PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);

                    return 1;
                }
            }
        }
    }

    return 0;
}

void Player_SetupEndClimb(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetActionFuncPreserveMoveFlags(play, this, func_8084C5F8, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, (4.0f / 3.0f));
}

s32 func_8083F0C8(Player* this, PlayState* play, u32 arg2) {
    CollisionPoly* wallPoly;
    Vec3f sp50[3];
    f32 sp4C;
    f32 phi_f2;
    f32 sp44;
    f32 phi_f12;
    s32 i;

    if (!LINK_IS_ADULT && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING) && (arg2 & 0x30)) {
        wallPoly = this->actor.wallPoly;
        CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, sp50);

        sp4C = phi_f2 = sp50[0].x;
        sp44 = phi_f12 = sp50[0].z;
        for (i = 1; i < 3; i++) {
            if (sp4C > sp50[i].x) {
                sp4C = sp50[i].x;
            } else if (phi_f2 < sp50[i].x) {
                phi_f2 = sp50[i].x;
            }

            if (sp44 > sp50[i].z) {
                sp44 = sp50[i].z;
            } else if (phi_f12 < sp50[i].z) {
                phi_f12 = sp50[i].z;
            }
        }

        sp4C = (sp4C + phi_f2) * 0.5f;
        sp44 = (sp44 + phi_f12) * 0.5f;

        phi_f2 = ((this->actor.world.pos.x - sp4C) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
                 ((this->actor.world.pos.z - sp44) * COLPOLY_GET_NORMAL(wallPoly->normal.x));

        if (fabsf(phi_f2) < 8.0f) {
            this->stateFlags2 |= PLAYER_STATE2_CAN_ENTER_CRAWLSPACE;

            if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 sp30 = this->wallDistance;

                Player_SetupMiniCsFunc(play, this, func_8083A40C);
                this->stateFlags2 |= PLAYER_STATE2_INSIDE_CRAWLSPACE;
                this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
                this->actor.world.pos.x = sp4C + (sp30 * wallPolyNormalX);
                this->actor.world.pos.z = sp44 + (sp30 * wallPolyNormalZ);
                Player_ClearAttentionModeAndStopMoving(this);
                this->actor.prevPos = this->actor.world.pos;
                Player_PlayAnimOnce(play, this, &gPlayerAnim_002708);
                Player_SetupAnimMovement(play, this,
                              PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                  PLAYER_ANIMMOVEFLAGS_7);

                return 1;
            }
        }
    }

    return 0;
}

s32 Player_SetPositionAndYawOnClimbWall(PlayState* play, Player* this, f32 yOffset, f32 xzDistToWall, f32 xzCheckBScale, f32 xzCheckAScale) {
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

    if (BgCheck_EntityLineTest1(&play->colCtx, &checkPosA, &checkPosB, &posResult, &this->actor.wallPoly, true, false, false, true,
                                &wallBgId)) {

        wallPoly = this->actor.wallPoly;

        this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
        this->actor.wallBgId = wallBgId;

        sTouchedWallFlags = func_80041DB8(&play->colCtx, wallPoly, wallBgId);

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
    return Player_SetPositionAndYawOnClimbWall(play, this, 26.0f, this->ageProperties->wallCheckRadius + 5.0f, 30.0f, 0.0f);
}

s32 func_8083F570(Player* this, PlayState* play) {
    s16 temp;

    if ((this->linearVelocity != 0.0f) && (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) && (sTouchedWallFlags & 0x30)) {

        temp = this->actor.shape.rot.y - this->actor.wallYaw;
        if (this->linearVelocity < 0.0f) {
            temp += 0x8000;
        }

        if (ABS(temp) > 0x4000) {
            Player_SetActionFunc(play, this, func_8084C81C, 0);

            if (this->linearVelocity > 0.0f) {
                this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
                Player_PlayAnimOnce(play, this, &gPlayerAnim_002700);
                Player_SetupAnimMovement(play, this,
                              PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                  PLAYER_ANIMMOVEFLAGS_7);
                OnePointCutscene_Init(play, 9601, 999, NULL, CAM_ID_MAIN);
            } else {
                this->actor.shape.rot.y = this->actor.wallYaw;
                LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_002708, -1.0f,
                                     Animation_GetLastFrame(&gPlayerAnim_002708), 0.0f, ANIMMODE_ONCE, 0.0f);
                Player_SetupAnimMovement(play, this,
                              PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                  PLAYER_ANIMMOVEFLAGS_7);
                OnePointCutscene_Init(play, 9602, 999, NULL, CAM_ID_MAIN);
            }

            this->currentYaw = this->actor.shape.rot.y;
            Player_StopMovement(this);

            return 1;
        }
    }

    return 0;
}

void Player_StartGrabPushPullWall(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    if (!Player_SetupMiniCsFunc(play, this, Player_SetupGrabPushPullWall)) {
        Player_SetActionFunc(play, this, Player_GrabPushPullWall, 0);
    }

    Player_PlayAnimOnce(play, this, anim);
    Player_ClearAttentionModeAndStopMoving(this);

    this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
}

s32 Player_SetupStartGrabPushPullWall(Player* this, PlayState* play) {
    DynaPolyActor* wallPolyActor;

    if (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        (sYawToTouchedWall < DEG_TO_BINANG(67.5f))) {

        if (((this->linearVelocity > 0.0f) && Player_SetupClimbWallOrLadder(this, play, sTouchedWallFlags)) ||
            func_8083F0C8(this, play, sTouchedWallFlags)) {
            return 1;
        }

        if (!Player_IsSwimming(this) && ((this->linearVelocity == 0.0f) || !(this->stateFlags2 & PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL)) &&
            (sTouchedWallFlags & 0x40) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->wallHeight >= 39.0f)) {

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

                Player_StartGrabPushPullWall(this, &gPlayerAnim_0030F8, play);

                return 1;
            }
        }
    }

    return 0;
}

s32 Player_SetupPushPullWallIdle(PlayState* play, Player* this) {
    if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) || CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))) {
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
    Player_PlayAnimOnce(play, this, &gPlayerAnim_003100);
    this->stateFlags2 &= ~PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    return 1;
}

void Player_SetupPushWall(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_PushWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    Player_PlayAnimOnce(play, this, &gPlayerAnim_0030F0);
}

void Player_SetupPullWall(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_PullWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_PULL_OBJECT, this->modelAnimType));
}

void func_8083FB7C(Player* this, PlayState* play) {
    this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING | PLAYER_STATE1_SWIMMING);
    Player_SetupFallFromLedge(this, play);
    this->linearVelocity = -0.4f;
}

s32 func_8083FBC0(Player* this, PlayState* play) {
    if (!CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((sTouchedWallFlags & 8) || (sTouchedWallFlags & 2) ||
         func_80041E4C(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId))) {
        return 0;
    }

    func_8083FB7C(this, play);
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

    if ((Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) && (this->targetActor == NULL)) {
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

s32 func_80840058(Player* this, f32* arg1, s16* arg2, PlayState* play) {
    Player_SetLookAngle(this, play);

    if ((*arg1 != 0.0f) || (ABS(this->unk_87C) > 400)) {
        s16 temp1 = *arg2 - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        u16 temp2 = (ABS(temp1) - 0x2000) & 0xFFFF;

        if ((temp2 < 0x4000) || (this->unk_87C != 0)) {
            return -1;
        } else {
            return 1;
        }
    }

    return 0;
}

void func_80840138(Player* this, f32 arg1, s16 arg2) {
    s16 temp = arg2 - this->actor.shape.rot.y;

    if (arg1 > 0.0f) {
        if (temp < 0) {
            this->leftRightBlendWeightTarget = 0.0f;
        } else {
            this->leftRightBlendWeightTarget = 1.0f;
        }
    }

    Math_StepToF(&this->leftRightBlendWeight, this->leftRightBlendWeightTarget, 0.3f);
}

void func_808401B0(PlayState* play, Player* this) {
    LinkAnimation_BlendToJoint(play, &this->skelAnime, Player_GetFightingRightAnim(this), this->walkFrame, Player_GetFightingLeftAnim(this),
                               this->walkFrame, this->leftRightBlendWeight, this->blendTable);
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
    } else if (Player_CheckWalkFrame(this->walkFrame, frameStep, 29.0f, 10.0f) || Player_CheckWalkFrame(this->walkFrame, frameStep, 29.0f, 24.0f)) {
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
            this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
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
        if (!Player_SetupStartUnfriendlyZTargeting(this) && (!Player_IsFriendlyZTargeting(this) || (func_80834B5C != this->upperActionFunc))) {
            Player_SetupEndUnfriendlyZTarget(this, play);
            return;
        }

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

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
        func_80840138(this, targetVelocity, targetYaw);

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

        if (func_80834B5C == this->upperActionFunc) {
            Player_SetupUnfriendlyZTarget(this, play);
            return;
        }

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

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
                    if (((rand != 0) && (rand != 3)) || ((this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) &&
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

    LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f) * sWaterSpeedScale, 0.0f, Animation_GetLastFrame(anim),
                         ANIMMODE_ONCE, -6.0f);
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

            Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

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

    if (this->skelAnime.animation == &gPlayerAnim_0026E8) {
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

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
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
    f32 temp1;
    f32 temp2;

    if (this->unk_864 < 1.0f) {
        temp1 = R_UPDATE_RATE * 0.5f;
        Player_SetupWalkSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_BACKWALKING, this->modelAnimType),
                                  this->walkFrame);
        this->unk_864 += 1 * temp1;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }
        temp1 = this->unk_864;
    } else {
        temp2 = this->linearVelocity - (REG(48) / 100.0f);
        if (temp2 < 0.0f) {
            temp1 = 1.0f;
            Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            LinkAnimation_LoadToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_BACKWALKING, this->modelAnimType),
                                      this->walkFrame);
        } else {
            temp1 = (REG(37) / 1000.0f) * temp2;
            if (temp1 < 1.0f) {
                Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            } else {
                temp1 = 1.0f;
                Player_SetupWalkSfx(this, 1.2f + ((REG(38) / 1000.0f) * temp2));
            }
            LinkAnimation_LoadToMorph(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_BACKWALKING, this->modelAnimType),
                                      this->walkFrame);
            LinkAnimation_LoadToJoint(play, &this->skelAnime, &gPlayerAnim_002DD0, this->walkFrame * (16.0f / 29.0f));
        }
    }

    if (temp1 < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - temp1);
    }
}

void func_8084140C(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, func_8084170C, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, &gPlayerAnim_002DA0);
}

s32 func_80841458(Player* this, f32* arg1, s16* arg2, PlayState* play) {
    if (this->linearVelocity > 6.0f) {
        func_8084140C(this, play);
        return 1;
    }

    if (*arg1 != 0.0f) {
        if (Player_StepLinearVelocityToZero(this)) {
            *arg1 = 0.0f;
            *arg2 = this->currentYaw;
        } else {
            return 1;
        }
    }

    return 0;
}

void Player_FriendlyBackwalk(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 sp2C;
    s16 targetYawDiff;

    Player_UpdateBackwalk(this, play);

    if (!Player_SetupSubAction(play, this, sFriendlyBackwalkSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupZTargetRunning(this, play, this->currentYaw);
            return;
        }

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        sp2C = Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play);

        if (sp2C >= 0) {
            if (!func_80841458(this, &targetVelocity, &targetYaw, play)) {
                if (sp2C != 0) {
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

void func_808416C0(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, func_808417FC, 1);
    Player_PlayAnimOnce(play, this, &gPlayerAnim_002DA8);
}

void func_8084170C(Player* this, PlayState* play) {
    s32 sp34;
    f32 targetVelocity;
    s16 targetYaw;

    sp34 = LinkAnimation_Update(play, &this->skelAnime);
    Player_StepLinearVelocityToZero(this);

    if (!Player_SetupSubAction(play, this, sFriendlyBackwalkSubActions, 1)) {
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;

            if (Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play) > 0) {
                Player_SetupRun(this, play);
            } else if ((targetVelocity != 0.0f) || (sp34 != 0)) {
                func_808416C0(this, play);
            }
        }
    }
}

void func_808417FC(Player* this, PlayState* play) {
    s32 sp1C;

    sp1C = LinkAnimation_Update(play, &this->skelAnime);

    if (!Player_SetupSubAction(play, this, sFriendlyBackwalkSubActions, 1)) {
        if (sp1C != 0) {
            Player_SetupFriendlyZTargetingStandStill(this, play);
        }
    }
}

void func_80841860(PlayState* play, Player* this) {
    f32 frame;
    LinkAnimationHeader* sp38 = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SIDEWALKING_LEFT, this->modelAnimType);
    LinkAnimationHeader* sp34 = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SIDEWALKING_RIGHT, this->modelAnimType);

    this->skelAnime.animation = sp38;

    Player_SetupWalkSfx(this, (REG(30) / 1000.0f) + ((REG(32) / 1000.0f) * this->linearVelocity));

    frame = this->walkFrame * (16.0f / 29.0f);
    LinkAnimation_BlendToJoint(play, &this->skelAnime, sp34, frame, sp38, frame, this->leftRightBlendWeight, this->blendTable);
}

void Player_Sidewalk(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;
    s32 moveDir;
    s16 targetYawDiff;
    s32 absTargetYawDiff;

    func_80841860(play, this);

    if (!Player_SetupSubAction(play, this, sSidewalkSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

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

        func_80840138(this, targetVelocity, targetYaw);

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

    Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

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

void func_80841CC4(Player* this, s32 arg1, PlayState* play) {
    LinkAnimationHeader* anim;
    s16 target;
    f32 rate;

    if (ABS(D_80853610) < 3640) {
        target = 0;
    } else {
        target = CLAMP(D_80853610, -10922, 10922);
    }

    Math_ScaledStepToS(&this->unk_89C, target, 400);

    if ((this->modelAnimType == PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON) || ((this->unk_89C == 0) && (this->sinkingOffsetY <= 0.0f))) {
        if (arg1 == 0) {
            LinkAnimation_LoadToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_WALKING, this->modelAnimType),
                                      this->walkFrame);
        } else {
            LinkAnimation_LoadToMorph(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_WALKING, this->modelAnimType),
                                      this->walkFrame);
        }
        return;
    }

    if (this->unk_89C != 0) {
        rate = this->unk_89C / 10922.0f;
    } else {
        rate = this->sinkingOffsetY * 0.0006f;
    }

    rate *= fabsf(this->linearVelocity) * 0.5f;

    if (rate > 1.0f) {
        rate = 1.0f;
    }

    if (rate < 0.0f) {
        anim = &gPlayerAnim_002E48;
        rate = -rate;
    } else {
        anim = &gPlayerAnim_002E90;
    }

    if (arg1 == 0) {
        LinkAnimation_BlendToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_WALKING, this->modelAnimType),
                                   this->walkFrame, anim, this->walkFrame, rate, this->blendTable);
    } else {
        LinkAnimation_BlendToMorph(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_WALKING, this->modelAnimType),
                                   this->walkFrame, anim, this->walkFrame, rate, this->blendTable);
    }
}

void func_80841EE4(Player* this, PlayState* play) {
    f32 temp1;
    f32 temp2;

    if (this->unk_864 < 1.0f) {
        temp1 = R_UPDATE_RATE * 0.5f;

        Player_SetupWalkSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(play, &this->skelAnime, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_WALKING, this->modelAnimType),
                                  this->walkFrame);

        this->unk_864 += 1 * temp1;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }

        temp1 = this->unk_864;
    } else {
        temp2 = this->linearVelocity - (REG(48) / 100.0f);

        if (temp2 < 0.0f) {
            temp1 = 1.0f;
            Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));

            func_80841CC4(this, 0, play);
        } else {
            temp1 = (REG(37) / 1000.0f) * temp2;
            if (temp1 < 1.0f) {
                Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            } else {
                temp1 = 1.0f;
                Player_SetupWalkSfx(this, 1.2f + ((REG(38) / 1000.0f) * temp2));
            }

            func_80841CC4(this, 1, play);

            LinkAnimation_LoadToJoint(play, &this->skelAnime, Player_GetRunningAnim(this), this->walkFrame * (20.0f / 29.0f));
        }
    }

    if (temp1 < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - temp1);
    }
}

void Player_Run(Player* this, PlayState* play) {
    f32 targetVelocity;
    s16 targetYaw;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    func_80841EE4(this, play);

    if (!Player_SetupSubAction(play, this, sRunSubActions, 1)) {
        if (Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

        if (!func_8083C484(this, &targetVelocity, &targetYaw)) {
            func_8083DF68(this, targetVelocity, targetYaw);
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
    func_80841EE4(this, play);

    if (!Player_SetupSubAction(play, this, sTargetRunSubActions, 1)) {
        if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupRun(this, play);
            return;
        }

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if (!func_8083C484(this, &targetVelocity, &targetYaw)) {
            if ((Player_IsFriendlyZTargeting(this) && (targetVelocity != 0.0f) && (Player_GetFriendlyZTargetingMoveDirection(this, &targetVelocity, &targetYaw, play) <= 0)) ||
                (!Player_IsFriendlyZTargeting(this) && (Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw) <= 0))) {
                Player_SetupStandingStillType(this, play);
                return;
            }

            func_8083DF68(this, targetVelocity, targetYaw);
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

        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        if ((this->skelAnime.morphWeight == 0.0f) && (this->skelAnime.curFrame > 5.0f)) {
            Player_StepLinearVelocityToZero(this);

            if ((this->skelAnime.curFrame > 10.0f) && (Player_GetUnfriendlyZTargetMoveDirection(this, targetVelocity, targetYaw) < 0)) {
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
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

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

void func_8084260C(Vec3f* src, Vec3f* dest, f32 arg2, f32 arg3, f32 arg4) {
    dest->x = (Rand_ZeroOne() * arg3) + src->x;
    dest->y = (Rand_ZeroOne() * arg4) + (src->y + arg2);
    dest->z = (Rand_ZeroOne() * arg3) + src->z;
}

static Vec3f D_808545B4 = { 0.0f, 0.0f, 0.0f };
static Vec3f D_808545C0 = { 0.0f, 0.0f, 0.0f };

s32 func_8084269C(PlayState* play, Player* this) {
    Vec3f sp2C;

    if ((this->moveSfxType == 0) || (this->moveSfxType == 1)) {
        func_8084260C(&this->actor.shape.feetPos[FOOT_LEFT], &sp2C,
                      this->actor.floorHeight - this->actor.shape.feetPos[FOOT_LEFT].y, 7.0f, 5.0f);
        func_800286CC(play, &sp2C, &D_808545B4, &D_808545C0, 50, 30);
        func_8084260C(&this->actor.shape.feetPos[FOOT_RIGHT], &sp2C,
                      this->actor.floorHeight - this->actor.shape.feetPos[FOOT_RIGHT].y, 7.0f, 5.0f);
        func_800286CC(play, &this->actor.shape.feetPos[FOOT_RIGHT], &D_808545B4, &D_808545C0, 50, 30);
        return 1;
    }

    return 0;
}

void func_8084279C(Player* this, PlayState* play) {
    Player_LoopAnimContinuously(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_CHECKING_OR_SPEAKING, this->modelAnimType));

    if (DECR(this->genericTimer) == 0) {
        if (!Player_SetupItemCutsceneOrCUp(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_CHECKING_OR_SPEAKING, this->modelAnimType), play);
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
    if (!Player_IsChildWithHylianShield(this) && (Player_GetMeleeWeaponHeld(this) != PLAYER_MELEEWEAPON_NONE) && sUsingItemAlreadyInHand) {
        Player_PlayAnimOnce(play, this, &gPlayerAnim_002EC8);
        this->unk_84F = 1;
        this->meleeWeaponAnimation = PLAYER_MWA_STAB_1H;
        this->currentYaw = this->actor.shape.rot.y + this->upperBodyRot.y;
        return 1;
    }

    return 0;
}

// Probably want to rename this one...
s32 Player_IsBusy(Player* this, PlayState* play) {
    return Player_SetupItemCutsceneOrCUp(this, play) || Player_SetupSpeakOrCheck(this, play) || Player_SetupGetItemOrHoldBehavior(this, play);
}

void Player_SetQuake(PlayState* play, s32 speed, s32 y, s32 countdown) {
    s32 quakeIdx = Quake_Add(Play_GetCamera(play, CAM_ID_MAIN), 3);

    Quake_SetSpeed(quakeIdx, speed);
    Quake_SetQuakeValues(quakeIdx, y, 0, 0, 0);
    Quake_SetCountdown(quakeIdx, countdown);
}

void Player_SetupHammerHit(PlayState* play, Player* this) {
    Player_SetQuake(play, 27767, 7, 20);
    play->actorCtx.unk_02 = 4;
    Player_RequestRumble(this, 255, 20, 150, 0);
    func_8002F7DC(&this->actor, NA_SE_IT_HAMMER_HIT);
}

void func_80842A88(PlayState* play, Player* this) {
    Inventory_ChangeAmmo(ITEM_STICK, -1);
    Player_UseItem(play, this, ITEM_NONE);
}

s32 func_80842AC4(PlayState* play, Player* this) {
    if ((this->heldItemActionParam == PLAYER_AP_STICK) && (this->dekuStickLength > 0.5f)) {
        if (AMMO(ITEM_STICK) != 0) {
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
    if (this->heldItemActionParam == PLAYER_AP_SWORD_BGS) {
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
    &gPlayerAnim_002B10,
    &gPlayerAnim_002B20,
    &gPlayerAnim_002B08,
    &gPlayerAnim_002B18,
};

void Player_SetupMeleeWeaponRebound(PlayState* play, Player* this) {
    s32 pad;
    s32 sp28;

    if (func_80843188 != this->actionFunc) {
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
    s32 sp48;

    if (this->isMeleeWeaponAttacking > 0) {
        if (this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) {
            if (!(this->meleeWeaponQuads[0].base.atFlags & AT_BOUNCED) &&
                !(this->meleeWeaponQuads[1].base.atFlags & AT_BOUNCED)) {
                if (this->skelAnime.curFrame >= 2.0f) {

                    dist = Math_Vec3f_DistXYZAndStoreDiff(&this->meleeWeaponInfo[0].tip,
                                                                         &this->meleeWeaponInfo[0].base, &tipBaseDiff);
                    if (dist != 0.0f) {
                        dist = (dist + 10.0f) / dist;
                    }

                    checkPos.x = this->meleeWeaponInfo[0].tip.x + (tipBaseDiff.x * dist);
                    checkPos.y = this->meleeWeaponInfo[0].tip.y + (tipBaseDiff.y * dist);
                    checkPos.z = this->meleeWeaponInfo[0].tip.z + (tipBaseDiff.z * dist);

                    if (BgCheck_EntityLineTest1(&play->colCtx, &checkPos, &this->meleeWeaponInfo[0].tip, &weaponHitPos, &colPoly, true,
                                                false, false, true, &colPolyBgId) &&
                        !SurfaceType_IsIgnoredByEntities(&play->colCtx, colPoly, colPolyBgId) &&
                        (func_80041D4C(&play->colCtx, colPoly, colPolyBgId) != BGCHECK_FLOORSPECIALPROPERTY_NO_FALL_DAMAGE) &&
                        (func_8002F9EC(play, &this->actor, colPoly, colPolyBgId, &weaponHitPos) == 0)) {

                        if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
                            Player_SetFreezeFlashTimer(play);
                            Player_SetupHammerHit(play, this);
                            Player_SetupMeleeWeaponRebound(play, this);
                            return 1;
                        }

                        if (this->linearVelocity >= 0.0f) {
                            sp48 = SurfaceType_GetMoveSfx(&play->colCtx, colPoly, colPolyBgId);

                            if (sp48 == 0xA) {
                                CollisionCheck_SpawnShieldParticlesWood(play, &weaponHitPos, &this->actor.projectedPos);
                            } else {
                                CollisionCheck_SpawnShieldParticles(play, &weaponHitPos);
                                if (sp48 == 0xB) {
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
            if (this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) {
                Actor* at = this->meleeWeaponQuads[temp1 ? 1 : 0].base.at;

                if ((at != NULL) && (at->id != ACTOR_EN_KANBAN)) {
                    Player_SetFreezeFlashTimer(play);
                }
            }

            if ((func_80842AC4(play, this) == 0) && (this->heldItemActionParam != PLAYER_AP_HAMMER)) {
                func_80842B7C(play, this);

                if (this->actor.colChkInfo.atHitEffect == PLAYER_HITEFFECTAT_ELECTRIC) {
                    this->actor.colChkInfo.damage = 8;
                    Player_SetupDamage(play, this, PLAYER_DMGREACTION_ELECTRIC_SHOCKED, 0.0f, 0.0f, this->actor.shape.rot.y, 20);
                    return 1;
                }
            }
        }
    }

    return 0;
}

void func_80843188(Player* this, PlayState* play) {
    f32 sp54;
    f32 sp50;
    s16 sp4E;
    s16 sp4C;
    s16 sp4A;
    s16 sp48;
    s16 sp46;
    f32 sp40;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_IsChildWithHylianShield(this)) {
            Player_PlayAnimLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_DEFENDING, this->modelAnimType));
        }
        this->genericTimer = 1;
        this->unk_84F = 0;
    }

    if (!Player_IsChildWithHylianShield(this)) {
        this->stateFlags1 |= PLAYER_STATE1_SHIELDING;
        Player_SetupCurrentUpperAction(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_SHIELDING;
    }

    Player_StepLinearVelocityToZero(this);

    if (this->genericTimer != 0) {
        sp54 = sControlInput->rel.stick_y * 100;
        sp50 = sControlInput->rel.stick_x * -120;
        sp4E = this->actor.shape.rot.y - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

        sp40 = Math_CosS(sp4E);
        sp4C = (Math_SinS(sp4E) * sp50) + (sp54 * sp40);
        sp40 = Math_CosS(sp4E);
        sp4A = (sp50 * sp40) - (Math_SinS(sp4E) * sp54);

        if (sp4C > 3500) {
            sp4C = 3500;
        }

        sp48 = ABS(sp4C - this->actor.focus.rot.x) * 0.25f;
        if (sp48 < 100) {
            sp48 = 100;
        }

        sp46 = ABS(sp4A - this->upperBodyRot.y) * 0.25f;
        if (sp46 < 50) {
            sp46 = 50;
        }

        Math_ScaledStepToS(&this->actor.focus.rot.x, sp4C, sp48);
        this->upperBodyRot.x = this->actor.focus.rot.x;
        Math_ScaledStepToS(&this->upperBodyRot.y, sp4A, sp46);

        if (this->unk_84F != 0) {
            if (!func_80842DF4(play, this)) {
                if (this->skelAnime.curFrame < 2.0f) {
                    Player_MeleeAttack(this, 1);
                }
            } else {
                this->genericTimer = 1;
                this->unk_84F = 0;
            }
        } else if (!Player_IsBusy(this, play)) {
            if (Player_SetupDefend(this, play)) {
                Player_AttackWhileDefending(this, play);
            } else {
                this->stateFlags1 &= ~PLAYER_STATE1_SHIELDING;
                Player_InactivateMeleeWeapon(this);

                if (Player_IsChildWithHylianShield(this)) {
                    Player_SetupReturnToStandStill(this, play);
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_002400, 1.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_002400), 0.0f, ANIMMODE_ONCE, 0.0f);
                    Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE);
                } else {
                    if (this->itemActionParam < 0) {
                        Player_SetHeldItem(this);
                    }
                    Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_DEFENDING, this->modelAnimType), play);
                }

                func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_REMOVE);
                return;
            }
        } else {
            return;
        }
    }

    this->stateFlags1 |= PLAYER_STATE1_SHIELDING;
    Player_SetModelsForHoldingShield(this);

    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
}

void func_808435C4(Player* this, PlayState* play) {
    s32 temp;
    LinkAnimationHeader* anim;
    f32 frames;

    Player_StepLinearVelocityToZero(this);

    if (this->unk_84F == 0) {
        D_808535E0 = Player_SetupCurrentUpperAction(this, play);
        if ((func_80834B5C == this->upperActionFunc) || (Player_SetupInterruptAction(play, this, &this->skelAnimeUpper, 4.0f) > 0)) {
            Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
        }
    } else {
        temp = Player_SetupInterruptAction(play, this, &this->skelAnime, 4.0f);
        if ((temp != 0) && ((temp > 0) || LinkAnimation_Update(play, &this->skelAnime))) {
            Player_SetActionFunc(play, this, func_80843188, 1);
            this->stateFlags1 |= PLAYER_STATE1_SHIELDING;
            Player_SetModelsForHoldingShield(this);
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_START_DEFENDING, this->modelAnimType);
            frames = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frames, frames, ANIMMODE_ONCE, 0.0f);
        }
    }
}

void func_8084370C(Player* this, PlayState* play) {
    s32 sp1C;

    Player_StepLinearVelocityToZero(this);

    sp1C = Player_SetupInterruptAction(play, this, &this->skelAnime, 16.0f);
    if ((sp1C != 0) && (LinkAnimation_Update(play, &this->skelAnime) || (sp1C > 0))) {
        Player_SetupStandingStillType(this, play);
    }
}

void func_8084377C(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

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
                Player_SetActionFunc(play, this, func_80843954, 0);
                this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;
            }

            Player_PlayAnimOnce(play, this,
                          (this->currentYaw != this->actor.shape.rot.y) ? &gPlayerAnim_002F60 : &gPlayerAnim_002DB8);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
        }
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
    }
}

void func_80843954(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    Player_RoundUpInvincibilityTimer(this);

    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime) && (this->linearVelocity == 0.0f)) {
        if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
            this->genericTimer++;
        } else {
            Player_SetActionFunc(play, this, func_80843A38, 0);
            this->stateFlags1 |= PLAYER_STATE1_TAKING_DAMAGE;
        }

        Player_PlayAnimOnceSlowed(play, this,
                      (this->currentYaw != this->actor.shape.rot.y) ? &gPlayerAnim_002F68 : &gPlayerAnim_002DC0);
        this->currentYaw = this->actor.shape.rot.y;
    }
}

static PlayerAnimSfxEntry D_808545DC[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4014 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x401E },
};

void func_80843A38(Player* this, PlayState* play) {
    s32 sp24;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    Player_RoundUpInvincibilityTimer(this);

    if (this->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) {
        LinkAnimation_Update(play, &this->skelAnime);
    } else {
        sp24 = Player_SetupInterruptAction(play, this, &this->skelAnime, 16.0f);
        if ((sp24 != 0) && (LinkAnimation_Update(play, &this->skelAnime) || (sp24 > 0))) {
            Player_SetupStandingStillType(this, play);
        }
    }

    Player_PlayAnimSfx(this, D_808545DC);
}

static Vec3f sDeathReviveFairyPosOffset = { 0.0f, 0.0f, 5.0f };

void Player_EndDie(PlayState* play, Player* this) {
    if (this->genericTimer != 0) {
        if (this->genericTimer > 0) {
            this->genericTimer--;
            if (this->genericTimer == 0) {
                if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_003328, 1.0f, 0.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_003328), ANIMMODE_ONCE, -16.0f);
                } else {
                    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_002878, 1.0f, 99.0f,
                                         Animation_GetLastFrame(&gPlayerAnim_002878), ANIMMODE_ONCE, 0.0f);
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
            this->unk_A87 = 20;
            Player_SetupInvincibilityNoDamageFlash(this, -20);
            func_800F47FC();
        }
    } else if (this->unk_84F != 0) {
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
        if ((play->roomCtx.curRoom.behaviorType2 == ROOM_BEHAVIOR_TYPE2_3) || (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_VOID_ON_TOUCH) ||
            ((Player_GetHurtFloorType(sFloorSpecialProperty) >= PLAYER_HURTFLOORTYPE_DEFAULT) &&
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

    if (this->skelAnime.animation == &gPlayerAnim_002878) {
        Player_PlayAnimSfx(this, sDyingOrRevivingAnimSfx);
    } else if (this->skelAnime.animation == &gPlayerAnim_002F08) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 88.0f)) {
            Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
        }
    }
}

void func_80843E14(Player* this, u16 sfxId) {
    Player_PlayVoiceSfxForAge(this, sfxId);

    if ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_EN_RU1)) {
        Audio_PlayActorSound2(this->heldActor, NA_SE_VO_RT_FALL);
    }
}

static FallImpactInfo sFallImpactSfx[] = {
    { -8, 180, 40, 100, NA_SE_VO_LI_LAND_DAMAGE_S },
    { -16, 255, 140, 150, NA_SE_VO_LI_LAND_DAMAGE_S },
};

s32 func_80843E64(PlayState* play, Player* this) {
    s32 fallDistance;

    if ((sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_NO_FALL_DAMAGE) || (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_VOID_ON_TOUCH)) {
        fallDistance = 0;
    } else {
        fallDistance = this->fallDistance;
    }

    Math_StepToF(&this->linearVelocity, 0.0f, 1.0f);

    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);

    if (fallDistance >= 400) {
        s32 impactIndex;
        FallImpactInfo* impactInfo;

        if (this->fallDistance < 800) {
            impactIndex = 0;
        } else {
            impactIndex = 1;
        }

        impactInfo = &sFallImpactSfx[impactIndex];

        if (Player_SetupInflictDamage(play, impactInfo->damage)) {
            return -1;
        }

        Player_SetupInvincibility(this, 40);
        Player_SetQuake(play, 32967, 2, 30);
        Player_RequestRumble(this, impactInfo->unk_01, impactInfo->unk_02, impactInfo->unk_03, 0);
        func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
        Player_PlayVoiceSfxForAge(this, impactInfo->sfxId);

        return impactIndex + 1;
    }

    if (fallDistance > 200) {
        fallDistance *= 2;

        if (fallDistance > 255) {
            fallDistance = 255;
        }

        Player_RequestRumble(this, (u8)fallDistance, (u8)(fallDistance * 0.1f), (u8)fallDistance, 0);

        if (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_NO_FALL_DAMAGE) {
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

    Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

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

        if (((this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) && (this->unk_84F == 2)) || !Player_SetupMidairJumpSlash(this, play)) {
            if (this->actor.velocity.y < 0.0f) {
                if (this->genericTimer >= 0) {
                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_WALL) || (this->genericTimer == 0) ||
                        (this->fallDistance > 0)) {
                        if ((sPlayerYDistToFloor > 800.0f) || (this->stateFlags1 & PLAYER_STATE1_END_HOOKSHOT_MOVE)) {
                            func_80843E14(this, NA_SE_VO_LI_FALL_S);
                            this->stateFlags1 &= ~PLAYER_STATE1_END_HOOKSHOT_MOVE;
                        }

                        LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_003020, 1.0f, 0.0f, 0.0f,
                                             ANIMMODE_ONCE, 8.0f);
                        this->genericTimer = -1;
                    }
                } else {
                    if ((this->genericTimer == -1) && (this->fallDistance > 120.0f) && (sPlayerYDistToFloor > 280.0f)) {
                        this->genericTimer = -2;
                        func_80843E14(this, NA_SE_VO_LI_FALL_L);
                    }

                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
                        !(this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) &&
                        !(this->stateFlags1 & (PLAYER_STATE1_HOLDING_ACTOR | PLAYER_STATE1_SWIMMING)) && (this->linearVelocity > 0.0f)) {
                        if ((this->wallHeight >= 150.0f) && (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0)) {
                            Player_SetupClimbWallOrLadder(this, play, sTouchedWallFlags);
                        } else if ((this->touchedWallJumpType >= PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP) && (this->wallHeight < 150.0f) &&
                                   (((this->actor.world.pos.y - this->actor.floorHeight) + this->wallHeight) >
                                    (70.0f * this->ageProperties->translationScale))) {
                            AnimationContext_DisableQueue(play);
                            if (this->stateFlags1 & PLAYER_STATE1_END_HOOKSHOT_MOVE) {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HOOKSHOT_HANG);
                            } else {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
                            }
                            this->actor.world.pos.y += this->wallHeight;
                            Player_SetupGrabLedge(play, this, this->actor.wallPoly, this->wallDistance,
                                          GET_PLAYER_ANIM(PLAYER_ANIMGROUP_HANGING_FROM_LEDGE, this->modelAnimType));
                            this->actor.shape.rot.y = this->currentYaw += 0x8000;
                            this->stateFlags1 |= PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP;
                        }
                    }
                }
            }
        }
    } else {
        LinkAnimationHeader* anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_TALL_JUMP_LANDING, this->modelAnimType);
        s32 sp3C;

        if (this->stateFlags2 & PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING) {
            if (Player_IsUnfriendlyZTargeting(this)) {
                anim = sManualJumpAnims[this->relativeStickInput][2];
            } else {
                anim = sManualJumpAnims[this->relativeStickInput][1];
            }
        } else if (this->skelAnime.animation == &gPlayerAnim_003148) {
            anim = &gPlayerAnim_003150;
        } else if (Player_IsUnfriendlyZTargeting(this)) {
            anim = &gPlayerAnim_002538;
            Player_ResetLeftRightBlendWeight(this);
        } else if (this->fallDistance <= 80) {
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SHORT_JUMP_LANDING, this->modelAnimType);
        } else if ((this->fallDistance < 800) && (this->relativeAnalogStickInputs[this->inputFrameCounter] == 0) &&
                   !(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR)) {
            Player_SetupRolling(this, play);
            return;
        }

        sp3C = func_80843E64(play, this);

        if (sp3C > 0) {
            Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_TALL_JUMP_LANDING, this->modelAnimType), play);
            this->skelAnime.endFrame = 8.0f;
            if (sp3C == 1) {
                this->genericTimer = 10;
            } else {
                this->genericTimer = 20;
            }
        } else if (sp3C == 0) {
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
    s32 temp;
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

            temp = Player_SetupInterruptAction(play, this, &this->skelAnime, 5.0f);
            if ((temp != 0) && ((temp > 0) || animDone)) {
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

                    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_17, this->modelAnimType));
                    this->linearVelocity = -this->linearVelocity;
                    Player_SetQuake(play, 33267, 3, 12);
                    Player_RequestRumble(this, 255, 20, 150, 0);
                    func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
                    this->genericTimer = 1;
                    return;
                }
            }

            if ((this->skelAnime.curFrame < 15.0f) || !Player_SetupMeleeWeaponAttack(this, play)) {
                if (this->skelAnime.curFrame >= 20.0f) {
                    Player_SetupReturnToStandStill(this, play);
                    return;
                }

                Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play);

                targetVelocity *= 1.5f;
                if ((targetVelocity < 3.0f) || (this->relativeAnalogStickInputs[this->inputFrameCounter] != 0)) {
                    targetVelocity = 3.0f;
                }

                func_8083DF68(this, targetVelocity, this->actor.shape.rot.y);

                if (func_8084269C(play, this)) {
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
        Player_PlayAnimLoop(play, this, &gPlayerAnim_003160);
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
            Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
            func_8083DFE0(this, &targetVelocity, &this->currentYaw);
            return;
        }

        if (func_80843E64(play, this) >= 0) {
            this->meleeWeaponAnimation += 2;
            Player_SetupMeleeWeaponAttackBehavior(play, this, this->meleeWeaponAnimation);
            this->slashCounter = 3;
            Player_PlayLandingSfx(this);
        }
    }
}

s32 Player_SetupReleaseSpinAttack(Player* this, PlayState* play) {
    s32 temp;

    if (Player_SetupCutscene(play, this)) {
        this->stateFlags2 |= PLAYER_STATE2_RELEASING_SPIN_ATTACK;
    } else {
        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
            if ((this->spinAttackTimer >= 0.85f) || Player_CanQuickspin(this)) {
                temp = D_80854384[Player_HoldsTwoHandedWeapon(this)];
            } else {
                temp = D_80854380[Player_HoldsTwoHandedWeapon(this)];
            }

            Player_SetupMeleeWeaponAttackBehavior(play, this, temp);
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
    s32 temp;

    this->stateFlags1 |= PLAYER_STATE1_CHARGING_SPIN_ATTACK;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        func_808355DC(this);
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
            Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

            temp = func_80840058(this, &targetVelocity, &targetYaw, play);
            if (temp > 0) {
                Player_SetupWalkChargingSpinAttack(this, play);
            } else if (temp < 0) {
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
    s32 temp4;
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
                               sSpinAttackChargeWalkAnims[Player_HoldsTwoHandedWeapon(this)], this->walkFrame * (21.0f / 29.0f), blendWeight,
                               this->blendTable);

    if (!Player_IsBusy(this, play) && !Player_SetupReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        temp4 = func_80840058(this, &targetVelocity, &targetYaw, play);

        if (temp4 < 0) {
            Player_SetupSidewalkChargingSpinAttack(this, play);
            return;
        }

        if (temp4 == 0) {
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
    s32 temp4;
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
                               sSpinAttackChargeSidewalkAnims[Player_HoldsTwoHandedWeapon(this)], this->walkFrame * (21.0f / 29.0f), blendWeight,
                               this->blendTable);

    if (!Player_IsBusy(this, play) && !Player_SetupReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);

        temp4 = func_80840058(this, &targetVelocity, &targetYaw, play);

        if (temp4 > 0) {
            Player_SetupWalkChargingSpinAttack(this, play);
            return;
        }

        if (temp4 == 0) {
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
    s32 interruptingSubAction;
    f32 landingSfxFrame;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    animDone = LinkAnimation_Update(play, &this->skelAnime);

    if (this->skelAnime.animation == &gPlayerAnim_002D48) {
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
        interruptingSubAction = Player_SetupInterruptAction(play, this, &this->skelAnime, 4.0f);

        if (interruptingSubAction == 0) {
            this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_JUMPING);
            return;
        }

        if ((animDone != 0) || (interruptingSubAction > 0)) {
            Player_SetupStandingStillNoMorph(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_JUMPING);
            return;
        }

        landingSfxFrame = 0.0f;

        if (this->skelAnime.animation == &gPlayerAnim_0032E8) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                Player_StartJumpOutOfWater(play, this, 10.0f);
            }
            landingSfxFrame = 50.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_002D40) {
            landingSfxFrame = 30.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_002D38) {
            landingSfxFrame = 16.0f;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, landingSfxFrame)) {
            Player_PlayLandingSfx(this);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }

        if ((this->skelAnime.animation == &gPlayerAnim_002D38) || (this->skelAnime.curFrame > 5.0f)) {
            if (this->genericTimer == 0) {
                Player_PlayJumpSfx(this);
                this->genericTimer = 1;
            }
            Math_StepToF(&this->actor.shape.yOffset, 0.0f, 150.0f);
        }
    }
}

void Player_RunMiniCutsceneFunc(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    LinkAnimation_Update(play, &this->skelAnime);

    if (((this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) && (this->heldActor != NULL) && (this->getItemId == GI_NONE)) ||
        !Player_SetupCurrentUpperAction(this, play)) {
        this->miniCsFunc(play, this);
    }
}

s32 func_80845964(PlayState* play, Player* this, CsCmdActorAction* arg2, f32 arg3, s16 arg4, s32 arg5) {
    if ((arg5 != 0) && (this->linearVelocity == 0.0f)) {
        return LinkAnimation_Update(play, &this->skelAnime);
    }

    if (arg5 != 2) {
        f32 sp34 = R_UPDATE_RATE * 0.5f;
        f32 selfDistX = arg2->endPos.x - this->actor.world.pos.x;
        f32 selfDistZ = arg2->endPos.z - this->actor.world.pos.z;
        f32 sp28 = sqrtf(SQ(selfDistX) + SQ(selfDistZ)) / sp34;
        s32 sp24 = (arg2->endFrame - play->csCtx.frames) + 1;

        arg4 = Math_Atan2S(selfDistZ, selfDistX);

        if (arg5 == 1) {
            f32 distX = arg2->endPos.x - arg2->startPos.x;
            f32 distZ = arg2->endPos.z - arg2->startPos.z;
            s32 temp = (((sqrtf(SQ(distX) + SQ(distZ)) / sp34) / (arg2->endFrame - arg2->startFrame)) / 1.5f) * 4.0f;

            if (temp >= sp24) {
                arg4 = this->actor.shape.rot.y;
                arg3 = 0.0f;
            } else {
                arg3 = sp28 / ((sp24 - temp) + 1);
            }
        } else {
            arg3 = sp28 / sp24;
        }
    }

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    func_80841EE4(this, play);
    func_8083DF68(this, arg3, arg4);

    if ((arg3 == 0.0f) && (this->linearVelocity == 0.0f)) {
        Player_EndRun(this, play);
    }

    return 0;
}

s32 func_80845BA0(PlayState* play, Player* this, f32* arg2, s32 arg3) {
    f32 dx = this->csStartPos.x - this->actor.world.pos.x;
    f32 dz = this->csStartPos.z - this->actor.world.pos.z;
    s32 sp2C = sqrtf(SQ(dx) + SQ(dz));
    s16 yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->csStartPos);

    if (sp2C < arg3) {
        *arg2 = 0.0f;
        yaw = this->actor.shape.rot.y;
    }

    if (func_80845964(play, this, NULL, *arg2, yaw, 2)) {
        return 0;
    }

    return sp2C;
}

s32 func_80845C68(PlayState* play, s32 arg1) {
    if (arg1 == 0) {
        Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
    }
    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 0;
    return arg1;
}

void func_80845CA4(Player* this, PlayState* play) {
    f32 sp3C;
    s32 temp;
    f32 sp34;
    s32 sp30;
    s32 pad;

    if (!Player_SetupItemCutsceneOrCUp(this, play)) {
        if (this->genericTimer == 0) {
            LinkAnimation_Update(play, &this->skelAnime);

            if (DECR(this->doorTimer) == 0) {
                this->linearVelocity = 0.1f;
                this->genericTimer = 1;
            }
        } else if (this->unk_84F == 0) {
            sp3C = 5.0f * sWaterSpeedScale;

            if (func_80845BA0(play, this, &sp3C, -1) < 30) {
                this->unk_84F = 1;
                this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;

                this->csStartPos.x = this->csEndPos.x;
                this->csStartPos.z = this->csEndPos.z;
            }
        } else {
            sp34 = 5.0f;
            sp30 = 20;

            if (this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) {
                sp34 = gSaveContext.entranceSpeed;

                if (sConveyorSpeedIndex != 0) {
                    this->csStartPos.x = (Math_SinS(sConveyorYaw) * 400.0f) + this->actor.world.pos.x;
                    this->csStartPos.z = (Math_CosS(sConveyorYaw) * 400.0f) + this->actor.world.pos.z;
                }
            } else if (this->genericTimer < 0) {
                this->genericTimer++;

                sp34 = gSaveContext.entranceSpeed;
                sp30 = -1;
            }

            temp = func_80845BA0(play, this, &sp34, sp30);

            if ((this->genericTimer == 0) || ((temp == 0) && (this->linearVelocity == 0.0f) &&
                                         (Play_GetCamera(play, CAM_ID_MAIN)->unk_14C & 0x10))) {

                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
                func_80845C68(play, gSaveContext.respawn[RESPAWN_MODE_DOWN].data);

                if (!Player_SetupSpeakOrCheck(this, play)) {
                    func_8083CF5C(this, play);
                }
            }
        }
    }

    if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
        Player_SetupCurrentUpperAction(this, play);
    }
}

void func_80845EF8(Player* this, PlayState* play) {
    s32 sp2C;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    sp2C = LinkAnimation_Update(play, &this->skelAnime);

    Player_SetupCurrentUpperAction(this, play);

    if (sp2C) {
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
        if (!Player_SetupItemCutsceneOrCUp(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_002FA0, play);
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
        Player_PlayAnimLoop(play, this, &gPlayerAnim_0032C0);
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
        Player_PlayAnimOnce(play, this, &gPlayerAnim_0032B8);
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
        Player_PlayAnimLoop(play, this, &gPlayerAnim_003070);
        this->genericTimer = 15;
        return;
    }

    if (this->genericTimer != 0) {
        this->genericTimer--;
        if (this->genericTimer == 0) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_003068, play);
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
        ((this->skelAnime.curFrame >= 8.0f) && Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.018f, play))) {
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

void Player_SpawnFromBlueWarp(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, func_8084F710, 0);
    if ((play->sceneNum == SCENE_SPOT06) && (gSaveContext.sceneSetupIndex >= 4)) {
        this->unk_84F = 1;
    }
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_003298, 2.0f / 3.0f, 0.0f, 24.0f, ANIMMODE_ONCE, 0.0f);
    this->actor.world.pos.y += 800.0f;
}

static u8 swordItemsByAge[] = { ITEM_SWORD_MASTER, ITEM_SWORD_KOKIRI };

void Player_CutsceneDrawSword(PlayState* play, Player* this, s32 arg2) {
    s32 item = swordItemsByAge[(void)0, gSaveContext.linkAge];
    s32 actionParam = sItemActionParams[item];

    Player_PutAwayHookshot(this);
    Player_DetatchHeldActor(play, this);

    this->heldItemId = item;
    this->nextModelGroup = Player_ActionToModelGroup(this, actionParam);

    Player_ChangeItem(play, this, actionParam);
    Player_SetupHeldItemUpperActionFunc(play, this);

    if (arg2 != 0) {
        func_8002F7DC(&this->actor, NA_SE_IT_SWORD_PICKOUT);
    }
}

static Vec3f D_808546F4 = { -1.0f, 69.0f, 20.0f };

void Player_SpawnFromTimeTravel(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, func_8084E9AC, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    Math_Vec3f_Copy(&this->actor.world.pos, &D_808546F4);
    this->currentYaw = this->actor.shape.rot.y = -0x8000;
    LinkAnimation_Change(play, &this->skelAnime, this->ageProperties->timeTravelEndAnim, 2.0f / 3.0f, 0.0f, 0.0f,
                         ANIMMODE_ONCE, 0.0f);
    Player_SetupAnimMovement(play, this,
                  PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                      PLAYER_ANIMMOVEFLAGS_7 | PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
    if (LINK_IS_ADULT) {
        Player_CutsceneDrawSword(play, this, 0);
    }
    this->genericTimer = 20;
}

void Player_SpawnOpeningDoor(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SetupOpenDoorFromSpawn, 0);
    Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
}

void Player_SpawnExitingGrotto(PlayState* play, Player* this) {
    Player_SetupJump(this, &gPlayerAnim_002FE0, 12.0f, play);
    Player_SetActionFunc(play, this, func_8084F9C0, 0);
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    this->fallStartHeight = this->actor.world.pos.y;
    OnePointCutscene_Init(play, 5110, 40, &this->actor, CAM_ID_MAIN);
}

void Player_SpawnWithKnockback(PlayState* play, Player* this) {
    Player_SetupDamage(play, this, PLAYER_DMGREACTION_KNOCKBACK, 2.0f, 2.0f, this->actor.shape.rot.y + 0x8000, 0);
}

void Player_SpawnFromWarpSong(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, func_8084F698, 0);
    this->actor.draw = NULL;
    this->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
}

static s16 sMagicSpellActors[] = { ACTOR_MAGIC_WIND, ACTOR_MAGIC_DARK, ACTOR_MAGIC_FIRE };

Actor* Player_SpawnMagicSpellActor(PlayState* play, Player* this, s32 index) {
    return Actor_Spawn(&play->actorCtx, play, sMagicSpellActors[index], this->actor.world.pos.x, this->actor.world.pos.y,
                       this->actor.world.pos.z, 0, 0, 0, 0);
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

    SkelAnime_InitLink(play, &this->skelAnime, skelHeader, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_STANDING_STILL, this->modelAnimType), 9,
                       this->jointTable, this->morphTable, PLAYER_LIMB_MAX);
    this->skelAnime.baseTransl = sBaseTransl;
    SkelAnime_InitLink(play, &this->skelAnimeUpper, skelHeader, Player_GetStandingStillAnim(this), 9, this->jointTableUpper,
                       this->morphTableUpper, PLAYER_LIMB_MAX);
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
    Player_SpawnFromBlueWarp,            // PLAYER_SPAWNMODE_FROM_BLUE_WARP
    Player_SpawnOpeningDoor,             // PLAYER_SPAWNMODE_OPENING_DOOR
    Player_SpawnExitingGrotto,           // PLAYER_SPAWNMODE_EXITING_GROTTO
    Player_SpawnFromWarpSong,            // PLAYER_SPAWNMODE_FROM_WARP_SONG
    Player_SetupSpawnFromFaroresWind,         // PLAYER_SPAWNMODE_FROM_FARORES_WIND
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
    play->damagePlayer = Player_SetupInflictDamage;
    play->talkWithPlayer = Player_StartTalkingWithActor;

    thisx->room = -1;
    this->ageProperties = &sAgeProperties[gSaveContext.linkAge];
    this->itemActionParam = this->heldItemActionParam = -1;
    this->heldItemId = ITEM_NONE;

    Player_UseItem(play, this, ITEM_NONE);
    Player_SetEquipmentData(play, this);
    this->prevBoots = this->currentBoots;
    Player_InitCommon(this, play, gPlayerSkelHeaders[((void)0, gSaveContext.linkAge)]);
    this->giObjectSegment = (void*)(((u32)ZeldaArena_MallocDebug(0x3008, "../z_player.c", 17175) + 8) & ~0xF);

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
            if ((gSaveContext.sceneSetupIndex < 4) &&
                (gEntranceTable[((void)0, gSaveContext.entranceIndex) + ((void)0, gSaveContext.sceneSetupIndex)].field &
                 ENTRANCE_INFO_DISPLAY_TITLE_CARD_FLAG) &&
                ((play->sceneNum != SCENE_DDAN) || GET_EVENTCHKINF(EVENTCHKINF_B0)) &&
                ((play->sceneNum != SCENE_NIGHT_SHOP) || GET_EVENTCHKINF(EVENTCHKINF_25))) {
                TitleCard_InitPlaceName(play, &play->actorCtx.titleCtx, this->giObjectSegment, 160, 120, 144, 24, 20);
            }
        }
        gSaveContext.showTitleCard = true;
    }

    if (func_80845C68(play, (respawnFlag == 2) ? 1 : 0) == 0) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = (thisx->params & 0xFF) | 0xD00;
    }

    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 1;

    if (play->sceneNum <= SCENE_GANONTIKA_SONOGO) {
        gSaveContext.infTable[INFTABLE_1AX_INDEX] |= gBitFlags[play->sceneNum];
    }

    spawnMode = (thisx->params & 0xF00) >> 8;
    if ((spawnMode == PLAYER_SPAWNMODE_FROM_WARP_SONG) || (spawnMode == PLAYER_SPAWNMODE_FROM_FARORES_WIND)) {
        if (gSaveContext.cutsceneIndex >= 0xFFF0) {
            spawnMode = PLAYER_SPAWNMODE_NO_MOMENTUM;
        }
    }

    sSpawnFuncs[spawnMode](play, this);

    if (spawnMode != PLAYER_SPAWNMODE_NO_UPDATE_OR_DRAW) {
        if ((gSaveContext.gameMode == 0) || (gSaveContext.gameMode == 3)) {
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
        Audio_PlayActorSound2(&this->actor, ((void)0, gSaveContext.entranceSound));
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
        if (this->unk_6B0 != 0) {
            Player_StepValueToZero(&this->unk_6B0);
        } else {
            Player_StepValueToZero(&this->upperBodyRot.y);
        }
    }

    if (!(this->lookFlags & PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Z)) {
        Player_StepValueToZero(&this->upperBodyRot.z);
    }

    this->lookFlags = 0;
}

static f32 D_80854784[] = { 120.0f, 240.0f, 360.0f };

static u8 sDiveDoActions[] = { DO_ACTION_1, DO_ACTION_2, DO_ACTION_3, DO_ACTION_4,
                               DO_ACTION_5, DO_ACTION_6, DO_ACTION_7, DO_ACTION_8 };

void func_808473D4(PlayState* play, Player* this) {
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) && (this->actor.category == ACTORCAT_PLAYER)) {
        Actor* heldActor = this->heldActor;
        Actor* interactRangeActor = this->interactRangeActor;
        s32 sp24;
        s32 sp20 = this->relativeAnalogStickInputs[this->inputFrameCounter];
        s32 sp1C = Player_IsSwimming(this);
        s32 doAction = DO_ACTION_NONE;

        if (!Player_InBlockingCsMode(play, this)) {
            if (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE) {
                doAction = DO_ACTION_RETURN;
            } else if ((this->heldItemActionParam == PLAYER_AP_FISHING_POLE) && (this->fishingState != 0)) {
                if (this->fishingState == 2) {
                    doAction = DO_ACTION_REEL;
                }
            } else if ((func_8084E3C4 != this->actionFunc) && !(this->stateFlags2 & PLAYER_STATE2_INSIDE_CRAWLSPACE)) {
                if ((this->doorType != PLAYER_DOORTYPE_NONE) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) ||
                     ((heldActor != NULL) && (heldActor->id == ACTOR_EN_RU1)))) {
                    doAction = DO_ACTION_OPEN;
                } else if ((!(this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) || (heldActor == NULL)) &&
                           (interactRangeActor != NULL) &&
                           ((!sp1C && (this->getItemId == GI_NONE)) ||
                            ((this->getItemId < 0) && !(this->stateFlags1 & PLAYER_STATE1_SWIMMING)))) {
                    if (this->getItemId < 0) {
                        doAction = DO_ACTION_OPEN;
                    } else if ((interactRangeActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                        doAction = DO_ACTION_DROP;
                    } else {
                        doAction = DO_ACTION_GRAB;
                    }
                } else if (!sp1C && (this->stateFlags2 & PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL)) {
                    doAction = DO_ACTION_GRAB;
                } else if ((this->stateFlags2 & PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL) ||
                           (!(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->rideActor != NULL))) {
                    doAction = DO_ACTION_CLIMB;
                } else if ((this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && !EN_HORSE_CHECK_4((EnHorse*)this->rideActor) &&
                           (func_8084D3E4 != this->actionFunc)) {
                    if ((this->stateFlags2 & PLAYER_STATE2_CAN_SPEAK_OR_CHECK) && (this->talkActor != NULL)) {
                        if (this->talkActor->category == ACTORCAT_NPC) {
                            doAction = DO_ACTION_SPEAK;
                        } else {
                            doAction = DO_ACTION_CHECK;
                        }
                    } else if (!Actor_PlayerIsAimingReadyFpsItem(this) && !(this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE)) {
                        doAction = DO_ACTION_FASTER;
                    }
                } else if ((this->stateFlags2 & PLAYER_STATE2_CAN_SPEAK_OR_CHECK) && (this->talkActor != NULL)) {
                    if (this->talkActor->category == ACTORCAT_NPC) {
                        doAction = DO_ACTION_SPEAK;
                    } else {
                        doAction = DO_ACTION_CHECK;
                    }
                } else if ((this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING)) ||
                           ((this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) && (this->stateFlags2 & PLAYER_STATE2_CAN_DISMOUNT_HORSE))) {
                    doAction = DO_ACTION_DOWN;
                } else if (this->stateFlags2 & PLAYER_STATE2_CAN_ENTER_CRAWLSPACE) {
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
                    sp24 = (D_80854784[CUR_UPG_VALUE(UPG_SCALE)] - this->actor.yDistToWater) / 40.0f;
                    sp24 = CLAMP(sp24, 0, 7);
                    doAction = sDiveDoActions[sp24];
                } else if (sp1C && !(this->stateFlags2 & PLAYER_STATE2_DIVING)) {
                    doAction = DO_ACTION_DIVE;
                } else if (!sp1C && (!(this->stateFlags1 & PLAYER_STATE1_SHIELDING) || Player_IsZTargeting(this) ||
                                     !Player_IsChildWithHylianShield(this))) {
                    if ((!(this->stateFlags1 & PLAYER_STATE1_CLIMBING_ONTO_LEDGE) && (sp20 <= 0) &&
                         (Player_IsUnfriendlyZTargeting(this) ||
                          ((sFloorSpecialProperty != BGCHECK_FLOORSPECIALPROPERTY_QUICKSAND_NO_HORSE) &&
                           (Player_IsFriendlyZTargeting(this) || ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
                                                    !(this->stateFlags1 & PLAYER_STATE1_SHIELDING) && (sp20 == 0))))))) {
                        doAction = DO_ACTION_ATTACK;
                    } else if ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) && Player_IsZTargeting(this) &&
                               (sp20 > 0)) {
                        doAction = DO_ACTION_JUMP;
                    } else if ((this->heldItemActionParam >= PLAYER_AP_SWORD_MASTER) ||
                               ((this->stateFlags2 & PLAYER_STATE2_NAVI_IS_ACTIVE) &&
                                (play->actorCtx.targetCtx.arrowPointedActor == NULL))) {
                        doAction = DO_ACTION_PUTAWAY;
                    }
                }
            }
        }

        if (doAction != DO_ACTION_PUTAWAY) {
            this->unk_837 = 20;
        } else if (this->unk_837 != 0) {
            doAction = DO_ACTION_NONE;
            this->unk_837--;
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

s32 func_80847A78(Player* this) {
    s32 cond;

    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && (this->hoverBootsTimer != 0)) {
        this->hoverBootsTimer--;
    } else {
        this->hoverBootsTimer = 0;
    }

    cond = (this->currentBoots == PLAYER_BOOTS_HOVER) &&
           ((this->actor.yDistToWater >= 0.0f) || (Player_GetHurtFloorType(sFloorSpecialProperty) >= 0) || Player_IsFloorSinkingSand(sFloorSpecialProperty));

    if (cond && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->hoverBootsTimer != 0)) {
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    }

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        if (!cond) {
            this->hoverBootsTimer = 19;
        }
        return 0;
    }

    sFloorSpecialProperty = BGCHECK_FLOORSPECIALPROPERTY_NONE;
    this->angleToFloorX = this->angleToFloorY = D_80853610 = 0;

    return 1;
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

    if (this->stateFlags2 & PLAYER_STATE2_INSIDE_CRAWLSPACE) {
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
        } else if ((this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) && ((this->sceneExitPosY - (s32)this->actor.world.pos.y) >= 100)) {
            updBgcheckFlags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) &&
                   ((func_80845EF8 == this->actionFunc) || (func_80845CA4 == this->actionFunc))) {
            this->actor.bgCheckFlags &= ~(BGCHECKFLAG_WALL | BGCHECKFLAG_PLAYER_WALL_INTERACT);
            updBgcheckFlags = UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else {
            updBgcheckFlags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
                   UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        }
    } else {
        updBgcheckFlags = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
               UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
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
        this->floorProperty = func_80041EA4(&play->colCtx, floorPoly, this->actor.floorBgId);
        this->prevMoveSfxType = this->moveSfxType;

        if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
            if (this->actor.yDistToWater < 20.0f) {
                this->moveSfxType = PLAYER_MOVESFXTYPE_SHALLOW_WATER;
            } else {
                this->moveSfxType = PLAYER_MOVESFXTYPE_DEEP_WATER;
            }
        } else {
            if (this->stateFlags2 & PLAYER_STATE2_SPAWN_DUST_AT_FEET) {
                this->moveSfxType = PLAYER_MOVESFXTYPE_SAND;
            } else {
                this->moveSfxType = SurfaceType_SetMoveSfx(&play->colCtx, floorPoly, this->actor.floorBgId);
            }
        }

        if (this->actor.category == ACTORCAT_PLAYER) {
            Audio_SetCodeReverb(SurfaceType_GetEcho(&play->colCtx, floorPoly, this->actor.floorBgId));

            if (this->actor.floorBgId == BGCHECK_SCENE) {
                Environment_ChangeLightSetting(
                    play, SurfaceType_GetLightSettingIndex(&play->colCtx, floorPoly, this->actor.floorBgId));
            } else {
                func_80043508(&play->colCtx, this->actor.floorBgId);
            }
        }

        // This block extracts the conveyor properties from the floor poly
        sConveyorSpeedIndex = SurfaceType_GetConveyorSpeed(&play->colCtx, floorPoly, this->actor.floorBgId);
        if (sConveyorSpeedIndex != 0) {
            sIsFloorConveyor = SurfaceType_IsFloorConveyor(&play->colCtx, floorPoly, this->actor.floorBgId);
            if ((!sIsFloorConveyor && (this->actor.yDistToWater > 20.0f) &&
                 (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                (sIsFloorConveyor && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {
                sConveyorYaw =
                    SurfaceType_GetConveyorDirection(&play->colCtx, floorPoly, this->actor.floorBgId) * (0x10000 / 64);
            } else {
                sConveyorSpeedIndex = 0;
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

        if (!(this->stateFlags2 & PLAYER_STATE2_INSIDE_CRAWLSPACE) &&
            Player_WallLineTestWithOffset(play, this, &sWallCheckOffset, &wallPoly, &wallBgId, &sWallIntersectPos)) {
            this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
            if (this->actor.wallPoly != wallPoly) {
                this->actor.wallPoly = wallPoly;
                this->actor.wallBgId = wallBgId;
                this->actor.wallYaw = Math_Atan2S(wallPoly->normal.z, wallPoly->normal.x);
            }
        }

        wallYawDiff = this->actor.shape.rot.y - (s16)(this->actor.wallYaw + 0x8000);

        sTouchedWallFlags = func_80041DB8(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId);

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
        if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sYawToTouchedWall < DEG_TO_BINANG(67.5f))) {
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

                ledgePosY = BgCheck_EntityRaycastFloor1(&play->colCtx, &floorPoly2, &checkPos);
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
                        !func_80041E18(&play->colCtx, wallPoly2, wallBgId)) {
                        this->wallHeight = 399.96002f;
                    } else if (func_80041DE4(&play->colCtx, wallPoly, this->actor.wallBgId) == false) {
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
        sFloorSpecialProperty = func_80041D4C(&play->colCtx, floorPoly, this->actor.floorBgId);

        if (!func_80847A78(this)) {
            f32 floorPolyNormalX;
            f32 invFloorPolyNormalY;
            f32 floorPolyNormalZ;
            f32 sin;
            s32 pad2;
            f32 cos;
            s32 pad3;

            if (this->actor.floorBgId != BGCHECK_SCENE) {
                func_800434C8(&play->colCtx, this->actor.floorBgId);
            }

            floorPolyNormalX = COLPOLY_GET_NORMAL(floorPoly->normal.x);
            invFloorPolyNormalY = 1.0f / COLPOLY_GET_NORMAL(floorPoly->normal.y);
            floorPolyNormalZ = COLPOLY_GET_NORMAL(floorPoly->normal.z);

            sin = Math_SinS(this->currentYaw);
            cos = Math_CosS(this->currentYaw);

            this->angleToFloorX =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sin) - (floorPolyNormalZ * cos)) * invFloorPolyNormalY);
            this->angleToFloorY =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * cos) - (floorPolyNormalZ * sin)) * invFloorPolyNormalY);

            sin = Math_SinS(this->actor.shape.rot.y);
            cos = Math_CosS(this->actor.shape.rot.y);

            D_80853610 =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sin) - (floorPolyNormalZ * cos)) * invFloorPolyNormalY);

            func_8083E318(play, this, floorPoly);
        }
    } else {
        func_80847A78(this);
    }

    if (this->prevFloorSpecialProperty == sFloorSpecialProperty) {
        this->hurtFloorTimer++;
    } else {
        this->prevFloorSpecialProperty = sFloorSpecialProperty;
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
            } else if (func_8084377C == this->actionFunc) {
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
            } else if (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE)) {
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
            } else if ((this->isMeleeWeaponAttacking != 0) && (this->meleeWeaponAnimation >= PLAYER_MWA_FORWARD_SLASH_1H) &&
                       (this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H)) {
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

        if (play->sceneNum != SCENE_TURIBORI) {
            Audio_SetSequenceMode(seqMode);
        }
    }
}

static Vec3f D_808547A4 = { 0.0f, 0.5f, 0.0f };
static Vec3f D_808547B0 = { 0.0f, 0.5f, 0.0f };

static Color_RGBA8 D_808547BC = { 255, 255, 100, 255 };
static Color_RGBA8 D_808547C0 = { 255, 50, 0, 0 };

void func_80848A04(PlayState* play, Player* this) {
    f32 newDekuStickLength;

    if (this->dekuStickLength == 0.0f) {
        Player_UseItem(play, this, ITEM_NONE);
        return;
    }

    newDekuStickLength = 1.0f;
    if (DECR(this->stickFlameTimer) == 0) {
        Inventory_ChangeAmmo(ITEM_STICK, -1);
        this->stickFlameTimer = 1;
        newDekuStickLength = 0.0f;
        this->dekuStickLength = newDekuStickLength;
    } else if (this->stickFlameTimer > 200) {
        newDekuStickLength = (210 - this->stickFlameTimer) / 10.0f;
    } else if (this->stickFlameTimer < 20) {
        newDekuStickLength = this->stickFlameTimer / 20.0f;
        this->dekuStickLength = newDekuStickLength;
    }

    func_8002836C(play, &this->meleeWeaponInfo[0].tip, &D_808547A4, &D_808547B0, &D_808547BC, &D_808547C0,
                  newDekuStickLength * 200.0f, 0, 8);
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

        if (play->sceneNum == SCENE_JYASINBOSS) {
            dmgCooldown = 0;
        } else {
            dmgCooldown = 7;
        }

        if ((dmgCooldown & play->gameplayFrames) == 0) {
            Player_SetupInflictDamage(play, -1);
        }
    } else {
        this->isBurning = false;
    }
}

void func_80848EF8(Player* this) {
    if (CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY)) {
        f32 temp = 200000.0f - (this->unk_6A4 * 5.0f);

        if (temp < 0.0f) {
            temp = 0.0f;
        }

        this->unk_6A0 += temp;
        if (this->unk_6A0 > 4000000.0f) {
            this->unk_6A0 = 0.0f;
            Player_RequestRumble(this, 120, 20, 10, 0);
        }
    }
}

static s8 sLinkCsActions[] = {
    0,  3,  3,  5,   4,   8,   9,   13, 14, 15, 16, 17, 18, -22, 23, 24, 25,  26, 27,  28,  29, 31, 32, 33, 34, -35,
    30, 36, 38, -39, -40, -41, 42,  43, 45, 46, 0,  0,  0,  67,  48, 47, -50, 51, -52, -53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63,  64,  -65, -66, 68, 11, 69, 70, 71, 8,  8,   72, 73, 78,  79, 80,  89,  90, 91, 92, 77, 19, 94,
};

static Vec3f sHorseRaycastOffset = { 0.0f, 0.0f, 200.0f };

static f32 sWaterConveyorSpeeds[] = { 2.0f, 4.0f, 7.0f };
static f32 sFloorConveyorSpeeds[] = { 0.5f, 1.0f, 3.0f };

void Player_UpdateCommon(Player* this, PlayState* play, Input* input) {
    s32 pad;

    // // TESTING

    // s32 i;

    // typedef struct {
    //     void* func;
    //     char name[60];
    // } TestFuncs;

    // static TestFuncs testFuncs[] = {
    //     { Player_FriendlyZTargetStandingStill, "Player_FriendlyZTargetStandingStill" },
    //     { Player_UnfriendlyZTargetStandingStill, "Player_UnfriendlyZTargetStandingStill" },
    //     { Player_FriendlyBackwalk, "Player_FriendlyBackwalk" },
    //     { Player_UnfriendlyBackwalk, "Player_UnfriendlyBackwalk" },
    //     { Player_Sidewalk, "Player_Sidewalk" },
    //     { Player_Turn, "Player_Turn" },
    //     { Player_ZTargetingRun, "Player_ZTargetingRun" },
    //     { Player_Run, "Player_Run" },
    //     { Player_ChargeSpinAttack, "Player_ChargeSpinAttack" },
    //     { Player_WalkChargingSpinAttack, "Player_WalkChargingSpinAttack" },
    //     { Player_SidewalkChargingSpinAttack, "Player_SidewalkChargingSpinAttack" },
    //     { Player_Rolling, "Player_Rolling" },
    //     { Player_UpdateMidair, "Player_UpdateMidair" },
    //     { Player_HoldActor, "Player_HoldActor" },
    // };

    // GfxPrint printer;
    // Gfx* gfx;

    // OPEN_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    // gfx = POLY_OPA_DISP + 1;
    // gSPDisplayList(OVERLAY_DISP++, gfx);

    // GfxPrint_Init(&printer);
    // GfxPrint_Open(&printer, gfx);

    // GfxPrint_SetColor(&printer, 255, 0, 255, 255);
    // GfxPrint_SetPos(&printer, 2, 10);
    // for (i = 0; i < ARRAY_COUNT(testFuncs); i++) {
    //     if (testFuncs[i].func == this->actionFunc) {
    //         GfxPrint_Printf(&printer, "%s", testFuncs[i].name);
    //     }
    // }
    // GfxPrint_SetPos(&printer, 2, 11);
    // GfxPrint_Printf(&printer, "this->voidRespawnCounter: %d", this->voidRespawnCounter);

    // gfx = GfxPrint_Close(&printer);
    // GfxPrint_Destroy(&printer);

    // gSPEndDisplayList(gfx++);
    // gSPBranchList(POLY_OPA_DISP, gfx);
    // POLY_OPA_DISP = gfx;

    // CLOSE_DISPS(play->state.gfxCtx, __FILE__, __LINE__);

    // // END TESTING

    sControlInput = input;

    if (this->voidRespawnCounter < 0) {
        this->voidRespawnCounter++;
        if (this->voidRespawnCounter == 0) {
            this->voidRespawnCounter = 1;
            func_80078884(NA_SE_OC_REVENGE);
        }
    }

    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.home.pos);

    if (this->unk_A73 != 0) {
        this->unk_A73--;
    }

    if (this->unk_88E != 0) {
        this->unk_88E--;
    }

    if (this->unk_A87 != 0) {
        this->unk_A87--;
    }

    if (this->invincibilityTimer < 0) {
        this->invincibilityTimer++;
    } else if (this->invincibilityTimer > 0) {
        this->invincibilityTimer--;
    }

    if (this->runDamageTimer != 0) {
        this->runDamageTimer--;
    }

    func_808473D4(play, this);
    func_80836BEC(this, play);

    if ((this->heldItemActionParam == PLAYER_AP_STICK) && (this->stickFlameTimer != 0)) {
        func_80848A04(play, this);
    } else if ((this->heldItemActionParam == PLAYER_AP_FISHING_POLE) && (this->stickFlameTimer < 0)) {
        this->stickFlameTimer++;
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
            Player_PlayAnimOnce(play, this, &gPlayerAnim_0033B8);
            Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
            this->genericTimer = 99;
        }

        if (this->comboTimer == 0) {
            this->slashCounter = 0;
        } else if (this->comboTimer < 0) {
            this->comboTimer++;
        } else {
            this->comboTimer--;
        }

        Math_ScaledStepToS(&this->unk_6C2, 0, 400);
        func_80032CB4(this->unk_3A8, 20, 80, 6);

        this->actor.shape.face = this->unk_3A8[0] + ((play->gameplayFrames & 32) ? 0 : 3);

        if (this->currentMask == PLAYER_MASK_BUNNY) {
            func_8085002C(this);
        }

        if (Actor_PlayerIsAimingFpsItem(this) != 0) {
            Player_BowStringMoveAfterShot(this);
        }

        if (!(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_7)) {
            if (((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (sFloorSpecialProperty == BGCHECK_FLOORSPECIALPROPERTY_SLIPPERY_ICE) &&
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
                !(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING)) &&
                (Player_JumpUpToLedge != this->actionFunc) && (Player_UpdateMagicSpell != this->actionFunc)) {
                this->actor.velocity.x += this->pushedSpeed * Math_SinS(this->pushedYaw);
                this->actor.velocity.z += this->pushedSpeed * Math_CosS(this->pushedYaw);
            }

            func_8002D7EC(&this->actor);
            Player_UpdateBgcheck(play, this);
        } else {
            sFloorSpecialProperty = BGCHECK_FLOORSPECIALPROPERTY_NONE;
            this->floorProperty = BGCHECK_FLOORPROPERTY_NONE;

            if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE) && (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE)) {
                EnHorse* rideActor = (EnHorse*)this->rideActor;
                CollisionPoly* floorPoly;
                s32 floorBgId;
                Vec3f raycastPos;

                if (!(rideActor->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_RaycastFloorWithOffset(play, this, &sHorseRaycastOffset, &raycastPos, &floorPoly, &floorBgId);
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

            sConveyorSpeedIndex = 0;
            this->pushedSpeed = 0.0f;
        }

        // This block applies the bg conveyor to pushedSpeed
        if ((sConveyorSpeedIndex != 0) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
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

        if (!Player_InBlockingCsMode(play, this) && !(this->stateFlags2 & PLAYER_STATE2_INSIDE_CRAWLSPACE)) {
            func_8083D53C(play, this);

            if ((this->actor.category == ACTORCAT_PLAYER) && (gSaveContext.health == 0)) {
                if (this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING)) {
                    Player_ResetAttributes(play, this);
                    Player_SetupFallFromLedge(this, play);
                } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->stateFlags1 & PLAYER_STATE1_SWIMMING)) {
                    Player_SetupDie(play, this,
                                    Player_IsSwimming(this)   ? &gPlayerAnim_003310
                                    : (this->shockTimer != 0) ? &gPlayerAnim_002F08
                                                              : &gPlayerAnim_002878);
                }
            } else {
                if ((this->actor.parent == NULL) && ((play->transitionTrigger == TRANS_TRIGGER_START) ||
                                                     (this->unk_A87 != 0) || !Player_UpdateDamage(this, play))) {
                    Player_SetupMidairBehavior(this, play);
                } else {
                    this->fallStartHeight = this->actor.world.pos.y;
                }
                func_80848EF8(this);
            }
        }

        if ((play->csCtx.state != CS_STATE_IDLE) && (this->csMode != PLAYER_CSMODE_UNK_6) && !(this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) &&
            !(this->stateFlags2 & PLAYER_STATE2_RESTRAINED_BY_ENEMY) && (this->actor.category == ACTORCAT_PLAYER)) {
            CsCmdActorAction* linkActionCsCmd = play->csCtx.linkAction;

            if ((linkActionCsCmd != NULL) && (sLinkCsActions[linkActionCsCmd->action] != 0)) {
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
                !(this->stateFlags1 & (PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_CLIMBING | PLAYER_STATE1_TAKING_DAMAGE))) {
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

        this->stateFlags1 &= ~(PLAYER_STATE1_SWINGING_BOTTLE | PLAYER_STATE1_READY_TO_SHOOT | PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_SHIELDING);
        this->stateFlags2 &= ~(PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL | PLAYER_STATE2_MAKING_REACTABLE_NOISE | PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION |
                               PLAYER_STATE2_ENABLE_PUSH_PULL_CAM | PLAYER_STATE2_SPAWN_DUST_AT_FEET | PLAYER_STATE2_IDLE_WHILE_CLIMBING | PLAYER_STATE2_FROZEN_IN_ICE |
                               PLAYER_STATE2_CAN_ENTER_CRAWLSPACE | PLAYER_STATE2_CAN_DISMOUNT_HORSE | PLAYER_STATE2_ENABLE_REFLECTION);
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
        D_80858AA4 = this->currentMask;

        if (!(this->stateFlags3 & PLAYER_STATE3_2)) {
            this->actionFunc(this, play);
        }

        Player_UpdateCamAndSeqModes(play, this);

        if (this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION) {
            AnimationContext_SetMoveActor(play, &this->actor, &this->skelAnime,
                                          (this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE) ? 1.0f : this->ageProperties->translationScale);
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
        this->unk_6A4 = FLT_MAX;

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

        if (this->stateFlags1 & PLAYER_STATE1_SHIELDING) {
            this->cylinder.dim.height = this->cylinder.dim.height * 0.8f;
        }

        Collider_UpdateCylinder(&this->actor, &this->cylinder);

        if (!(this->stateFlags2 & PLAYER_STATE2_FROZEN_IN_ICE)) {
            if (!(this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP | PLAYER_STATE1_CLIMBING_ONTO_LEDGE | PLAYER_STATE1_RIDING_HORSE))) {
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->cylinder.base);
            }

            if (!(this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_TAKING_DAMAGE)) && (this->invincibilityTimer <= 0)) {
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

    if (this->stateFlags1 & (PLAYER_STATE1_IN_DEATH_CUTSCENE | PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE)) {
        this->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        this->actor.colChkInfo.mass = 50;
    }

    this->stateFlags3 &= ~PLAYER_STATE3_2;

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
    Input sp44;
    Actor* dog;

    if (func_8084FCAC(this, play)) {
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
            bzero(&sp44, sizeof(sp44));
        } else {
            sp44 = play->state.input[0];
            if (this->unk_88E != 0) {
                sp44.cur.button &= ~(BTN_A | BTN_B | BTN_CUP);
                sp44.press.button &= ~(BTN_A | BTN_B | BTN_CUP);
            }
        }

        Player_UpdateCommon(this, play, &sp44);
    }

    MREG(52) = this->actor.world.pos.x;
    MREG(53) = this->actor.world.pos.y;
    MREG(54) = this->actor.world.pos.z;
    MREG(55) = this->actor.world.rot.y;
}

static struct_80858AC8 D_80858AC8;
static Vec3s D_80858AD8[25];

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
                       Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, 0, 16, 32, 1, 0, (play->gameplayFrames * -15) % 128,
                                        16, 32));
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

        if ((this->csMode != PLAYER_CSMODE_NONE) || (Player_IsUnfriendlyZTargeting(this) && 0) || (this->actor.projectedPos.z < 160.0f)) {
            lod = 0;
        } else {
            lod = 1;
        }

        func_80093C80(play);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);

        if (this->invincibilityTimer > 0) {
            this->damageFlashTimer += CLAMP(50 - this->invincibilityTimer, 8, 40);
            POLY_OPA_DISP =
                Gfx_SetFog2(POLY_OPA_DISP, 255, 0, 0, 0, 0, 4000 - (s32)(Math_CosS(this->damageFlashTimer * 256) * 2000.0f));
        }

        func_8002EBCC(&this->actor, play, 0);
        func_8002ED80(&this->actor, play, 0);

        if (this->attentionMode != PLAYER_ATTENTIONMODE_NONE) {
            Vec3f projectedHeadPos;

            SkinMatrix_Vec3fMtxFMultXYZ(&play->viewProjectionMtxF, &this->actor.focus.pos, &projectedHeadPos);
            if (projectedHeadPos.z < -4.0f) {
                overrideLimbDraw = Player_OverrideLimbDrawGameplayFirstPerson;
            }
        } else if (this->stateFlags2 & PLAYER_STATE2_INSIDE_CRAWLSPACE) {
            if (this->actor.projectedPos.z < 0.0f) {
                overrideLimbDraw = Player_OverrideLimbDrawGameplay_80090440;
            }
        }

        if (this->stateFlags2 & PLAYER_STATE2_ENABLE_REFLECTION) {
            f32 sp78 = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 600));
            f32 sp74 = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 1000));

            Matrix_Push();
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_SetTranslateRotateYXZ(
                this->actor.world.pos.x,
                (this->actor.floorHeight + (this->actor.floorHeight - this->actor.world.pos.y)) +
                    (this->actor.shape.yOffset * this->actor.scale.y),
                this->actor.world.pos.z, &this->actor.shape.rot);
            Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
            Matrix_RotateX(sp78, MTXMODE_APPLY);
            Matrix_RotateY(sp74, MTXMODE_APPLY);
            Matrix_Scale(1.1f, 0.95f, 1.05f, MTXMODE_APPLY);
            Matrix_RotateY(-sp74, MTXMODE_APPLY);
            Matrix_RotateX(-sp78, MTXMODE_APPLY);
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
            f32 scale = (this->unk_84F >> 1) * 22.0f;

            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, (0 - play->gameplayFrames) % 128, 32, 32, 1, 0,
                                        (play->gameplayFrames * -2) % 128, 32, 32));

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

s16 func_8084ABD8(PlayState* play, Player* this, s32 arg2, s16 arg3) {
    s32 temp1;
    s16 temp2;
    s16 temp3;

    if (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this) && (arg2 == 0)) {
        temp2 = sControlInput->rel.stick_y * 240.0f;
        Math_SmoothStepToS(&this->actor.focus.rot.x, temp2, 14, 4000, 30);

        temp2 = sControlInput->rel.stick_x * -16.0f;
        temp2 = CLAMP(temp2, -3000, 3000);
        this->actor.focus.rot.y += temp2;
    } else {
        temp1 = (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) ? 3500 : 14000;
        temp3 = ((sControlInput->rel.stick_y >= 0) ? 1 : -1) *
                (s32)((1.0f - Math_CosS(sControlInput->rel.stick_y * 200)) * 1500.0f);
        this->actor.focus.rot.x += temp3;
        this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -temp1, temp1);

        temp1 = 19114;
        temp2 = this->actor.focus.rot.y - this->actor.shape.rot.y;
        temp3 = ((sControlInput->rel.stick_x >= 0) ? 1 : -1) *
                (s32)((1.0f - Math_CosS(sControlInput->rel.stick_x * 200)) * -1500.0f);
        temp2 += temp3;
        this->actor.focus.rot.y = CLAMP(temp2, -temp1, temp1) + this->actor.shape.rot.y;
    }

    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y;
    return Player_UpdateLookAngles(this, (play->shootingGalleryStatus != 0) || Actor_PlayerIsAimingReadyFpsItem(this) || Player_IsAimingReadyBoomerang(this)) - arg3;
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

void func_8084B1D8(Player* this, PlayState* play) {
    if (this->stateFlags1 & PLAYER_STATE1_SWIMMING) {
        Player_SetVerticalWaterVelocity(this);
        Player_UpdateSwimMovement(this, &this->linearVelocity, 0, this->actor.shape.rot.y);
    } else {
        Player_StepLinearVelocityToZero(this);
    }

    if ((this->attentionMode == PLAYER_ATTENTIONMODE_AIMING) && (Actor_PlayerIsAimingFpsItem(this) || Player_IsAimingBoomerang(this))) {
        Player_SetupCurrentUpperAction(this, play);
    }

    if ((this->csMode != PLAYER_CSMODE_NONE) || (this->attentionMode == PLAYER_ATTENTIONMODE_NONE) || (this->attentionMode >= PLAYER_ATTENTIONMODE_ITEM_CUTSCENE) || Player_SetupStartUnfriendlyZTargeting(this) ||
        (this->targetActor != NULL) || !Player_SetupCameraMode(play, this) ||
        (((this->attentionMode == PLAYER_ATTENTIONMODE_AIMING) && (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_R) ||
                                   Player_IsFriendlyZTargeting(this) || (!Actor_PlayerIsAimingReadyFpsItem(this) && !Player_IsAimingReadyBoomerang(this)))) ||
         ((this->attentionMode == PLAYER_ATTENTIONMODE_C_UP) &&
          CHECK_BTN_ANY(sControlInput->press.button,
                        BTN_A | BTN_B | BTN_R | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)))) {
        Player_ClearLookAndAttention(this, play);
        func_80078884(NA_SE_SY_CAMERA_ZOOM_UP);
    } else if ((DECR(this->genericTimer) == 0) || (this->attentionMode != PLAYER_ATTENTIONMODE_AIMING)) {
        if (Player_IsShootingHookshot(this)) {
            this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y | PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X;
        } else {
            this->actor.shape.rot.y = func_8084ABD8(play, this, 0, 0);
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

void Player_SetOcarinaItemAP(Player* this) {
    this->itemActionParam =
        (INV_CONTENT(ITEM_OCARINA_FAIRY) == ITEM_OCARINA_FAIRY) ? PLAYER_AP_OCARINA_FAIRY : PLAYER_AP_OCARINA_TIME;
}

s32 Player_SetupForcePullOcarina(PlayState* play, Player* this) {
    if (this->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA) {
        this->stateFlags3 &= ~PLAYER_STATE3_FORCE_PULL_OCARINA;
        Player_SetOcarinaItemAP(this);
        this->attentionMode = PLAYER_ATTENTIONMODE_ITEM_CUTSCENE;
        Player_SetupItemCutsceneOrCUp(this, play);
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

        if (!Player_SetupForcePullOcarina(play, this) && !Player_SetupShootingGalleryPlay(play, this) && !Player_SetupCutscene(play, this)) {
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

        this->unk_88E = 10;
        return;
    }

    if (this->stateFlags1 & PLAYER_STATE1_RIDING_HORSE) {
        Player_RideHorse(this, play);
    } else if (Player_IsSwimming(this)) {
        Player_UpdateSwimIdle(this, play);
    } else if (!Player_IsUnfriendlyZTargeting(this) && LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->skelAnime.moveFlags != 0) {
            Player_EndAnimMovement(this);
            if ((this->talkActor->category == ACTORCAT_NPC) &&
                (this->heldItemActionParam != PLAYER_AP_FISHING_POLE)) {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_0031A0);
            } else {
                Player_PlayAnimLoop(play, this, Player_GetStandingStillAnim(this));
            }
        } else {
            Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_0031A8);
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

    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION | PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;
    Player_PushPullSetPositionAndYaw(play, this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_SetupPushPullWallIdle(play, this)) {
            Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
            pushPullDir = Player_GetPushPullDirection(this, &targetVelocity, &targetYaw);
            if (pushPullDir > 0) {
                Player_SetupPushWall(this, play);
            } else if (pushPullDir < 0) {
                Player_SetupPullWall(this, play);
            }
        }
    }
}

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

    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION | PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;

    if (Player_LoopAnimContinuously(play, this, &gPlayerAnim_003108)) {
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
        }
    }

    Player_PlayAnimSfx(this, sPushPullWallAnimSfx);
    Player_PushPullSetPositionAndYaw(play, this);

    if (!Player_SetupPushPullWallIdle(play, this)) {
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        pushPullDir = Player_GetPushPullDirection(this, &targetVelocity, &targetYaw);
        if (pushPullDir < 0) {
            Player_SetupPullWall(this, play);
        } else if (pushPullDir == 0) {
            Player_StartGrabPushPullWall(this, &gPlayerAnim_0030E0, play);
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

    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_PULL_OBJECT, this->modelAnimType);
    this->stateFlags2 |= PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION | PLAYER_STATE2_ENABLE_PUSH_PULL_CAM;

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
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        pushPullDir = Player_GetPushPullDirection(this, &targetVelocity, &targetYaw);
        if (pushPullDir > 0) {
            Player_SetupPushWall(this, play);
        } else if (pushPullDir == 0) {
            Player_StartGrabPushPullWall(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_PUSH_OBJECT, this->modelAnimType), play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_MOVING_PUSH_PULL_WALL;
        }
    }

    if (this->stateFlags2 & PLAYER_STATE2_MOVING_PUSH_PULL_WALL) {
        floorPosY = Player_RaycastFloorWithOffset2(play, this, &sPullWallRaycastOffset, &raycastPos) - this->actor.world.pos.y;
        if (fabsf(floorPosY) < 20.0f) {
            lineCheckPos.x = this->actor.world.pos.x;
            lineCheckPos.z = this->actor.world.pos.z;
            lineCheckPos.y = raycastPos.y;
            if (!BgCheck_EntityLineTest1(&play->colCtx, &lineCheckPos, &raycastPos, &posResult, &colPoly, true, false, false, true, &polyBgId)) {
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
        anim = (this->climbStatus > PLAYER_CLIMBSTATUS_MOVING_UP) ? &gPlayerAnim_002F28 : GET_PLAYER_ANIM(PLAYER_ANIMGROUP_CLIMBING_IDLE, this->modelAnimType); Player_PlayAnimLoop(play, this, anim);
        // clang-format on
    } else if (this->climbStatus == PLAYER_CLIMBSTATUS_MOVING_UP) {
        if (this->skelAnime.animation == &gPlayerAnim_002F10) {
            temp = 11.0f;
        } else {
            temp = 1.0f;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, temp)) {
            Player_PlayMoveSfx(this, NA_SE_PL_WALK_GROUND);
            if (this->skelAnime.animation == &gPlayerAnim_002F10) {
                this->climbStatus = PLAYER_CLIMBSTATUS_KNOCKED_DOWN;
            } else {
                this->climbStatus = PLAYER_CLIMBSTATUS_MOVING_DOWN;
            }
        }
    }

    Math_ScaledStepToS(&this->actor.shape.rot.y, this->currentYaw, 0x800);

    if (this->climbStatus != PLAYER_CLIMBSTATUS_MOVING_UP) {
        Player_SetOrGetVelocityAndYaw(this, &targetVelocity, &targetYaw, 0.0f, play);
        if (this->analogStickInputs[this->inputFrameCounter] >= 0) {
            if (this->climbStatus > PLAYER_CLIMBSTATUS_MOVING_UP) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_KNOCKED_FROM_CLIMBING, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_CLIMBING, this->modelAnimType);
            }
            Player_SetupClimbOntoLedge(this, anim, play);
            return;
        }

        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) || (this->actor.shape.feetFloorFlag != 0)) {
            Player_SetLedgeGrabPosition(this);
            if (this->unk_84F < 0) {
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
    func_8002F7DC(&this->actor, (this->unk_84F != 0) ? NA_SE_PL_WALK_WALL : NA_SE_PL_WALK_LADDER);
}

void Player_ClimbDownFromLedge(Player* this, PlayState* play) {
    static Vec3f raycastOffset = { 0.0f, 0.0f, 26.0f };
    s32 sp84;
    s32 sp80;
    f32 phi_f0;
    f32 phi_f2;
    Vec3f sp6C;
    s32 sp68;
    Vec3f raycastPos;
    f32 floorPosY;
    LinkAnimationHeader* anim1;
    LinkAnimationHeader* anim2;

    sp84 = sControlInput->rel.stick_y;
    sp80 = sControlInput->rel.stick_x;

    this->fallStartHeight = this->actor.world.pos.y;
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if ((this->unk_84F != 0) && (ABS(sp84) < ABS(sp80))) {
        phi_f0 = ABS(sp80) * 0.0325f;
        sp84 = 0;
    } else {
        phi_f0 = ABS(sp84) * 0.05f;
        sp80 = 0;
    }

    if (phi_f0 < 1.0f) {
        phi_f0 = 1.0f;
    } else if (phi_f0 > 3.35f) {
        phi_f0 = 3.35f;
    }

    if (this->skelAnime.playSpeed >= 0.0f) {
        phi_f2 = 1.0f;
    } else {
        phi_f2 = -1.0f;
    }

    this->skelAnime.playSpeed = phi_f2 * phi_f0;

    if (this->genericTimer >= 0) {
        if ((this->actor.wallPoly != NULL) && (this->actor.wallBgId != BGCHECK_SCENE)) {
            DynaPolyActor* wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
            if (wallPolyActor != NULL) {
                Math_Vec3f_Diff(&wallPolyActor->actor.world.pos, &wallPolyActor->actor.prevPos, &sp6C);
                Math_Vec3f_Sum(&this->actor.world.pos, &sp6C, &this->actor.world.pos);
            }
        }

        Actor_UpdateBgCheckInfo(play, &this->actor, 26.0f, 6.0f, this->ageProperties->ceilingCheckHeight,
                                UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
        Player_SetPositionAndYawOnClimbWall(play, this, 26.0f, this->ageProperties->unk_3C, 50.0f, -20.0f);
    }

    if ((this->genericTimer < 0) || !func_8083FBC0(this, play)) {
        if (LinkAnimation_Update(play, &this->skelAnime) != 0) {
            if (this->genericTimer < 0) {
                this->genericTimer = ABS(this->genericTimer) & 1;
                return;
            }

            if (sp84 != 0) {
                sp68 = this->unk_84F + this->genericTimer;

                if (sp84 > 0) {
                    raycastOffset.y = this->ageProperties->unk_40;
                    floorPosY = Player_RaycastFloorWithOffset2(play, this, &raycastOffset, &raycastPos);

                    if (this->actor.world.pos.y < floorPosY) {
                        if (this->unk_84F != 0) {
                            this->actor.world.pos.y = floorPosY;
                            this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
                            Player_SetupGrabLedge(play, this, this->actor.wallPoly, this->ageProperties->unk_3C,
                                          &gPlayerAnim_003000);
                            this->currentYaw += 0x8000;
                            this->actor.shape.rot.y = this->currentYaw;
                            Player_SetupClimbOntoLedge(this, &gPlayerAnim_003000, play);
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
                        if (this->unk_84F != 0) {
                            func_8083FB7C(this, play);
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
                        anim1 = this->ageProperties->verticalClimbAnim[sp68];
                        LinkAnimation_Change(play, &this->skelAnime, anim1, -1.0f, Animation_GetLastFrame(anim1), 0.0f,
                                             ANIMMODE_ONCE, 0.0f);
                    }
                }
                this->genericTimer ^= 1;
            } else {
                if ((this->unk_84F != 0) && (sp80 != 0)) {
                    anim2 = this->ageProperties->horizontalClimbAnim[this->genericTimer];

                    if (sp80 > 0) {
                        this->skelAnime.prevTransl = this->ageProperties->unk_7A[this->genericTimer];
                        Player_PlayAnimOnce(play, this, anim2);
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_86[this->genericTimer];
                        LinkAnimation_Change(play, &this->skelAnime, anim2, -1.0f, Animation_GetLastFrame(anim2), 0.0f,
                                             ANIMMODE_ONCE, 0.0f);
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

static f32 D_80854898[] = { 10.0f, 20.0f };
static f32 D_808548A0[] = { 40.0f, 50.0f };

static PlayerAnimSfxEntry sClimbLadderAnimSfx[] = {
    { NA_SE_PL_WALK_LADDER, 0x80A },
    { NA_SE_PL_WALK_LADDER, 0x814 },
    { NA_SE_PL_WALK_LADDER, -0x81E },
};

void func_8084C5F8(Player* this, PlayState* play) {
    s32 temp;
    f32* sp38;
    CollisionPoly* sp34;
    s32 sp30;
    Vec3f sp24;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    temp = Player_SetupInterruptAction(play, this, &this->skelAnime, 4.0f);

    if (temp == 0) {
        this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
        return;
    }

    if ((temp > 0) || LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_CLIMBING;
        return;
    }

    sp38 = D_80854898;

    if (this->genericTimer != 0) {
        Player_PlayAnimSfx(this, sClimbLadderAnimSfx);
        sp38 = D_808548A0;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, sp38[0]) || LinkAnimation_OnFrame(&this->skelAnime, sp38[1])) {
        sp24.x = this->actor.world.pos.x;
        sp24.y = this->actor.world.pos.y + 20.0f;
        sp24.z = this->actor.world.pos.z;
        if (BgCheck_EntityRaycastFloor3(&play->colCtx, &sp34, &sp30, &sp24) != 0.0f) {
            this->moveSfxType = SurfaceType_GetMoveSfx(&play->colCtx, sp34, sp30);
            Player_PlayLandingSfx(this);
        }
    }
}

static PlayerAnimSfxEntry D_808548B4[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3028 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3030 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3038 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3040 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3048 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3050 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3058 },  { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3060 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3068 },
};

void func_8084C760(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_EXITING_SCENE)) {
            if (this->skelAnime.moveFlags != 0) {
                this->skelAnime.moveFlags = 0;
                return;
            }

            if (!func_8083F570(this, play)) {
                this->linearVelocity = sControlInput->rel.stick_y * 0.03f;
            }
        }
        return;
    }

    Player_PlayAnimSfx(this, D_808548B4);
}

static PlayerAnimSfxEntry D_808548D8[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x300A },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3012 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x301A },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3022 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3034 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x303C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3044 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x304C },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3054 },
};

void func_8084C81C(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_INSIDE_CRAWLSPACE;
        return;
    }

    Player_PlayAnimSfx(this, D_808548D8);
}

static Vec3f D_808548FC[] = {
    { 40.0f, 0.0f, 0.0f },
    { -40.0f, 0.0f, 0.0f },
};

static Vec3f D_80854914[] = {
    { 60.0f, 20.0f, 0.0f },
    { -60.0f, 20.0f, 0.0f },
};

static Vec3f D_8085492C[] = {
    { 60.0f, -20.0f, 0.0f },
    { -60.0f, -20.0f, 0.0f },
};

s32 func_8084C89C(PlayState* play, Player* this, s32 arg2, f32* floorPosY) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 sp50;
    f32 sp4C;
    Vec3f raycastPos;
    Vec3f sp34;
    CollisionPoly* sp30;
    s32 sp2C;

    sp50 = rideActor->actor.world.pos.y + 20.0f;
    sp4C = rideActor->actor.world.pos.y - 20.0f;

    *floorPosY = Player_RaycastFloorWithOffset2(play, this, &D_808548FC[arg2], &raycastPos);

    return (sp4C < *floorPosY) && (*floorPosY < sp50) && !Player_WallLineTestWithOffset(play, this, &D_80854914[arg2], &sp30, &sp2C, &sp34) &&
           !Player_WallLineTestWithOffset(play, this, &D_8085492C[arg2], &sp30, &sp2C, &sp34);
}

s32 func_8084C9BC(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    s32 sp38;
    f32 sp34;

    if (this->genericTimer < 0) {
        this->genericTimer = 99;
    } else {
        sp38 = (this->mountSide < 0) ? 0 : 1;
        if (!func_8084C89C(play, this, sp38, &sp34)) {
            sp38 ^= 1;
            if (!func_8084C89C(play, this, sp38, &sp34)) {
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
                Player_SetActionFuncPreserveMoveFlags(play, this, func_8084D3E4, 0);
                this->unk_878 = sp34 - rideActor->actor.world.pos.y;
                Player_PlayAnimOnce(play, this, (this->mountSide < 0) ? &gPlayerAnim_003390 : &gPlayerAnim_0033A0);
                return 1;
            }
        }
    }

    return 0;
}

void func_8084CBF4(Player* this, f32 arg1, f32 arg2) {
    f32 temp;
    f32 dir;

    if ((this->unk_878 != 0.0f) && (arg2 <= this->skelAnime.curFrame)) {
        if (arg1 < fabsf(this->unk_878)) {
            if (this->unk_878 >= 0.0f) {
                dir = 1;
            } else {
                dir = -1;
            }
            temp = dir * arg1;
        } else {
            temp = this->unk_878;
        }
        this->actor.world.pos.y += temp;
        this->unk_878 -= temp;
    }
}

static LinkAnimationHeader* D_80854944[] = {
    &gPlayerAnim_003370,
    &gPlayerAnim_003368,
    &gPlayerAnim_003380,
    &gPlayerAnim_003358,
    &gPlayerAnim_003338,
    &gPlayerAnim_003348,
    &gPlayerAnim_003350,
    NULL,
    NULL,
};

static LinkAnimationHeader* D_80854968[] = {
    &gPlayerAnim_003388,
    &gPlayerAnim_003388,
    &gPlayerAnim_003388,
    &gPlayerAnim_003360,
    &gPlayerAnim_003340,
    &gPlayerAnim_003340,
    &gPlayerAnim_003340,
    NULL,
    NULL,
};

static LinkAnimationHeader* D_8085498C[] = {
    &gPlayerAnim_0033C8,
    &gPlayerAnim_0033B8,
    &gPlayerAnim_0033C0,
};

static u8 D_80854998[2][2] = {
    { 32, 58 },
    { 25, 42 },
};

static Vec3s D_8085499C = { -69, 7146, -266 };

static PlayerAnimSfxEntry sHorseIdleAnimSfx[] = {
    { NA_SE_PL_CALM_HIT, 0x830 }, { NA_SE_PL_CALM_HIT, 0x83A },  { NA_SE_PL_CALM_HIT, 0x844 },
    { NA_SE_PL_CALM_PAT, 0x85C }, { NA_SE_PL_CALM_PAT, 0x86E },  { NA_SE_PL_CALM_PAT, 0x87E },
    { NA_SE_PL_CALM_PAT, 0x884 }, { NA_SE_PL_CALM_PAT, -0x888 },
};

void Player_RideHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    u8* arr;

    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    func_8084CBF4(this, 1.0f, 10.0f);

    if (this->genericTimer == 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->skelAnime.animation = &gPlayerAnim_0033B8;
            this->genericTimer = 99;
            return;
        }

        arr = D_80854998[(this->mountSide < 0) ? 0 : 1];

        if (LinkAnimation_OnFrame(&this->skelAnime, arr[0])) {
            func_8002F7DC(&this->actor, NA_SE_PL_CLIMB_CLIFF);
            return;
        }

        if (LinkAnimation_OnFrame(&this->skelAnime, arr[1])) {
            func_8002DE74(play, this);
            func_8002F7DC(&this->actor, NA_SE_PL_SIT_ON_HORSE);
            return;
        }

        return;
    }

    func_8002DE74(play, this);
    this->skelAnime.prevTransl = D_8085499C;

    if ((rideActor->animationIdx != this->genericTimer) && ((rideActor->animationIdx >= 2) || (this->genericTimer >= 2))) {
        if ((this->genericTimer = rideActor->animationIdx) < 2) {
            f32 rand = Rand_ZeroOne();
            s32 temp = 0;

            this->genericTimer = 1;

            if (rand < 0.1f) {
                temp = 2;
            } else if (rand < 0.2f) {
                temp = 1;
            }
            Player_PlayAnimOnce(play, this, D_8085498C[temp]);
        } else {
            this->skelAnime.animation = D_80854944[this->genericTimer - 2];
            Animation_SetMorph(play, &this->skelAnime, 8.0f);
            if (this->genericTimer < 4) {
                Player_SetupHeldItemUpperActionFunc(play, this);
                this->unk_84F = 0;
            }
        }
    }

    if (this->genericTimer == 1) {
        if ((D_808535E0 != 0) || Player_CheckActorTalkRequested(play)) {
            Player_PlayAnimOnce(play, this, &gPlayerAnim_0033C8);
        } else if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->genericTimer = 99;
        } else if (this->skelAnime.animation == &gPlayerAnim_0033B8) {
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
        this->unk_84F = 0;
    } else if ((this->genericTimer < 2) || (this->genericTimer >= 4)) {
        D_808535E0 = Player_SetupCurrentUpperAction(this, play);
        if (D_808535E0 != 0) {
            this->unk_84F = 0;
        }
    }

    this->actor.world.pos.x = rideActor->actor.world.pos.x + rideActor->riderPos.x;
    this->actor.world.pos.y = (rideActor->actor.world.pos.y + rideActor->riderPos.y) - 27.0f;
    this->actor.world.pos.z = rideActor->actor.world.pos.z + rideActor->riderPos.z;

    this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;

    if ((this->csMode != PLAYER_CSMODE_NONE) ||
        (!Player_CheckActorTalkRequested(play) && ((rideActor->actor.speedXZ != 0.0f) || !Player_SetupSpeakOrCheck(this, play)) &&
         !Player_SetupRollOrPutAway(this, play))) {
        if (D_808535E0 == 0) {
            if (this->unk_84F != 0) {
                if (LinkAnimation_Update(play, &this->skelAnimeUpper)) {
                    rideActor->stateFlags &= ~ENHORSE_FLAG_8;
                    this->unk_84F = 0;
                }

                if (this->skelAnimeUpper.animation == &gPlayerAnim_0033B0) {
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
                    anim = &gPlayerAnim_0033B0;
                } else if (EN_HORSE_CHECK_2(rideActor)) {
                    if ((this->genericTimer >= 2) && (this->genericTimer != 99)) {
                        anim = D_80854968[this->genericTimer - 2];
                    }
                }

                if (anim != NULL) {
                    LinkAnimation_PlayOnce(play, &this->skelAnimeUpper, anim);
                    this->unk_84F = 1;
                }
            }
        }

        if (this->stateFlags1 & PLAYER_STATE1_IN_FIRST_PERSON_MODE) {
            if (!Player_SetupCameraMode(play, this) || CHECK_BTN_ANY(sControlInput->press.button, BTN_A) ||
                Player_IsZTargeting(this)) {
                this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
                this->stateFlags1 &= ~PLAYER_STATE1_IN_FIRST_PERSON_MODE;
            } else {
                this->upperBodyRot.y = func_8084ABD8(play, this, 1, -5000) - this->actor.shape.rot.y;
                this->upperBodyRot.y += 5000;
                this->unk_6B0 = -5000;
            }
            return;
        }

        if ((this->csMode != PLAYER_CSMODE_NONE) || (!func_8084C9BC(this, play) && !Player_SetupItemCutsceneOrCUp(this, play))) {
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
                    this->upperBodyRot.y = func_8084ABD8(play, this, 1, -5000) - this->actor.shape.rot.y;
                    this->upperBodyRot.y += 5000;
                    this->unk_6B0 = -5000;
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

void func_8084D3E4(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    func_8084CBF4(this, 1.0f, 10.0f);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        EnHorse* rideActor = (EnHorse*)this->rideActor;

        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_RIDING_HORSE;
        this->actor.parent = NULL;
        AREG(6) = 0;

        if (Flags_GetEventChkInf(EVENTCHKINF_18) || (DREG(1) != 0)) {
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
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_0032F0);
}

void func_8084D5CC(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, func_8084DAB4, 0);
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_0032F0);
}

void Player_UpdateSwimIdle(Player* this, PlayState* play) {
    f32 swimVelocity;
    s16 swimYaw;

    Player_LoopAnimContinuously(play, this, &gPlayerAnim_003328);
    Player_SetVerticalWaterVelocity(this);

    if (!Player_CheckActorTalkRequested(play) && !Player_SetupSubAction(play, this, sSwimSubActions, 1) &&
        !func_8083D12C(play, this, sControlInput)) {
        if (this->attentionMode != PLAYER_ATTENTIONMODE_C_UP) {
            this->attentionMode = PLAYER_ATTENTIONMODE_NONE;
        }

        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            swimVelocity = 0.0f;
            swimYaw = this->actor.shape.rot.y;

            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SHORT_JUMP_LANDING, this->modelAnimType), play);
                Player_PlayLandingSfx(this);
            }
        } else {
            Player_SetOrGetVelocityAndYaw(this, &swimVelocity, &swimYaw, 0.0f, play);

            if (swimVelocity != 0.0f) {
                s16 temp = this->actor.shape.rot.y - swimYaw;

                if ((ABS(temp) > 0x6000) && !Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                    return;
                }

                if (Player_IsZTargetingSetupStartUnfriendly(this)) {
                    func_8084D5CC(play, this);
                } else {
                    Player_SetupSwim(play, this, swimYaw);
                }
            }
        }

        Player_UpdateSwimMovement(this, &this->linearVelocity, swimVelocity, swimYaw);
    }
}

void Player_SpawnSwimming(Player* this, PlayState* play) {
    if (!Player_SetupItemCutsceneOrCUp(this, play)) {
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

    if (!Player_SetupSubAction(play, this, sSwimSubActions, 1) && !func_8083D12C(play, this, sControlInput)) {
        Player_SetOrGetVelocityAndYaw(this, &swimVelocity, &swimYaw, 0.0f, play);

        yawDiff = this->actor.shape.rot.y - swimYaw;
        if ((swimVelocity == 0.0f) || (ABS(yawDiff) > DEG_TO_BINANG(135.0f)) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
            Player_SetupSwimIdle(play, this);
        } else if (Player_IsZTargetingSetupStartUnfriendly(this)) {
            func_8084D5CC(play, this);
        }

        Player_SetupSwimMovement(this, &this->linearVelocity, swimVelocity, swimYaw);
    }
}

s32 func_8084D980(PlayState* play, Player* this, f32* arg2, s16* arg3) {
    LinkAnimationHeader* anim;
    s16 temp1;
    s32 temp2;

    temp1 = this->currentYaw - *arg3;

    if (ABS(temp1) > DEG_TO_BINANG(135.0f)) {
        anim = &gPlayerAnim_003328;

        if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
            this->currentYaw = *arg3;
        } else {
            *arg2 = 0.0f;
            *arg3 = this->currentYaw;
        }
    } else {
        temp2 = Player_GetFriendlyZTargetingMoveDirection(this, arg2, arg3, play);

        if (temp2 > 0) {
            anim = &gPlayerAnim_0032F0;
        } else if (temp2 < 0) {
            anim = &gPlayerAnim_0032D8;
        } else if ((temp1 = this->actor.shape.rot.y - *arg3) > 0) {
            anim = &gPlayerAnim_0032D0;
        } else {
            anim = &gPlayerAnim_0032C8;
        }
    }

    if (anim != this->skelAnime.animation) {
        Player_ChangeAnimLongMorphLoop(play, this, anim);
        return 1;
    }

    return 0;
}

void func_8084DAB4(Player* this, PlayState* play) {
    f32 swimVelocity;
    s16 swimYaw;

    Player_PlaySwimAnim(play, this, sControlInput, this->linearVelocity);
    Player_SetVerticalWaterVelocity(this);

    if (!Player_SetupSubAction(play, this, sSwimSubActions, 1) && !func_8083D12C(play, this, sControlInput)) {
        Player_SetOrGetVelocityAndYaw(this, &swimVelocity, &swimYaw, 0.0f, play);

        if (swimVelocity == 0.0f) {
            Player_SetupSwimIdle(play, this);
        } else if (!Player_IsZTargetingSetupStartUnfriendly(this)) {
            Player_SetupSwim(play, this, swimYaw);
        } else {
            func_8084D980(play, this, &swimVelocity, &swimYaw);
        }

        Player_SetupSwimMovement(this, &this->linearVelocity, swimVelocity, swimYaw);
    }
}

void func_8084DBC4(PlayState* play, Player* this, f32 swimNextVelocity) {
    f32 swimBaseVelocity;
    s16 swimYaw;

    Player_SetOrGetVelocityAndYaw(this, &swimBaseVelocity, &swimYaw, 0.0f, play);
    Player_UpdateSwimMovement(this, &this->linearVelocity, swimBaseVelocity * 0.5f, swimYaw);
    Player_UpdateSwimMovement(this, &this->actor.velocity.y, swimNextVelocity, this->currentYaw);
}

void func_8084DC48(Player* this, PlayState* play) {
    f32 sp2C;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;
    this->actor.gravity = 0.0f;
    Player_SetupCurrentUpperAction(this, play);

    if (!Player_SetupItemCutsceneOrCUp(this, play)) {
        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            Player_SetupSwimIdle(play, this);
            return;
        }

        if (this->unk_84F == 0) {
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
            this->unk_6C2 = 16000;

            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) && !Player_SetupGetItemOrHoldBehavior(this, play) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
                (this->actor.yDistToWater < D_80854784[CUR_UPG_VALUE(UPG_SCALE)])) {
                func_8084DBC4(play, this, -2.0f);
            } else {
                this->unk_84F++;
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_003328);
            }
        } else if (this->unk_84F == 1) {
            LinkAnimation_Update(play, &this->skelAnime);
            Player_SetVerticalWaterVelocity(this);

            if (this->unk_6C2 < 10000) {
                this->unk_84F++;
                this->genericTimer = this->actor.yDistToWater;
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_0032F0);
            }
        } else if (!func_8083D12C(play, this, sControlInput)) {
            sp2C = (this->genericTimer * 0.018f) + 4.0f;

            if (this->stateFlags1 & PLAYER_STATE1_HOLDING_ACTOR) {
                sControlInput = NULL;
            }

            Player_PlaySwimAnim(play, this, sControlInput, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->unk_6C2, -10000, 800);

            if (sp2C > 8.0f) {
                sp2C = 8.0f;
            }

            func_8084DBC4(play, this, sp2C);
        }
    }
}

void func_8084DF6C(PlayState* play, Player* this) {
    this->giDrawIdPlusOne = 0;
    this->stateFlags1 &= ~(PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HOLDING_ACTOR);
    this->getItemId = GI_NONE;
    func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
}

void func_8084DFAC(PlayState* play, Player* this) {
    func_8084DF6C(play, this);
    Player_AddRootYawToShapeYaw(this);
    Player_SetupStandingStillNoMorph(this, play);
    this->currentYaw = this->actor.shape.rot.y;
}

s32 func_8084DFF4(PlayState* play, Player* this) {
    GetItemEntry* giEntry;
    s32 temp1;
    s32 temp2;

    if (this->getItemId == GI_NONE) {
        return 1;
    }

    if (this->unk_84F == 0) {
        giEntry = &sGetItemTable[this->getItemId - 1];
        this->unk_84F = 1;

        Message_StartTextbox(play, giEntry->textId, &this->actor);
        Item_Give(play, giEntry->itemId);

        if (((this->getItemId >= GI_RUPEE_GREEN) && (this->getItemId <= GI_RUPEE_RED)) ||
            ((this->getItemId >= GI_RUPEE_PURPLE) && (this->getItemId <= GI_RUPEE_GOLD)) ||
            ((this->getItemId >= GI_RUPEE_GREEN_LOSE) && (this->getItemId <= GI_RUPEE_PURPLE_LOSE)) ||
            (this->getItemId == GI_HEART)) {
            Audio_PlaySoundGeneral(NA_SE_SY_GET_BOXITEM, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
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
            if (this->getItemId == GI_GAUNTLETS_SILVER) {
                play->nextEntranceIndex = ENTR_SPOT11_0;
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

void func_8084E1EC(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) || func_8084DFF4(play, this)) {
            func_8084DF6C(play, this);
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

void func_8084E30C(Player* this, PlayState* play) {
    Player_SetVerticalWaterVelocity(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupSwimIdle(play, this);
    }

    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_SetupDrown(Player* this, PlayState* play) {
    Player_SetVerticalWaterVelocity(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndDie(play, this);
    }

    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

static s16 sWarpSongEntrances[] = {
    ENTR_SPOT05_2, ENTR_SPOT17_4, ENTR_SPOT06_8, ENTR_SPOT11_5, ENTR_SPOT02_7, ENTR_TOKINOMA_7,
};

void func_8084E3C4(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_0030A8);
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
        } else if (!Player_SetupItemCutsceneOrCUp(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_003098, play);
        }

        this->stateFlags2 &= ~(PLAYER_STATE2_NEAR_OCARINA_ACTOR | PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR | PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR);
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

        if (Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0xF) == NULL) {
            Environment_WarpSongLeave(play);
        }

        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    }
}

void Player_ThrowDekuNut(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_003050, play);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Inventory_ChangeAmmo(ITEM_NUT, -1);
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ARROW, this->bodyPartsPos[PLAYER_BODYPART_R_HAND].x,
                    this->bodyPartsPos[PLAYER_BODYPART_R_HAND].y, this->bodyPartsPos[PLAYER_BODYPART_R_HAND].z, 4000,
                    this->actor.shape.rot.y, 0, ARROW_NUT);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }

    Player_StepLinearVelocityToZero(this);
}

static PlayerAnimSfxEntry D_808549E0[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x3857 },
    { NA_SE_VO_LI_CLIMB_END, 0x2057 },
    { NA_SE_VO_LI_AUTO_JUMP, 0x2045 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x287B },
};

void func_8084E6D4(Player* this, PlayState* play) {
    s32 cond;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericTimer != 0) {
            if (this->genericTimer >= 2) {
                this->genericTimer--;
            }

            if (func_8084DFF4(play, this) && (this->genericTimer == 1)) {
                cond = ((this->talkActor != NULL) && (this->exchangeItemId < 0)) ||
                       (this->stateFlags3 & PLAYER_STATE3_FORCE_PULL_OCARINA);

                if (cond || (gSaveContext.healthAccumulator == 0)) {
                    if (cond) {
                        func_8084DF6C(play, this);
                        this->exchangeItemId = EXCH_ITEM_NONE;

                        if (Player_SetupForcePullOcarina(play, this) == 0) {
                            Player_StartTalkingWithActor(play, this->talkActor);
                        }
                    } else {
                        func_8084DFAC(play, this);
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

            if (this->skelAnime.animation == &gPlayerAnim_002DF8) {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002788);
            } else {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002780);
            }

            this->genericTimer = 2;
            Player_SetCameraTurnAround(play, 9);
        }
    } else {
        if (this->genericTimer == 0) {
            if (!LINK_IS_ADULT) {
                Player_PlayAnimSfx(this, D_808549E0);
            }
            return;
        }

        if (this->skelAnime.animation == &gPlayerAnim_002788) {
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

static PlayerAnimSfxEntry D_808549F4[] = {
    { NA_SE_VO_LI_AUTO_JUMP, 0x2005 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x280F },
};

void func_8084E9AC(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F == 0) {
            if (DECR(this->genericTimer) == 0) {
                this->unk_84F = 1;
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
            Player_PlayAnimSfx(this, D_808549F4);
        } else {
            Player_PlaySwingSwordSfx(this);
        }
    }
}

static u8 D_808549FC[] = {
    0x01, 0x03, 0x02, 0x04, 0x04,
};

void func_8084EAC0(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->genericTimer == 0) {
            if (this->itemActionParam == PLAYER_AP_BOTTLE_POE) {
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
                s32 sp28 = D_808549FC[this->itemActionParam - PLAYER_AP_BOTTLE_POTION_RED];

                if (sp28 & 1) {
                    gSaveContext.healthAccumulator = 0x140;
                }

                if (sp28 & 2) {
                    Magic_Fill(play);
                }

                if (sp28 & 4) {
                    gSaveContext.healthAccumulator = 0x50;
                }
            }

            Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_002670);
            this->genericTimer = 1;
            return;
        }

        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    } else if (this->genericTimer == 1) {
        if ((gSaveContext.healthAccumulator == 0) && (gSaveContext.magicState != MAGIC_STATE_FILL)) {
            Player_ChangeAnimSlowedMorphToLastFrame(play, this, &gPlayerAnim_002660);
            this->genericTimer = 2;
            Player_UpdateBottleHeld(play, this, ITEM_BOTTLE, PLAYER_AP_BOTTLE);
        }
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DRINK - SFX_FLAG);
    } else if ((this->genericTimer == 2) && LinkAnimation_OnFrame(&this->skelAnime, 29.0f)) {
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
    }
}

static BottleCatchInfo sBottleCatchInfos[] = {
    { ACTOR_EN_ELF, ITEM_FAIRY, PLAYER_AP_BOTTLE_FAIRY, 0x46 },
    { ACTOR_EN_FISH, ITEM_FISH, PLAYER_AP_BOTTLE_FISH, 0x47 },
    { ACTOR_EN_ICE_HONO, ITEM_BLUE_FIRE, PLAYER_AP_BOTTLE_FIRE, 0x5D },
    { ACTOR_EN_INSECT, ITEM_BUG, PLAYER_AP_BOTTLE_BUG, 0x7A },
};

void Player_SwingBottle(Player* this, PlayState* play) {
    struct_80854554* sp24;
    BottleCatchInfo* catchInfo;
    s32 temp;
    s32 i;

    sp24 = &sBottleSwingAnims[this->genericTimer];
    Player_StepLinearVelocityToZero(this);

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F != 0) {
            if (this->genericTimer == 0) {
                Message_StartTextbox(play, sBottleCatchInfos[this->unk_84F - 1].textId, &this->actor);
                Audio_PlayFanfare(NA_BGM_ITEM_GET | 0x900);
                this->genericTimer = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->unk_84F = 0;
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
        }
    } else {
        if (this->unk_84F == 0) {
            temp = this->skelAnime.curFrame - sp24->unk_08;

            if (temp >= 0) {
                if (sp24->unk_09 >= temp) {
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
                            this->unk_84F = i + 1;
                            this->genericTimer = 0;
                            this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                            this->interactRangeActor->parent = &this->actor;
                            Player_UpdateBottleHeld(play, this, catchInfo->itemId, ABS(catchInfo->actionParam));
                            Player_PlayAnimOnceSlowed(play, this, sp24->unk_04);
                            Player_SetCameraTurnAround(play, 4);
                        }
                    }
                }
            }
        }
    }

    //! @bug If the animation is changed at any point above (such as by Player_SetupStandingStillNoMorph() or func_808322D0()), it will
    //! change the curFrame to 0. This causes this flag to be set for one frame, at a time when it does not look like
    //! Player is swinging the bottle.
    if (this->skelAnime.curFrame <= 7.0f) {
        this->stateFlags1 |= PLAYER_STATE1_SWINGING_BOTTLE;
    }
}

static Vec3f sBottleFairyPosOffset = { 0.0f, 0.0f, 5.0f };

void func_8084EED8(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        return;
    }

    if (LinkAnimation_OnFrame(&this->skelAnime, 37.0f)) {
        Player_SpawnFairy(play, this, &this->leftHandPos, &sBottleFairyPosOffset, FAIRY_REVIVE_BOTTLE);
        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE, PLAYER_AP_BOTTLE);
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
        BottleDropInfo* dropInfo = &D_80854A28[this->itemActionParam - PLAYER_AP_BOTTLE_FISH];

        Actor_Spawn(&play->actorCtx, play, dropInfo->actorId,
                    (Math_SinS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.x, this->leftHandPos.y,
                    (Math_CosS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.z, 0x4000, this->actor.shape.rot.y,
                    0, dropInfo->actorParams);

        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE, PLAYER_AP_BOTTLE);
        return;
    }

    Player_PlayAnimSfx(this, sBottleDropAnimSfx);
}

static PlayerAnimSfxEntry sExchangeItemAnimSfx[] = {
    { NA_SE_PL_PUT_OUT_ITEM, -0x81E },
};

void func_8084F104(Player* this, PlayState* play) {
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

            if (this->itemActionParam >= PLAYER_AP_LETTER_ZELDA) {
                if (giEntry->gi >= 0) {
                    this->giDrawIdPlusOne = giEntry->gi;
                } else {
                    this->giDrawIdPlusOne = -giEntry->gi;
                }
            }

            if (this->genericTimer == 0) {
                Message_StartTextbox(play, this->actor.textId, &this->actor);

                if ((this->itemActionParam == PLAYER_AP_CHICKEN) || (this->itemActionParam == PLAYER_AP_POCKET_CUCCO)) {
                    func_8002F7DC(&this->actor, NA_SE_EV_CHICKEN_CRY_M);
                }

                this->genericTimer = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->actor.flags &= ~ACTOR_FLAG_8;
                this->giDrawIdPlusOne = 0;

                if (this->unk_84F == 1) {
                    Player_PlayAnimOnce(play, this, &gPlayerAnim_002698);
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

    if ((this->unk_84F == 0) && (this->targetActor != NULL)) {
        this->currentYaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_RestrainedByEnemy(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_003128);
    }

    if (Player_MashTimerThresholdExceeded(this, 0, 100)) {
        Player_SetupStandingStillType(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_RESTRAINED_BY_ENEMY;
    }
}

void func_8084F390(Player* this, PlayState* play) {
    CollisionPoly* floorPoly;
    f32 sp50;
    f32 sp4C;
    f32 sp48;
    s16 downwardSlopeYaw;
    s16 sp44;
    Vec3f slopeNormal;

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING | PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION;
    LinkAnimation_Update(play, &this->skelAnime);
    func_8084269C(play, this);
    func_800F4138(&this->actor.projectedPos, NA_SE_PL_SLIP_LEVEL - SFX_FLAG, this->actor.speedXZ);

    if (Player_SetupItemCutsceneOrCUp(this, play) == 0) {
        floorPoly = this->actor.floorPoly;

        if (floorPoly == NULL) {
            Player_SetupFallFromLedge(this, play);
            return;
        }

        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);

        sp44 = downwardSlopeYaw;
        if (this->unk_84F != 0) {
            sp44 = downwardSlopeYaw + 0x8000;
        }

        if (this->linearVelocity < 0) {
            downwardSlopeYaw += 0x8000;
        }

        sp50 = (1.0f - slopeNormal.y) * 40.0f;
        sp50 = CLAMP(sp50, 0, 10.0f);
        sp4C = (sp50 * sp50) * 0.015f;
        sp48 = slopeNormal.y * 0.01f;

        if (SurfaceType_GetSlope(&play->colCtx, floorPoly, this->actor.floorBgId) != 1) {
            sp50 = 0;
            sp48 = slopeNormal.y * 10.0f;
        }

        if (sp4C < 1.0f) {
            sp4C = 1.0f;
        }

        if (Math_AsymStepToF(&this->linearVelocity, sp50, sp4C, sp48) && (sp50 == 0)) {
            LinkAnimationHeader* anim;

            if (this->unk_84F == 0) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_SLIDING_DOWN_SLOPE, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_END_SLIDING_DOWN_SLOPE, this->modelAnimType);
            }
            Player_SetupReturnToStandStillSetAnim(this, anim, play);
        }

        Math_SmoothStepToS(&this->currentYaw, downwardSlopeYaw, 10, 4000, 800);
        Math_ScaledStepToS(&this->actor.shape.rot.y, sp44, 2000);
    }
}

void func_8084F608(Player* this, PlayState* play) {
    if ((DECR(this->genericTimer) == 0) && Player_SetupCutscene(play, this)) {
        func_80852280(play, this, NULL);
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        Player_StartCutscene(this, play);
    }
}

void func_8084F698(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, func_8084F608, 0);
    this->genericTimer = 40;
    Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0x10);
}

void func_8084F710(Player* this, PlayState* play) {
    s32 pad;

    if ((this->unk_84F != 0) && (play->csCtx.frames < 0x131)) {
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
                if ((play->sceneNum == SCENE_SPOT04) && Player_SetupCutscene(play, this)) {
                    return;
                }
                Player_SetupStandingStillMorph(this, play);
            }
        }
        Math_SmoothStepToF(&this->actor.velocity.y, 2.0f, 0.3f, 8.0f, 0.5f);
    }

    if ((play->sceneNum == SCENE_KENJYANOMA) && Player_SetupCutscene(play, this)) {
        return;
    }

    if ((play->csCtx.state != CS_STATE_IDLE) && (play->csCtx.linkAction != NULL)) {
        f32 sp28 = this->actor.world.pos.y;
        func_808529D0(play, this, play->csCtx.linkAction);
        this->actor.world.pos.y = sp28;
    }
}

void func_8084F88C(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);

    if ((this->genericTimer++ > 8) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {

        if (this->unk_84F != 0) {
            if (play->sceneNum == 9) {
                Play_TriggerRespawn(play);
                play->nextEntranceIndex = ENTR_ICE_DOUKUTO_0;
            } else if (this->unk_84F < 0) {
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

void func_8084F9C0(Player* this, PlayState* play) {
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

    this->upperBodyRot.y = func_8084ABD8(play, this, 1, 0) - this->actor.shape.rot.y;
    this->lookFlags |= PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y;

    if (play->shootingGalleryStatus < 0) {
        play->shootingGalleryStatus++;
        if (play->shootingGalleryStatus == 0) {
            Player_ClearLookAndAttention(this, play);
        }
    }
}

void Player_FrozenInIce(Player* this, PlayState* play) {
    if (this->unk_84F >= 0) {
        if (this->unk_84F < 6) {
            this->unk_84F++;
        }

        if (Player_MashTimerThresholdExceeded(this, 1, 100)) {
            this->unk_84F = -1;
            EffectSsIcePiece_SpawnBurst(play, &this->actor.world.pos, this->actor.scale.x);
            func_8002F7DC(&this->actor, NA_SE_PL_ICE_BROKEN);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_FROZEN_IN_ICE;
        }

        if ((play->gameplayFrames % 4) == 0) {
            Player_SetupInflictDamage(play, -1);
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

s32 func_8084FCAC(Player* this, PlayState* play) {
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

void func_8085002C(Player* this) {
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

s32 Player_SetupMeleeWeaponAttack(Player* this, PlayState* play) {
    if (Player_CanSwingBottleOrCastFishingRod(play, this) == false) {
        if (Player_CanJumpSlash(this) != false) {
            s32 attackAnim = Player_GetMeleeAttackAnim(this);

            Player_SetupMeleeWeaponAttackBehavior(play, this, attackAnim);

            if (attackAnim >= PLAYER_MWA_SPIN_ATTACK_1H) {
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
    struct_80854190* attackAnim = &D_80854190[this->meleeWeaponAnimation];

    this->stateFlags2 |= PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_Z_TARGETING;

    if (!func_80842DF4(play, this)) {
        Player_SetupMeleeAttack(this, 0.0f, attackAnim->startFrame, attackAnim->endFrame);

        if ((this->stateFlags2 & PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK) && (this->heldItemActionParam != PLAYER_AP_HAMMER) &&
            LinkAnimation_OnFrame(&this->skelAnime, 0.0f)) {
            this->linearVelocity = 15.0f;
            this->stateFlags2 &= ~PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK;
        }

        if (this->linearVelocity > 12.0f) {
            func_8084269C(play, this);
        }

        Math_StepToF(&this->linearVelocity, 0.0f, 5.0f);
        func_8083C50C(this);

        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (!Player_SetupMeleeWeaponAttack(this, play)) {
                u8 savedMoveFlags = this->skelAnime.moveFlags;
                LinkAnimationHeader* anim;

                if (Player_IsUnfriendlyZTargeting(this)) {
                    anim = attackAnim->fightEndAnim;
                } else {
                    anim = attackAnim->endAnim;
                }

                Player_InactivateMeleeWeapon(this);
                this->skelAnime.moveFlags = 0;

                if ((anim == &gPlayerAnim_002908) && (this->modelAnimType != PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON)) {
                    anim = &gPlayerAnim_002AC8;
                }

                Player_SetupReturnToStandStillSetAnim(this, anim, play);

                this->skelAnime.moveFlags = savedMoveFlags;
                this->stateFlags3 |= PLAYER_STATE3_ENDING_MELEE_ATTACK;
            }
        } else if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
            if ((this->meleeWeaponAnimation == PLAYER_MWA_HAMMER_FORWARD) ||
                (this->meleeWeaponAnimation == PLAYER_MWA_JUMPSLASH_FINISH)) {
                static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
                Vec3f shockwavePos;
                f32 sp2C;

                shockwavePos.y = Player_RaycastFloorWithOffset2(play, this, &sShockwaveRaycastPos, &shockwavePos);
                sp2C = this->actor.world.pos.y - shockwavePos.y;

                Math_ScaledStepToS(&this->actor.focus.rot.x, Math_Atan2S(45.0f, sp2C), 800);
                Player_UpdateLookAngles(this, true);

                if ((((this->meleeWeaponAnimation == PLAYER_MWA_HAMMER_FORWARD) &&
                      LinkAnimation_OnFrame(&this->skelAnime, 7.0f)) ||
                     ((this->meleeWeaponAnimation == PLAYER_MWA_JUMPSLASH_FINISH) &&
                      LinkAnimation_OnFrame(&this->skelAnime, 2.0f))) &&
                    (sp2C > -40.0f) && (sp2C < 40.0f)) {
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
            func_80088AF0(play);
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
    &gPlayerAnim_002CF8,
    &gPlayerAnim_002CE0,
    &gPlayerAnim_002D10,
};

static LinkAnimationHeader* sCastMagicSpellAnims[] = {
    &gPlayerAnim_002D00,
    &gPlayerAnim_002CE8,
    &gPlayerAnim_002D18,
};

static LinkAnimationHeader* sEndCastMagicSpellAnims[] = {
    &gPlayerAnim_002D08,
    &gPlayerAnim_002CF0,
    &gPlayerAnim_002D20,
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
            if ((this->itemActionParam == PLAYER_AP_NAYRUS_LOVE) || (gSaveContext.magicState == MAGIC_STATE_IDLE)) {
                Player_ReturnToStandStill(this, play);
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            if (this->genericTimer == 0) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sStartCastMagicSpellAnims[this->magicSpellType], 0.83f);

                if (Player_SpawnMagicSpellActor(play, this, this->magicSpellType) != NULL) {
                    this->stateFlags1 |= PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE | PLAYER_STATE1_IN_CUTSCENE;
                    if ((this->unk_84F != 0) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
                    }
                } else {
                    Magic_Reset(play);
                }
            } else {
                LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, sCastMagicSpellAnims[this->magicSpellType], 0.83f);

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
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sEndCastMagicSpellAnims[this->magicSpellType], 0.83f);
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
        Player_PlayAnimLoop(play, this, &gPlayerAnim_002C98);
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

    if ((this->skelAnime.animation != &gPlayerAnim_002C90) || (4.0f <= this->skelAnime.curFrame)) {
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

        LinkAnimation_BlendToJoint(play, &this->skelAnime, &gPlayerAnim_002C38, this->skelAnime.curFrame,
                                   (this->spinAttackTimer < 0.0f) ? &gPlayerAnim_002C18 : &gPlayerAnim_002C20, 5.0f,
                                   fabsf(this->spinAttackTimer), this->blendTable);
        LinkAnimation_BlendToMorph(play, &this->skelAnime, &gPlayerAnim_002C38, this->skelAnime.curFrame,
                                   (this->unk_85C < 0.0f) ? &gPlayerAnim_002C28 : &gPlayerAnim_002C10, 5.0f,
                                   fabsf(this->unk_85C), D_80858AD8);
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 0.5f);
    } else if (LinkAnimation_Update(play, &this->skelAnime)) {
        this->fishingState = 2;
        Player_PlayAnimLoop(play, this, &gPlayerAnim_002C38);
        this->genericTimer = 1;
    }

    Player_StepLinearVelocityToZero(this);

    if (this->fishingState == 0) {
        Player_SetupStandingStillMorph(this, play);
    } else if (this->fishingState == 3) {
        Player_SetActionFunc(play, this, Player_ReleaseCaughtFish, 0);
        Player_ChangeAnimMorphToLastFrame(play, this, &gPlayerAnim_002C00);
    }
}

void Player_ReleaseCaughtFish(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->fishingState == 0)) {
        Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_002C08, play);
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
    { 0, NULL },                                        // PLAYER_CSMODE_NONE
    { -1, Player_CutsceneSetupIdle },                   // PLAYER_CSMODE_IDLE
    { 2, &gPlayerAnim_002790 },                         // PLAYER_CSMODE_TURN_AROUND_SURPRISED_SHORT
    { 0, NULL },                                        // PLAYER_CSMODE_UNK_3
    { 0, NULL },                                        // PLAYER_CSMODE_UNK_4
    { 3, &gPlayerAnim_002740 },                         // PLAYER_CSMODE_SURPRISED
    { 0, NULL },                                        // PLAYER_CSMODE_UNK_6
    { 0, NULL },                                        // PLAYER_CSMODE_END
    { -1, Player_CutsceneSetupIdle },                   // PLAYER_CSMODE_WAIT
    { 2, &gPlayerAnim_002778 },                         // PLAYER_CSMODE_TURN_AROUND_SURPRISED_LONG
    { -1, Player_CutsceneSetupEnterWarp },              // PLAYER_CSMODE_ENTER_WARP
    { 3, &gPlayerAnim_002860 },                         // PLAYER_CSMODE_RAISED_BY_WARP
    { -1, Player_CutsceneSetupFightStance },            // PLAYER_CSMODE_FIGHT_STANCE
    { 7, &gPlayerAnim_002348 },                         // PLAYER_CSMODE_START_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_002350 },                         // PLAYER_CSMODE_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_002358 },                         // PLAYER_CSMODE_END_GET_SPIRITUAL_STONE
    { 5, &gPlayerAnim_0023B0 },                         // PLAYER_CSMODE_GET_UP_FROM_DEKU_TREE_STORY
    { 7, &gPlayerAnim_0023B8 },                         // PLAYER_CSMODE_SIT_LISTENING_TO_DEKU_TREE_STORY
    { -1, Player_CutsceneSetupSwordIntoPedestal },      // PLAYER_CSMODE_SWORD_INTO_PEDESTAL
    { 2, &gPlayerAnim_002728 },                         // PLAYER_CSMODE_REACT_TO_QUAKE
    { 2, &gPlayerAnim_002738 },                         // PLAYER_CSMODE_END_REACT_TO_QUAKE
    { 0, NULL },                                        // PLAYER_CSMODE_UNK_21
    { -1, Player_CutsceneSetupWarpToSages },            // PLAYER_CSMODE_WARP_TO_SAGES
    { 3, &gPlayerAnim_0027A8 },                         // PLAYER_CSMODE_LOOK_AT_SELF
    { 9, &gPlayerAnim_002DB0 },                         // PLAYER_CSMODE_KNOCKED_TO_GROUND
    { 2, &gPlayerAnim_002DC0 },                         // PLAYER_CSMODE_GET_UP_FROM_GROUND
    { -1, Player_CutsceneSetupStartPlayOcarina },       // PLAYER_CSMODE_START_PLAY_OCARINA
    { 2, &gPlayerAnim_003098 },                         // PLAYER_CSMODE_END_PLAY_OCARINA
    { 3, &gPlayerAnim_002780 },                         // PLAYER_CSMODE_GET_ITEM
    { -1, Player_CutsceneSetupIdle },                   // PLAYER_CSMODE_IDLE_2
    { 2, &gPlayerAnim_003088 },                         // PLAYER_CSMODE_DRAW_AND_BRANDISH_SWORD
    { 0, NULL },                                        // PLAYER_CSMODE_CLOSE_EYES
    { 0, NULL },                                        // PLAYER_CSMODE_OPEN_EYES
    { 5, &gPlayerAnim_002320 },                         // PLAYER_CSMODE_SURPRIED_STUMBLE_BACK_AND_FALL
    { -1, Player_CutsceneSetupSwimIdle },               // PLAYER_CSMODE_SURFACE_FROM_DIVE
    { -1, Player_CutsceneSetupGetItemInWater },         // PLAYER_CSMODE_GET_ITEM_IN_WATER
    { 5, &gPlayerAnim_002328 },                         // PLAYER_CSMODE_GENTLE_KNOCKBACK_INTO_SIT
    { 16, &gPlayerAnim_002F90 },                        // PLAYER_CSMODE_GRABBED_AND_CARRIED_BY_NECK
    { -1, Player_CutsceneSetupSleepingRestless },       // PLAYER_CSMODE_SLEEPING_RESTLESS
    { -1, Player_CutsceneSetupSleeping },               // PLAYER_CSMODE_SLEEPING
    { 6, &gPlayerAnim_002410 },                         // PLAYER_CSMODE_AWAKEN
    { 6, &gPlayerAnim_002418 },                         // PLAYER_CSMODE_GET_OFF_BED
    { -1, Player_CutsceneSetupBlownBackward },          // PLAYER_CSMODE_BLOWN_BACKWARD
    { 5, &gPlayerAnim_002390 },                         // PLAYER_CSMODE_STAND_UP_AND_WATCH
    { -1, func_808521F4 },                              // PLAYER_CSMODE_IDLE_3
    { -1, func_8085225C },                              // PLAYER_CSMODE_STOP
    { -1, func_80852280 },                              // PLAYER_CSMODE_STOP_2
    { 5, &gPlayerAnim_0023A0 },                         // PLAYER_CSMODE_LOOK_THROUGH_PEEPHOLE
    { 5, &gPlayerAnim_002368 },                         // PLAYER_CSMODE_STEP_BACK_CAUTIOUSLY
    { -1, Player_CutsceneSetupIdle },                   // PLAYER_CSMODE_IDLE_4
    { 5, &gPlayerAnim_002370 },                         // PLAYER_CSMODE_DRAW_SWORD_CHILD
    { 5, &gPlayerAnim_0027B0 },                         // PLAYER_CSMODE_JUMP_TO_ZELDAS_CRYSTAL
    { 5, &gPlayerAnim_0027B8 },                         // PLAYER_CSMODE_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL
    { 5, &gPlayerAnim_0027C0 },                         // PLAYER_CSMODE_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING
    { 3, &gPlayerAnim_002768 },                         // PLAYER_CSMODE_TURN_AROUND_SLOWLY
    { 3, &gPlayerAnim_0027D8 },                         // PLAYER_CSMODE_END_SHIELD_EYES_WITH_HAND
    { 4, &gPlayerAnim_0027E0 },                         // PLAYER_CSMODE_SHIELD_EYES_WITH_HAND
    { 3, &gPlayerAnim_002380 },                         // PLAYER_CSMODE_LOOK_AROUND_SURPRISED
    { 3, &gPlayerAnim_002828 },                         // PLAYER_CSMODE_INSPECT_GROUND_CAREFULLY
    { 6, &gPlayerAnim_002470 },                         // PLAYER_CSMODE_STARTLED_BY_GORONS_FALLING
    { 6, &gPlayerAnim_0032A8 },                         // PLAYER_CSMODE_FALL_TO_KNEE
    { 14, &gPlayerAnim_0032A0 },                        // PLAYER_CSMODE_FLAT_ON_BACK
    { 3, &gPlayerAnim_0032A0 },                         // PLAYER_CSMODE_RAISE_FROM_FLAT_ON_BACK
    { 5, &gPlayerAnim_002AE8 },                         // PLAYER_CSMODE_START_SPIN_ATTACK
    { 16, &gPlayerAnim_002450 },                        // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_IDLE
    { 15, &gPlayerAnim_002460 },                        // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA
    { 15, &gPlayerAnim_002458 },                        // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA
    { 3, &gPlayerAnim_002440 },                         // PLAYER_CSMODE_START_LOOK_AROUND_AFTER_SWORD_WARP
    { 3, &gPlayerAnim_002438 },                         // PLAYER_CSMODE_END_LOOK_AROUND_AFTER_SWORD_WARP
    { 3, &gPlayerAnim_002C88 },                         // PLAYER_CSMODE_LOOK_AROUND_AND_AT_SELF_QUICKLY
    { 6, &gPlayerAnim_003450 },                         // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_ADULT
    { 6, &gPlayerAnim_003448 },                         // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_ADULT
    { 6, &gPlayerAnim_003460 },                         // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_CHILD
    { 6, &gPlayerAnim_003440 },                         // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_CHILD
    { 3, &gPlayerAnim_002798 },                         // PLAYER_CSMODE_RESIST_DARK_MAGIC
    { 3, &gPlayerAnim_002818 },                         // PLAYER_CSMODE_TRIFORCE_HAND_RESONATES
    { 4, &gPlayerAnim_002848 },                         // PLAYER_CSMODE_STARE_DOWN_STARTLED
    { 3, &gPlayerAnim_002850 },                         // PLAYER_CSMODE_LOOK_UP_STARTLED
    { 3, &gPlayerAnim_0034E0 },                         // PLAYER_CSMODE_LOOK_TO_CHARACTER_AT_SIDE_SMILING
    { 3, &gPlayerAnim_0034D8 },                         // PLAYER_CSMODE_LOOK_TO_CHARACTER_ABOVE_SMILING
    { 6, &gPlayerAnim_0034C8 },                         // PLAYER_CSMODE_SURPRISED_DEFENSE
    { 3, &gPlayerAnim_003470 },                         // PLAYER_CSMODE_START_HALF_TURN_SURPRISED
    { 3, &gPlayerAnim_003478 },                         // PLAYER_CSMODE_END_HALF_TURN_SURPRISED
    { 3, &gPlayerAnim_0034C0 },                         // PLAYER_CSMODE_START_LOOK_UP_DEFENSE
    { 3, &gPlayerAnim_003480 },                         // PLAYER_CSMODE_LOOK_UP_DEFENSE_IDLE
    { 3, &gPlayerAnim_003490 },                         // PLAYER_CSMODE_END_LOOK_UP_DEFENSE
    { 3, &gPlayerAnim_003488 },                         // PLAYER_CSMODE_START_SWORD_KNOCKED_FROM_HAND
    { 3, &gPlayerAnim_003498 },                         // PLAYER_CSMODE_SWORD_KNOCKED_FROM_HAND_IDLE
    { 3, &gPlayerAnim_0034B0 },                         // PLAYER_CSMODE_END_SWORD_KNOCKED_FROM_HAND
    { -1, Player_CutsceneSetupSpinAttackIdle },         // PLAYER_CSMODE_SPIN_ATTACK_IDLE
    { 3, &gPlayerAnim_003420 },                         // PLAYER_CSMODE_INSPECT_WEAPON
    { -1, Player_SetupDoNothing4 },                     // PLAYER_CSMODE_UNK_91
    { -1, Player_CutsceneSetupKnockedToGroundDamaged }, // PLAYER_CSMODE_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT
    { 3, &gPlayerAnim_003250 },                         // PLAYER_CSMODE_REACT_TO_HEAT
    { -1, Player_CutsceneSetupGetSwordBack },           // PLAYER_CSMODE_GET_SWORD_BACK
    { 3, &gPlayerAnim_002810 },                         // PLAYER_CSMODE_CAUGHT_BY_GUARD
    { 3, &gPlayerAnim_002838 },                         // PLAYER_CSMODE_GET_SWORD_BACK_2
    { 3, &gPlayerAnim_002CD0 },                         // PLAYER_CSMODE_START_GANON_KILL_COMBO
    { 3, &gPlayerAnim_002CD8 },                         // PLAYER_CSMODE_END_GANON_KILL_COMBO
    { 3, &gPlayerAnim_002868 },                         // PLAYER_CSMODE_WATCH_ZELDA_STUN_GANON
    { 3, &gPlayerAnim_0027E8 },                         // PLAYER_CSMODE_START_LOOK_AT_SWORD_GLOW
    { 3, &gPlayerAnim_0027F8 },                         // PLAYER_CSMODE_LOOK_AT_SWORD_GLOW_IDLE
    { 3, &gPlayerAnim_002800 },                         // PLAYER_CSMODE_END_LOOK_AT_SWORD_GLOW
};

static CutsceneModeEntry sCutsceneModeUpdateFuncs[] = {
    { 0, NULL },                                         // PLAYER_CSMODE_NONE
    { -1, Player_CutsceneIdle },                         // PLAYER_CSMODE_IDLE
    { -1, Player_CutsceneTurnAroundSurprisedShort },     // PLAYER_CSMODE_TURN_AROUND_SURPRISED_SHORT
    { -1, func_80851998 },                               // PLAYER_CSMODE_UNK_3
    { -1, func_808519C0 },                               // PLAYER_CSMODE_UNK_4
    { 11, NULL },                                        // PLAYER_CSMODE_SURPRISED
    { -1, func_80852C50 },                               // PLAYER_CSMODE_UNK_6
    { -1, Player_CutsceneEnd },                          // PLAYER_CSMODE_END
    { -1, Player_CutsceneWait },                         // PLAYER_CSMODE_WAIT
    { -1, Player_CutsceneTurnAroundSurprisedLong },      // PLAYER_CSMODE_TURN_AROUND_SURPRISED_LONG
    { -1, Player_CutsceneEnterWarp },                    // PLAYER_CSMODE_ENTER_WARP
    { -1, Player_CutsceneRaisedByWarp },                 // PLAYER_CSMODE_RAISED_BY_WARP
    { -1, Player_CutsceneFightStance },                  // PLAYER_CSMODE_FIGHT_STANCE
    { 11, NULL },                                        // PLAYER_CSMODE_START_GET_SPIRITUAL_STONE
    { 11, NULL },                                        // PLAYER_CSMODE_GET_SPIRITUAL_STONE
    { 11, NULL },                                        // PLAYER_CSMODE_END_GET_SPIRITUAL_STONE
    { 18, sGetUpFromDekuTreeStoryAnimSfx },              // PLAYER_CSMODE_GET_UP_FROM_DEKU_TREE_STORY
    { 11, NULL },                                        // PLAYER_CSMODE_SIT_LISTENING_TO_DEKU_TREE_STORY
    { -1, Player_CutsceneSwordIntoPedestal },            // PLAYER_CSMODE_SWORD_INTO_PEDESTAL
    { 12, &gPlayerAnim_002730 },                         // PLAYER_CSMODE_REACT_TO_QUAKE
    { 11, NULL },                                        // PLAYER_CSMODE_END_REACT_TO_QUAKE
    { 0, NULL },                                         // PLAYER_CSMODE_UNK_21
    { -1, Player_CutsceneWarpToSages },                  // PLAYER_CSMODE_WARP_TO_SAGES
    { 11, NULL },                                        // PLAYER_CSMODE_LOOK_AT_SELF
    { -1, Player_CutsceneKnockedToGround },              // PLAYER_CSMODE_KNOCKED_TO_GROUND
    { 11, NULL },                                        // PLAYER_CSMODE_GET_UP_FROM_GROUND
    { 17, &gPlayerAnim_0030A8 },                         // PLAYER_CSMODE_START_PLAY_OCARINA
    { 11, NULL },                                        // PLAYER_CSMODE_END_PLAY_OCARINA
    { 11, NULL },                                        // PLAYER_CSMODE_GET_ITEM
    { 11, NULL },                                        // PLAYER_CSMODE_IDLE_2
    { -1, Player_CutsceneDrawAndBrandishSword },         // PLAYER_CSMODE_DRAW_AND_BRANDISH_SWORD
    { -1, Player_CutsceneCloseEyes },                    // PLAYER_CSMODE_CLOSE_EYES
    { -1, Player_CutsceneOpenEyes },                     // PLAYER_CSMODE_OPEN_EYES
    { 18, sSurprisedStumbleBackFallAnimSfx },                                  // PLAYER_CSMODE_SURPRIED_STUMBLE_BACK_AND_FALL
    { -1, Player_CutsceneSurfaceFromDive },              // PLAYER_CSMODE_SURFACE_FROM_DIVE
    { 11, NULL },                                        // PLAYER_CSMODE_GET_ITEM_IN_WATER
    { 11, NULL },                                        // PLAYER_CSMODE_GENTLE_KNOCKBACK_INTO_SIT
    { 11, NULL },                                        // PLAYER_CSMODE_GRABBED_AND_CARRIED_BY_NECK
    { 11, NULL },                                        // PLAYER_CSMODE_SLEEPING_RESTLESS
    { -1, Player_CutsceneSleeping },                     // PLAYER_CSMODE_SLEEPING
    { -1, Player_CutsceneAwaken },                       // PLAYER_CSMODE_AWAKEN
    { -1, Player_CutsceneGetOffBed },                    // PLAYER_CSMODE_GET_OFF_BED
    { -1, Player_CutsceneBlownBackward },                // PLAYER_CSMODE_BLOWN_BACKWARD
    { 13, &gPlayerAnim_002398 },                         // PLAYER_CSMODE_STAND_UP_AND_WATCH
    { -1, Player_CutsceneIdle3 },                        // PLAYER_CSMODE_IDLE_3
    { 0, NULL },                                         // PLAYER_CSMODE_STOP
    { 0, NULL },                                         // PLAYER_CSMODE_STOP_2
    { 11, NULL },                                        // PLAYER_CSMODE_LOOK_THROUGH_PEEPHOLE
    { -1, Player_CutsceneStepBackCautiously },           // PLAYER_CSMODE_STEP_BACK_CAUTIOUSLY
    { -1, Player_CutsceneWait },                         // PLAYER_CSMODE_IDLE_4
    { -1, Player_CutsceneDrawSwordChild },               // PLAYER_CSMODE_DRAW_SWORD_CHILD
    { 13, &gPlayerAnim_0027D0 },                         // PLAYER_CSMODE_JUMP_TO_ZELDAS_CRYSTAL
    { -1, Player_CutsceneDesperateLookAtZeldasCrystal }, // PLAYER_CSMODE_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL
    { 13, &gPlayerAnim_0027C8 },                         // PLAYER_CSMODE_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING
    { -1, Player_CutsceneTurnAroundSlowly },             // PLAYER_CSMODE_TURN_AROUND_SLOWLY
    { 11, NULL },                                        // PLAYER_CSMODE_END_SHIELD_EYES_WITH_HAND
    { 11, NULL },                                        // PLAYER_CSMODE_SHIELD_EYES_WITH_HAND
    { 12, &gPlayerAnim_002388 },                         // PLAYER_CSMODE_LOOK_AROUND_SURPRISED
    { -1, Player_CutsceneInspectGroundCarefully },       // PLAYER_CSMODE_INSPECT_GROUND_CAREFULLY
    { 11, NULL },                                        // PLAYER_CSMODE_STARTLED_BY_GORONS_FALLING
    { 18, sFallToKneeAnimSfx },                                  // PLAYER_CSMODE_FALL_TO_KNEE
    { 11, NULL },                                        // PLAYER_CSMODE_FLAT_ON_BACK
    { 11, NULL },                                        // PLAYER_CSMODE_RAISE_FROM_FLAT_ON_BACK
    { 11, NULL },                                        // PLAYER_CSMODE_START_SPIN_ATTACK
    { 11, NULL },                                        // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_IDLE
    { -1, Player_CutsceneStartPassOcarina },             // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA
    { 17, &gPlayerAnim_002450 },                         // PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA
    { 12, &gPlayerAnim_002448 },                         // PLAYER_CSMODE_START_LOOK_AROUND_AFTER_SWORD_WARP
    { 12, &gPlayerAnim_002450 },                         // PLAYER_CSMODE_END_LOOK_AROUND_AFTER_SWORD_WARP
    { 11, NULL },                                        // PLAYER_CSMODE_LOOK_AROUND_AND_AT_SELF_QUICKLY
    { -1, Player_LearnOcarinaSong },                     // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_ADULT
    { 17, &gPlayerAnim_003468 },                         // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_ADULT
    { -1, Player_LearnOcarinaSong },                     // PLAYER_CSMODE_START_LEARN_OCARINA_SONG_CHILD
    { 17, &gPlayerAnim_003468 },                         // PLAYER_CSMODE_END_LEARN_OCARINA_SONG_CHILD
    { 12, &gPlayerAnim_0027A0 },                         // PLAYER_CSMODE_RESIST_DARK_MAGIC
    { 12, &gPlayerAnim_002820 },                         // PLAYER_CSMODE_TRIFORCE_HAND_RESONATES
    { 11, NULL },                                        // PLAYER_CSMODE_STARE_DOWN_STARTLED
    { 12, &gPlayerAnim_002858 },                         // PLAYER_CSMODE_LOOK_UP_STARTLED
    { 12, &gPlayerAnim_0034D0 },                         // PLAYER_CSMODE_LOOK_TO_CHARACTER_AT_SIDE_SMILING
    { 13, &gPlayerAnim_0034F0 },                         // PLAYER_CSMODE_LOOK_TO_CHARACTER_ABOVE_SMILING
    { 12, &gPlayerAnim_0034E8 },                         // PLAYER_CSMODE_SURPRISED_DEFENSE
    { 12, &gPlayerAnim_0034A8 },                         // PLAYER_CSMODE_START_HALF_TURN_SURPRISED
    { 11, NULL },                                        // PLAYER_CSMODE_END_HALF_TURN_SURPRISED
    { 11, NULL },                                        // PLAYER_CSMODE_START_LOOK_UP_DEFENSE
    { 11, NULL },                                        // PLAYER_CSMODE_LOOK_UP_DEFENSE_IDLE
    { 11, NULL },                                        // PLAYER_CSMODE_END_LOOK_UP_DEFENSE
    { -1, Player_CutsceneSwordKnockedFromHand },         // PLAYER_CSMODE_START_SWORD_KNOCKED_FROM_HAND
    { 11, NULL },                                        // PLAYER_CSMODE_SWORD_KNOCKED_FROM_HAND_IDLE
    { 12, &gPlayerAnim_0034A0 },                         // PLAYER_CSMODE_END_SWORD_KNOCKED_FROM_HAND
    { -1, Player_CutsceneSpinAttackIdle },               // PLAYER_CSMODE_SPIN_ATTACK_IDLE
    { -1, Player_CutsceneInspectWeapon },                // PLAYER_CSMODE_INSPECT_WEAPON
    { -1, Player_DoNothing5 },                           // PLAYER_CSMODE_UNK_91
    { -1, Player_CutsceneKnockedToGroundDamaged },       // PLAYER_CSMODE_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT
    { 11, NULL },                                        // PLAYER_CSMODE_REACT_TO_HEAT
    { 11, NULL },                                        // PLAYER_CSMODE_GET_SWORD_BACK
    { 11, NULL },                                        // PLAYER_CSMODE_CAUGHT_BY_GUARD
    { -1, Player_CutsceneGetSwordBack },                 // PLAYER_CSMODE_GET_SWORD_BACK_2
    { -1, Player_CutsceneGanonKillCombo },               // PLAYER_CSMODE_START_GANON_KILL_COMBO
    { -1, Player_CutsceneGanonKillCombo },               // PLAYER_CSMODE_END_GANON_KILL_COMBO
    { 12, &gPlayerAnim_002870 },                         // PLAYER_CSMODE_WATCH_ZELDA_STUN_GANON
    { 12, &gPlayerAnim_0027F0 },                         // PLAYER_CSMODE_START_LOOK_AT_SWORD_GLOW
    { 12, &gPlayerAnim_002808 },                         // PLAYER_CSMODE_LOOK_AT_SWORD_GLOW_IDLE
    { 12, &gPlayerAnim_002450 },                         // PLAYER_CSMODE_END_LOOK_AT_SWORD_GLOW
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

void Player_CutsceneSetupSwimIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->stateFlags1 |= PLAYER_STATE1_SWIMMING;
    this->stateFlags2 |= PLAYER_STATE2_DIVING;
    this->stateFlags1 &= ~(PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALLING);

    Player_PlayAnimLoop(play, this, &gPlayerAnim_0032F0);
}

void Player_CutsceneSurfaceFromDive(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.gravity = 0.0f;

    if (this->unk_84F == 0) {
        if (func_8083D12C(play, this, NULL)) {
            this->unk_84F = 1;
        } else {
            Player_PlaySwimAnim(play, this, NULL, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->unk_6C2, -10000, 800);
            Player_UpdateSwimMovement(this, &this->actor.velocity.y, 4.0f, this->currentYaw);
        }
        return;
    }

    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F == 1) {
            Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_003328);
        } else {
            Player_PlayAnimLoop(play, this, &gPlayerAnim_003328);
        }
    }

    Player_SetVerticalWaterVelocity(this);
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_CutsceneIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
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

void Player_CutsceneTurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_CutsceneSetupIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimationHeader* anim;

    if (Player_IsSwimming(this)) {
        Player_CutsceneSetupSwimIdle(play, this, 0);
        return;
    }

    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_RELAX, this->modelAnimType);

    if ((this->csAction == 6) || (this->csAction == 0x2E)) {
        Player_PlayAnimOnce(play, this, anim);
    } else {
        Player_ClearRootLimbPosY(this);
        LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f), 0.0f, Animation_GetLastFrame(anim),
                             ANIMMODE_LOOP, -4.0f);
    }

    Player_StopMovement(this);
}

void Player_CutsceneWait(PlayState* play, Player* this, CsCmdActorAction* arg2) {
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

void Player_CutsceneTurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sTurnAroundSurprisedAnimSfx);
}

void Player_CutsceneSetupEnterWarp(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->stateFlags1 &= ~PLAYER_STATE1_AWAITING_THROWN_BOOMERANG;

    this->currentYaw = this->actor.shape.rot.y = this->actor.world.rot.y =
        Math_Vec3f_Yaw(&this->actor.world.pos, &this->csStartPos);

    if (this->linearVelocity <= 0.0f) {
        this->linearVelocity = 0.1f;
    } else if (this->linearVelocity > 2.5f) {
        this->linearVelocity = 2.5f;
    }
}

void Player_CutsceneEnterWarp(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    f32 sp1C = 2.5f;

    func_80845BA0(play, this, &sp1C, 10);

    if (play->sceneNum == SCENE_BDAN_BOSS) {
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

void Player_CutsceneSetupFightStance(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_SetupUnfriendlyZTarget(this, play);
}

void Player_CutsceneFightStance(PlayState* play, Player* this, CsCmdActorAction* arg2) {
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

void func_80851998(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_80845964(play, this, arg2, 0.0f, 0, 0);
}

void func_808519C0(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_80845964(play, this, arg2, 0.0f, 0, 1);
}

// unused
static LinkAnimationHeader* D_80855190[] = {
    &gPlayerAnim_002720,
    &gPlayerAnim_002360,
};

static Vec3f D_80855198 = { -1.0f, 70.0f, 20.0f };

void Player_CutsceneSetupSwordIntoPedestal(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Math_Vec3f_Copy(&this->actor.world.pos, &D_80855198);
    this->actor.shape.rot.y = -0x8000;
    Player_PlayAnimOnceSlowed(play, this, this->ageProperties->timeTravelStartAnim);
    Player_SetupAnimMovement(play, this,
                  PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_UPDATE_Y | PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                      PLAYER_ANIMMOVEFLAGS_7 | PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE);
}

static struct_808551A4 D_808551A4[] = {
    { NA_SE_IT_SWORD_PUTAWAY_STN, 0 },
    { NA_SE_IT_SWORD_STICK_STN, NA_SE_VO_LI_SWORD_N },
};

static PlayerAnimSfxEntry sSwordIntoPedestalAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x401D },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4027 },
};

void Player_CutsceneSwordIntoPedestal(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    struct_808551A4* sp2C;
    Gfx** dLists;

    LinkAnimation_Update(play, &this->skelAnime);

    if ((LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 70.0f)) ||
        (!LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 87.0f))) {
        sp2C = &D_808551A4[gSaveContext.linkAge];
        this->interactRangeActor->parent = &this->actor;

        if (!LINK_IS_ADULT) {
            dLists = gPlayerLeftHandBgsDLs;
        } else {
            dLists = gPlayerLeftHandClosedDLs;
        }
        this->leftHandDLists = dLists + gSaveContext.linkAge;

        func_8002F7DC(&this->actor, sp2C->unk_00);
        if (!LINK_IS_ADULT) {
            Player_PlayVoiceSfxForAge(this, sp2C->unk_02);
        }
    } else if (LINK_IS_ADULT) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 66.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
        }
    } else {
        Player_PlayAnimSfx(this, sSwordIntoPedestalAnimSfx);
    }
}

void Player_CutsceneSetupWarpToSages(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_002860, -(2.0f / 3.0f), 12.0f, 12.0f, ANIMMODE_ONCE,
                         0.0f);
}

static PlayerAnimSfxEntry sWarpToSagesAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x281E },
};

void Player_CutsceneWarpToSages(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);

    this->genericTimer++;

    if (this->genericTimer >= 180) {
        if (this->genericTimer == 180) {
            LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_003298, (2.0f / 3.0f), 10.0f,
                                 Animation_GetLastFrame(&gPlayerAnim_003298), ANIMMODE_ONCE, -8.0f);
        }
        Player_PlayAnimSfx(this, sWarpToSagesAnimSfx);
    }
}

void Player_CutsceneKnockedToGround(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->genericTimer == 0) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_PlayAnimOnce(play, this, &gPlayerAnim_002DB8);
        this->genericTimer = 1;
    }

    if (this->genericTimer != 0) {
        Player_StepLinearVelocityToZero(this);
    }
}

void Player_CutsceneSetupStartPlayOcarina(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_StopAnimAndMovementSlowed(play, this, &gPlayerAnim_0030A0);
    Player_SetOcarinaItemAP(this);
    Player_SetModels(this, Player_ActionToModelGroup(this, this->itemActionParam));
}

static PlayerAnimSfxEntry sDrawAndBrandishSwordAnimSfx[] = {
    { NA_SE_IT_SWORD_PICKOUT, -0x80C },
};

void Player_CutsceneDrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Player_CutsceneDrawSword(play, this, 0);
    } else {
        Player_PlayAnimSfx(this, sDrawAndBrandishSwordAnimSfx);
    }
}

void Player_CutsceneCloseEyes(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 0, 1);
}

void Player_CutsceneOpenEyes(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 2, 1);
}

void Player_CutsceneSetupGetItemInWater(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, &gPlayerAnim_003318,
                                          PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE |
                                              PLAYER_ANIMMOVEFLAGS_7);
}

void Player_CutsceneSetupSleeping(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlayAnimOnceWithMovement(play, this, &gPlayerAnim_002408,
                                    PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                        PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                        PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_GROAN);
}

void Player_CutsceneSleeping(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovement(play, this, &gPlayerAnim_002428,
                                        PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE |
                                            PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION |
                                            PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
    }
}

void func_80851F14(PlayState* play, Player* this, LinkAnimationHeader* anim, PlayerAnimSfxEntry* arg3) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, anim);
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        Player_PlayAnimSfx(this, arg3);
    }
}

void Player_CutsceneSetupSleepingRestless(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.shape.shadowDraw = NULL;
    Player_AnimPlaybackType7(play, this, &gPlayerAnim_002420);
}

static PlayerAnimSfxEntry sAwakenAnimSfx[] = {
    { NA_SE_VO_LI_RELAX, 0x2023 },
    { NA_SE_PL_SLIPDOWN, 0x8EC },
    { NA_SE_PL_SLIPDOWN, -0x900 },
};

void Player_CutsceneAwaken(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovement(play, this, &gPlayerAnim_002430, 0x9C);
        this->genericTimer = 1;
    } else if (this->genericTimer == 0) {
        Player_PlayAnimSfx(this, sAwakenAnimSfx);
        if (LinkAnimation_OnFrame(&this->skelAnime, 240.0f)) {
            this->actor.shape.shadowDraw = ActorShadow_DrawFeet;
        }
    }
}

static PlayerAnimSfxEntry sGetOffBedAnimSfx[] = {
    { NA_SE_PL_LAND_LADDER, 0x843 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x4854 },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x485A },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4860 },
};

void Player_CutsceneGetOffBed(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sGetOffBedAnimSfx);
}

void Player_CutsceneSetupBlownBackward(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, &gPlayerAnim_002340, 0x9D);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void func_808520BC(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    f32 startX = arg2->startPos.x;
    f32 startY = arg2->startPos.y;
    f32 startZ = arg2->startPos.z;
    f32 distX = (arg2->endPos.x - startX);
    f32 distY = (arg2->endPos.y - startY);
    f32 distZ = (arg2->endPos.z - startZ);
    f32 sp4 = (f32)(play->csCtx.frames - arg2->startFrame) / (f32)(arg2->endFrame - arg2->startFrame);

    this->actor.world.pos.x = distX * sp4 + startX;
    this->actor.world.pos.y = distY * sp4 + startY;
    this->actor.world.pos.z = distZ * sp4 + startZ;
}

static PlayerAnimSfxEntry sBlownBackwardAnimSfx[] = {
    { NA_SE_PL_BOUND, 0x1014 },
    { NA_SE_PL_BOUND, -0x101E },
};

void Player_CutsceneBlownBackward(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_808520BC(play, this, arg2);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sBlownBackwardAnimSfx);
}

void Player_CutsceneRaisedByWarp(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (arg2 != NULL) {
        func_808520BC(play, this, arg2);
    }
    LinkAnimation_Update(play, &this->skelAnime);
}

void func_808521F4(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_RELAX, this->modelAnimType));
    Player_StopMovement(this);
}

void Player_CutsceneIdle3(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void func_8085225C(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE | PLAYER_ANIMMOVEFLAGS_7);
}

void func_80852280(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.draw = Player_Draw;
}

void Player_CutsceneDrawSwordChild(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, &gPlayerAnim_002378);
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

void Player_CutsceneTurnAroundSlowly(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_80851F14(play, this, &gPlayerAnim_002770, sTurnAroundSlowlyAnimSfx);
}

static PlayerAnimSfxEntry sInspectGroundCarefullyAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x400F },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x4023 },
};

void Player_CutsceneInspectGroundCarefully(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_80851F14(play, this, &gPlayerAnim_002830, sInspectGroundCarefullyAnimSfx);
}

void Player_CutsceneStartPassOcarina(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_002468);
        this->genericTimer = 1;
    }

    if ((this->genericTimer != 0) && (play->csCtx.frames >= 900)) {
        this->rightHandType = PLAYER_MODELTYPE_LH_OPEN;
    } else {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
    }
}

void func_80852414(PlayState* play, Player* this, LinkAnimationHeader* anim, PlayerAnimSfxEntry* arg3) {
    Player_AnimPlaybackType12(play, this, anim);
    if (this->genericTimer == 0) {
        Player_PlayAnimSfx(this, arg3);
    }
}

static PlayerAnimSfxEntry sStepBackCautiouslyAnimSfx[] = {
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, 0x300F },
    { NA_SE_PL_WALK_GROUND - SFX_FLAG, -0x3021 },
};

void Player_CutsceneStepBackCautiously(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_80852414(play, this, &gPlayerAnim_002378, sStepBackCautiouslyAnimSfx);
}

static PlayerAnimSfxEntry sDesperateLookAtZeldasCrystalAnimSfx[] = {
    { NA_SE_PL_KNOCK, -0x84E },
};

void Player_CutsceneDesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    func_80852414(play, this, &gPlayerAnim_0027D0, sDesperateLookAtZeldasCrystalAnimSfx);
}

void Player_CutsceneSetupSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_SetupSpinAttackAnims(play, this);
}

void Player_CutsceneSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    sControlInput->press.button |= BTN_B;

    Player_ChargeSpinAttack(this, play);
}

void Player_CutsceneInspectWeapon(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_ChargeSpinAttack(this, play);
}

void Player_SetupDoNothing4(PlayState* play, Player* this, CsCmdActorAction* arg2) {
}

void Player_DoNothing5(PlayState* play, Player* this, CsCmdActorAction* arg2) {
}

void Player_CutsceneSetupKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->stateFlags3 |= PLAYER_STATE3_MIDAIR;
    this->linearVelocity = 2.0f;
    this->actor.velocity.y = -1.0f;

    Player_PlayAnimOnce(play, this, &gPlayerAnim_002DB0);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

static void (*D_808551FC[])(Player* this, PlayState* play) = {
    func_8084377C,
    func_80843954,
    func_80843A38,
};

void Player_CutsceneKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    D_808551FC[this->genericTimer](this, play);
}

void Player_CutsceneSetupGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_CutsceneDrawSword(play, this, 0);
    Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002838);
}

void Player_CutsceneSwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);

    if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
        this->heldItemActionParam = this->itemActionParam = PLAYER_AP_NONE;
        this->heldItemId = ITEM_NONE;
        this->modelGroup = this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_AP_NONE);
        this->leftHandDLists = gPlayerLeftHandOpenDLs;
        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        gSaveContext.equips.buttonItems[0] = ITEM_SWORD_MASTER;
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SWORD);
    }
}

static LinkAnimationHeader* sLearnOcarinaSongAnims[] = {
    &gPlayerAnim_0034B8,
    &gPlayerAnim_003458,
};

static Vec3s sBaseSparklePos[2][2] = {
    { { -200, 700, 100 }, { 800, 600, 800 } },
    { { -200, 500, 0 }, { 600, 400, 600 } },
};

void Player_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorAction* arg2) {
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

void Player_CutsceneGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_CutsceneEnd(play, this, arg2);
    } else if (this->genericTimer == 0) {
        Item_Give(play, ITEM_SWORD_MASTER);
        Player_CutsceneDrawSword(play, this, 0);
    } else {
        Player_PlaySwingSwordSfx(this);
    }
}

void Player_CutsceneGanonKillCombo(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupMeleeAttack(this, 0.0f, 99.0f, this->skelAnime.endFrame - 8.0f);
    }

    if (this->heldItemActionParam != PLAYER_AP_SWORD_MASTER) {
        Player_CutsceneDrawSword(play, this, 1);
    }
}

void Player_CutsceneEnd(PlayState* play, Player* this, CsCmdActorAction* arg2) {
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

void func_808529D0(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.world.pos.x = arg2->startPos.x;
    this->actor.world.pos.y = arg2->startPos.y;
    if ((play->sceneNum == SCENE_SPOT04) && !LINK_IS_ADULT) {
        this->actor.world.pos.y -= 1.0f;
    }
    this->actor.world.pos.z = arg2->startPos.z;
    this->currentYaw = this->actor.shape.rot.y = arg2->rot.y;
}

void func_80852A54(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    f32 dx = arg2->startPos.x - (s32)this->actor.world.pos.x;
    f32 dy = arg2->startPos.y - (s32)this->actor.world.pos.y;
    f32 dz = arg2->startPos.z - (s32)this->actor.world.pos.z;
    f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    s16 yawDiff = arg2->rot.y - this->actor.shape.rot.y;

    if ((this->linearVelocity == 0.0f) && ((dist > 50.0f) || (ABS(yawDiff) > 0x4000))) {
        func_808529D0(play, this, arg2);
    }

    this->skelAnime.moveFlags = 0;
    Player_ClearRootLimbPosY(this);
}

void func_80852B4C(PlayState* play, Player* this, CsCmdActorAction* arg2, CutsceneModeEntry* arg3) {
    if (arg3->playbackFuncID > 0) {
        csModePlaybackFuncs[arg3->playbackFuncID](play, this, arg3->ptr);
    } else if (arg3->playbackFuncID < 0) {
        arg3->func(play, this, arg2);
    }

    if ((D_80858AA0 & 4) && !(this->skelAnime.moveFlags & PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE)) {
        this->skelAnime.morphTable[0].y /= this->ageProperties->translationScale;
        D_80858AA0 = 0;
    }
}

void func_80852C0C(PlayState* play, Player* this, s32 csMode) {
    if ((csMode != PLAYER_CSMODE_IDLE) && (csMode != PLAYER_CSMODE_WAIT) && (csMode != PLAYER_CSMODE_IDLE_4) &&
        (csMode != PLAYER_CSMODE_END)) {
        Player_DetatchHeldActor(play, this);
    }
}

void func_80852C50(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    CsCmdActorAction* linkCsAction = play->csCtx.linkAction;
    s32 pad;
    s32 currentCsAction;

    if (play->csCtx.state == CS_STATE_UNSKIPPABLE_INIT) {
        Actor_SetPlayerCutscene(play, NULL, PLAYER_CSMODE_END);
        this->csAction = 0;
        Player_StopMovement(this);
        return;
    }

    if (linkCsAction == NULL) {
        this->actor.flags &= ~ACTOR_FLAG_6;
        return;
    }

    if (this->csAction != linkCsAction->action) {
        currentCsAction = sLinkCsActions[linkCsAction->action];
        if (currentCsAction >= 0) {
            if ((currentCsAction == 3) || (currentCsAction == 4)) {
                func_80852A54(play, this, linkCsAction);
            } else {
                func_808529D0(play, this, linkCsAction);
            }
        }

        D_80858AA0 = this->skelAnime.moveFlags;

        Player_EndAnimMovement(this);
        osSyncPrintf("TOOL MODE=%d\n", currentCsAction);
        func_80852C0C(play, this, ABS(currentCsAction));
        func_80852B4C(play, this, linkCsAction, &sCutsceneModeInitFuncs[ABS(currentCsAction)]);

        this->genericTimer = 0;
        this->unk_84F = 0;
        this->csAction = linkCsAction->action;
    }

    currentCsAction = sLinkCsActions[this->csAction];
    func_80852B4C(play, this, linkCsAction, &sCutsceneModeUpdateFuncs[ABS(currentCsAction)]);
}

void Player_StartCutscene(Player* this, PlayState* play) {
    if (this->csMode != this->prevCsMode) {
        D_80858AA0 = this->skelAnime.moveFlags;

        Player_EndAnimMovement(this);
        this->prevCsMode = this->csMode;
        osSyncPrintf("DEMO MODE=%d\n", this->csMode);
        func_80852C0C(play, this, this->csMode);
        func_80852B4C(play, this, NULL, &sCutsceneModeInitFuncs[this->csMode]);
    }

    func_80852B4C(play, this, NULL, &sCutsceneModeUpdateFuncs[this->csMode]);
}

s32 Player_IsDroppingFish(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (Player_DropItemFromBottle == this->actionFunc) && (this->itemActionParam == PLAYER_AP_BOTTLE_FISH);
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
        Player_PlayAnimOnce(play, this, &gPlayerAnim_003120);
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

s32 Player_SetupInflictDamage(PlayState* play, s32 damage) {
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
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_003328);
            } else if ((actor->category != ACTORCAT_NPC) || (this->heldItemActionParam == PLAYER_AP_FISHING_POLE)) {
                Player_SetupTalkWithActor(play, this);

                if (!Player_IsUnfriendlyZTargeting(this)) {
                    if ((actor != this->naviActor) && (actor->xzDistToPlayer < 40.0f)) {
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_002DF0);
                    } else {
                        Player_PlayAnimLoop(play, this, Player_GetStandingStillAnim(this));
                    }
                }
            } else {
                Player_SetupMiniCsFunc(play, this, Player_SetupTalkWithActor);
                Player_PlayAnimOnceSlowed(play, this, (actor->xzDistToPlayer < 40.0f) ? &gPlayerAnim_002DF0 : &gPlayerAnim_0031A0);
            }

            if (this->skelAnime.animation == &gPlayerAnim_002DF0) {
                Player_SetupAnimMovement(play, this, PLAYER_ANIMMOVEFLAGS_UPDATE_XZ | PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION | PLAYER_ANIMMOVEFLAGS_NO_MOVE);
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

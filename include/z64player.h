#ifndef Z64PLAYER_H
#define Z64PLAYER_H

#include "z64actor.h"
#include "alignment.h"

struct Player;

typedef enum {
    /* 0 */ PLAYER_SWORD_NONE,
    /* 1 */ PLAYER_SWORD_KOKIRI,
    /* 2 */ PLAYER_SWORD_MASTER,
    /* 3 */ PLAYER_SWORD_BGS,
    /* 4 */ PLAYER_SWORD_MAX
} PlayerSword;

typedef enum {
    /* 0x00 */ PLAYER_SHIELD_NONE,
    /* 0x01 */ PLAYER_SHIELD_DEKU,
    /* 0x02 */ PLAYER_SHIELD_HYLIAN,
    /* 0x03 */ PLAYER_SHIELD_MIRROR,
    /* 0x04 */ PLAYER_SHIELD_MAX
} PlayerShield;

typedef enum {
    /* 0x00 */ PLAYER_TUNIC_KOKIRI,
    /* 0x01 */ PLAYER_TUNIC_GORON,
    /* 0x02 */ PLAYER_TUNIC_ZORA,
    /* 0x03 */ PLAYER_TUNIC_MAX
} PlayerTunic;

typedef enum {
    /* 0x00 */ PLAYER_BOOTS_KOKIRI,
    /* 0x01 */ PLAYER_BOOTS_IRON,
    /* 0x02 */ PLAYER_BOOTS_HOVER,
    /* Values below are only relevant when setting regs in Player_SetBootData */
    /* 0x03 */ PLAYER_BOOTS_INDOOR,
    /* 0x04 */ PLAYER_BOOTS_IRON_UNDERWATER,
    /* 0x05 */ PLAYER_BOOTS_KOKIRI_CHILD,
    /* 0x06 */ PLAYER_BOOTS_MAX
} PlayerBoots;

typedef enum {
    /* 0x00 */ PLAYER_STR_NONE,
    /* 0x01 */ PLAYER_STR_BRACELET,
    /* 0x02 */ PLAYER_STR_SILVER_G,
    /* 0x03 */ PLAYER_STR_GOLD_G,
    /* 0x04 */ PLAYER_STR_MAX
} PlayerStrength;

typedef enum {
    /* 0x00 */ PLAYER_MASK_NONE,
    /* 0x01 */ PLAYER_MASK_KEATON,
    /* 0x02 */ PLAYER_MASK_SKULL,
    /* 0x03 */ PLAYER_MASK_SPOOKY,
    /* 0x04 */ PLAYER_MASK_BUNNY,
    /* 0x05 */ PLAYER_MASK_GORON,
    /* 0x06 */ PLAYER_MASK_ZORA,
    /* 0x07 */ PLAYER_MASK_GERUDO,
    /* 0x08 */ PLAYER_MASK_TRUTH,
    /* 0x09 */ PLAYER_MASK_MAX
} PlayerMask;

typedef enum {
    /* 0x00 */ PLAYER_AP_NONE,
    /* 0x01 */ PLAYER_AP_LAST_USED,
    /* 0x02 */ PLAYER_AP_FISHING_POLE,
    /* 0x03 */ PLAYER_AP_SWORD_MASTER,
    /* 0x04 */ PLAYER_AP_SWORD_KOKIRI,
    /* 0x05 */ PLAYER_AP_SWORD_BGS,
    /* 0x06 */ PLAYER_AP_STICK,
    /* 0x07 */ PLAYER_AP_HAMMER,
    /* 0x08 */ PLAYER_AP_BOW,
    /* 0x09 */ PLAYER_AP_BOW_FIRE,
    /* 0x0A */ PLAYER_AP_BOW_ICE,
    /* 0x0B */ PLAYER_AP_BOW_LIGHT,
    /* 0x0C */ PLAYER_AP_BOW_0C,
    /* 0x0D */ PLAYER_AP_BOW_0D,
    /* 0x0E */ PLAYER_AP_BOW_0E,
    /* 0x0F */ PLAYER_AP_SLINGSHOT,
    /* 0x10 */ PLAYER_AP_HOOKSHOT,
    /* 0x11 */ PLAYER_AP_LONGSHOT,
    /* 0x12 */ PLAYER_AP_BOMB,
    /* 0x13 */ PLAYER_AP_BOMBCHU,
    /* 0x14 */ PLAYER_AP_BOOMERANG,
    /* 0x15 */ PLAYER_AP_MAGIC_SPELL_15,
    /* 0x16 */ PLAYER_AP_MAGIC_SPELL_16,
    /* 0x17 */ PLAYER_AP_MAGIC_SPELL_17,
    /* 0x18 */ PLAYER_AP_FARORES_WIND,
    /* 0x19 */ PLAYER_AP_NAYRUS_LOVE,
    /* 0x1A */ PLAYER_AP_DINS_FIRE,
    /* 0x1B */ PLAYER_AP_NUT,
    /* 0x1C */ PLAYER_AP_OCARINA_FAIRY,
    /* 0x1D */ PLAYER_AP_OCARINA_TIME,
    /* 0x1E */ PLAYER_AP_BOTTLE,
    /* 0x1F */ PLAYER_AP_BOTTLE_FISH,
    /* 0x20 */ PLAYER_AP_BOTTLE_FIRE,
    /* 0x21 */ PLAYER_AP_BOTTLE_BUG,
    /* 0x22 */ PLAYER_AP_BOTTLE_POE,
    /* 0x23 */ PLAYER_AP_BOTTLE_BIG_POE,
    /* 0x24 */ PLAYER_AP_BOTTLE_LETTER,
    /* 0x25 */ PLAYER_AP_BOTTLE_POTION_RED,
    /* 0x26 */ PLAYER_AP_BOTTLE_POTION_BLUE,
    /* 0x27 */ PLAYER_AP_BOTTLE_POTION_GREEN,
    /* 0x28 */ PLAYER_AP_BOTTLE_MILK,
    /* 0x29 */ PLAYER_AP_BOTTLE_MILK_HALF,
    /* 0x2A */ PLAYER_AP_BOTTLE_FAIRY,
    /* 0x2B */ PLAYER_AP_LETTER_ZELDA,
    /* 0x2C */ PLAYER_AP_WEIRD_EGG,
    /* 0x2D */ PLAYER_AP_CHICKEN,
    /* 0x2E */ PLAYER_AP_BEAN,
    /* 0x2F */ PLAYER_AP_POCKET_EGG,
    /* 0x30 */ PLAYER_AP_POCKET_CUCCO,
    /* 0x31 */ PLAYER_AP_COJIRO,
    /* 0x32 */ PLAYER_AP_ODD_MUSHROOM,
    /* 0x33 */ PLAYER_AP_ODD_POTION,
    /* 0x34 */ PLAYER_AP_SAW,
    /* 0x35 */ PLAYER_AP_SWORD_BROKEN,
    /* 0x36 */ PLAYER_AP_PRESCRIPTION,
    /* 0x37 */ PLAYER_AP_FROG,
    /* 0x38 */ PLAYER_AP_EYEDROPS,
    /* 0x39 */ PLAYER_AP_CLAIM_CHECK,
    /* 0x3A */ PLAYER_AP_MASK_KEATON,
    /* 0x3B */ PLAYER_AP_MASK_SKULL,
    /* 0x3C */ PLAYER_AP_MASK_SPOOKY,
    /* 0x3D */ PLAYER_AP_MASK_BUNNY,
    /* 0x3E */ PLAYER_AP_MASK_GORON,
    /* 0x3F */ PLAYER_AP_MASK_ZORA,
    /* 0x40 */ PLAYER_AP_MASK_GERUDO,
    /* 0x41 */ PLAYER_AP_MASK_TRUTH,
    /* 0x42 */ PLAYER_AP_LENS,
    /* 0x43 */ PLAYER_AP_MAX
} PlayerActionParam;

typedef enum {
    /* 0x00 */ PLAYER_LIMB_NONE,
    /* 0x01 */ PLAYER_LIMB_ROOT,
    /* 0x02 */ PLAYER_LIMB_WAIST,
    /* 0x03 */ PLAYER_LIMB_LOWER,
    /* 0x04 */ PLAYER_LIMB_R_THIGH,
    /* 0x05 */ PLAYER_LIMB_R_SHIN,
    /* 0x06 */ PLAYER_LIMB_R_FOOT,
    /* 0x07 */ PLAYER_LIMB_L_THIGH,
    /* 0x08 */ PLAYER_LIMB_L_SHIN,
    /* 0x09 */ PLAYER_LIMB_L_FOOT,
    /* 0x0A */ PLAYER_LIMB_UPPER,
    /* 0x0B */ PLAYER_LIMB_HEAD,
    /* 0x0C */ PLAYER_LIMB_HAT,
    /* 0x0D */ PLAYER_LIMB_COLLAR,
    /* 0x0E */ PLAYER_LIMB_L_SHOULDER,
    /* 0x0F */ PLAYER_LIMB_L_FOREARM,
    /* 0x10 */ PLAYER_LIMB_L_HAND,
    /* 0x11 */ PLAYER_LIMB_R_SHOULDER,
    /* 0x12 */ PLAYER_LIMB_R_FOREARM,
    /* 0x13 */ PLAYER_LIMB_R_HAND,
    /* 0x14 */ PLAYER_LIMB_SHEATH,
    /* 0x15 */ PLAYER_LIMB_TORSO,
    /* 0x16 */ PLAYER_LIMB_MAX
} PlayerLimb;

typedef enum {
    /* 0x00 */ PLAYER_BODYPART_WAIST,      // PLAYER_LIMB_WAIST
    /* 0x01 */ PLAYER_BODYPART_R_THIGH,    // PLAYER_LIMB_R_THIGH
    /* 0x02 */ PLAYER_BODYPART_R_SHIN,     // PLAYER_LIMB_R_SHIN
    /* 0x03 */ PLAYER_BODYPART_R_FOOT,     // PLAYER_LIMB_R_FOOT
    /* 0x04 */ PLAYER_BODYPART_L_THIGH,    // PLAYER_LIMB_L_THIGH
    /* 0x05 */ PLAYER_BODYPART_L_SHIN,     // PLAYER_LIMB_L_SHIN
    /* 0x06 */ PLAYER_BODYPART_L_FOOT,     // PLAYER_LIMB_L_FOOT
    /* 0x07 */ PLAYER_BODYPART_HEAD,       // PLAYER_LIMB_HEAD
    /* 0x08 */ PLAYER_BODYPART_HAT,        // PLAYER_LIMB_HAT
    /* 0x09 */ PLAYER_BODYPART_COLLAR,     // PLAYER_LIMB_COLLAR
    /* 0x0A */ PLAYER_BODYPART_L_SHOULDER, // PLAYER_LIMB_L_SHOULDER
    /* 0x0B */ PLAYER_BODYPART_L_FOREARM,  // PLAYER_LIMB_L_FOREARM
    /* 0x0C */ PLAYER_BODYPART_L_HAND,     // PLAYER_LIMB_L_HAND
    /* 0x0D */ PLAYER_BODYPART_R_SHOULDER, // PLAYER_LIMB_R_SHOULDER
    /* 0x0E */ PLAYER_BODYPART_R_FOREARM,  // PLAYER_LIMB_R_FOREARM
    /* 0x0F */ PLAYER_BODYPART_R_HAND,     // PLAYER_LIMB_R_HAND
    /* 0x10 */ PLAYER_BODYPART_SHEATH,     // PLAYER_LIMB_SHEATH
    /* 0x11 */ PLAYER_BODYPART_TORSO,      // PLAYER_LIMB_TORSO
    /* 0x12 */ PLAYER_BODYPART_MAX
} PlayerBodyPart;

typedef enum {
    /*  0 */ PLAYER_MWA_FORWARD_SLASH_1H,
    /*  1 */ PLAYER_MWA_FORWARD_SLASH_2H,
    /*  2 */ PLAYER_MWA_FORWARD_COMBO_1H,
    /*  3 */ PLAYER_MWA_FORWARD_COMBO_2H,
    /*  4 */ PLAYER_MWA_LEFT_SLASH_1H,
    /*  5 */ PLAYER_MWA_LEFT_SLASH_2H,
    /*  6 */ PLAYER_MWA_LEFT_COMBO_1H,
    /*  7 */ PLAYER_MWA_LEFT_COMBO_2H,
    /*  8 */ PLAYER_MWA_RIGHT_SLASH_1H,
    /*  9 */ PLAYER_MWA_RIGHT_SLASH_2H,
    /* 10 */ PLAYER_MWA_RIGHT_COMBO_1H,
    /* 11 */ PLAYER_MWA_RIGHT_COMBO_2H,
    /* 12 */ PLAYER_MWA_STAB_1H,
    /* 13 */ PLAYER_MWA_STAB_2H,
    /* 14 */ PLAYER_MWA_STAB_COMBO_1H,
    /* 15 */ PLAYER_MWA_STAB_COMBO_2H,
    /* 16 */ PLAYER_MWA_FLIPSLASH_START,
    /* 17 */ PLAYER_MWA_JUMPSLASH_START,
    /* 18 */ PLAYER_MWA_FLIPSLASH_FINISH,
    /* 19 */ PLAYER_MWA_JUMPSLASH_FINISH,
    /* 20 */ PLAYER_MWA_BACKSLASH_RIGHT,
    /* 21 */ PLAYER_MWA_BACKSLASH_LEFT,
    /* 22 */ PLAYER_MWA_HAMMER_FORWARD,
    /* 23 */ PLAYER_MWA_HAMMER_SIDE,
    /* 24 */ PLAYER_MWA_SPIN_ATTACK_1H,
    /* 25 */ PLAYER_MWA_SPIN_ATTACK_2H,
    /* 26 */ PLAYER_MWA_BIG_SPIN_1H,
    /* 27 */ PLAYER_MWA_BIG_SPIN_2H,
    /* 28 */ PLAYER_MWA_MAX
} PlayerMeleeWeaponAnimation;

typedef enum {
    /* -1 */ PLAYER_DOORTYPE_AJAR = -1,
    /*  0 */ PLAYER_DOORTYPE_NONE,
    /*  1 */ PLAYER_DOORTYPE_HANDLE,
    /*  2 */ PLAYER_DOORTYPE_SLIDING,
    /*  3 */ PLAYER_DOORTYPE_FAKE
} PlayerDoorType;

typedef enum {
    /* 0x00 */ PLAYER_MODELGROUP_SHIELD, // unused (except with the `func_80091880` bug)
    /* 0x01 */ PLAYER_MODELGROUP_CHILD_HYLIAN_SHIELD, // kokiri/master sword, shield not in hand
    /* 0x02 */ PLAYER_MODELGROUP_SWORD, // kokiri/master sword and possibly shield
    /* 0x03 */ PLAYER_MODELGROUP_DEFAULT, // non-specific models, for items that don't have particular link models
    /* 0x04 */ PLAYER_MODELGROUP_4, // unused, same as PLAYER_MODELGROUP_DEFAULT
    /* 0x05 */ PLAYER_MODELGROUP_BGS, // biggoron sword
    /* 0x06 */ PLAYER_MODELGROUP_BOW_SLINGSHOT, // bow/slingshot
    /* 0x07 */ PLAYER_MODELGROUP_EXPLOSIVES, // bombs, bombchus, same as PLAYER_MODELGROUP_DEFAULT
    /* 0x08 */ PLAYER_MODELGROUP_BOOMERANG,
    /* 0x09 */ PLAYER_MODELGROUP_HOOKSHOT,
    /* 0x0A */ PLAYER_MODELGROUP_10, // stick/fishing pole (which are drawn separately)
    /* 0x0B */ PLAYER_MODELGROUP_HAMMER,
    /* 0x0C */ PLAYER_MODELGROUP_OCARINA, // ocarina
    /* 0x0D */ PLAYER_MODELGROUP_OOT, // ocarina of time
    /* 0x0E */ PLAYER_MODELGROUP_BOTTLE, // bottles (drawn separately)
    /* 0x0F */ PLAYER_MODELGROUP_15, // "last used"
    /* 0x10 */ PLAYER_MODELGROUP_MAX
} PlayerModelGroup;

typedef enum {
    /* 0x00 */ PLAYER_MODELGROUPENTRY_ANIM,
    /* 0x01 */ PLAYER_MODELGROUPENTRY_LEFT_HAND,
    /* 0x02 */ PLAYER_MODELGROUPENTRY_RIGHT_HAND,
    /* 0x03 */ PLAYER_MODELGROUPENTRY_SHEATH,
    /* 0x04 */ PLAYER_MODELGROUPENTRY_WAIST,
    /* 0x05 */ PLAYER_MODELGROUPENTRY_MAX
} PlayerModelGroupEntry;

typedef enum {
    // left hand
    /* 0x00 */ PLAYER_MODELTYPE_LH_OPEN, // empty open hand
    /* 0x01 */ PLAYER_MODELTYPE_LH_CLOSED, // empty closed hand
    /* 0x02 */ PLAYER_MODELTYPE_LH_SWORD, // holding kokiri/master sword
    /* 0x03 */ PLAYER_MODELTYPE_LH_SWORD_2, // unused, same as PLAYER_MODELTYPE_LH_SWORD
    /* 0x04 */ PLAYER_MODELTYPE_LH_BGS, // holding bgs/broken giant knife (child: master sword)
    /* 0x05 */ PLAYER_MODELTYPE_LH_HAMMER, // holding hammer (child: empty hand)
    /* 0x06 */ PLAYER_MODELTYPE_LH_BOOMERANG, // holding boomerang (adult: empty hand)
    /* 0x07 */ PLAYER_MODELTYPE_LH_BOTTLE, // holding bottle (bottle drawn separately)
    // right hand
    /* 0x08 */ PLAYER_MODELTYPE_RH_OPEN, // empty open hand
    /* 0x09 */ PLAYER_MODELTYPE_RH_CLOSED, // empty closed hand
    /* 0x0A */ PLAYER_MODELTYPE_RH_SHIELD, // holding a shield (including no shield)
    /* 0x0B */ PLAYER_MODELTYPE_RH_BOW_SLINGSHOT, // holding bow/slingshot
    /* 0x0C */ PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2, // unused, same as PLAYER_MODELTYPE_RH_BOW_SLINGSHOT
    /* 0x0D */ PLAYER_MODELTYPE_RH_OCARINA, // holding ocarina (child: fairy ocarina, adult: OoT)
    /* 0x0E */ PLAYER_MODELTYPE_RH_OOT, // holding OoT
    /* 0x0F */ PLAYER_MODELTYPE_RH_HOOKSHOT, // holding hookshot (child: empty hand)
    // sheath
    /* 0x10 */ PLAYER_MODELTYPE_SHEATH_16, // sheathed kokiri/master sword?
    /* 0x11 */ PLAYER_MODELTYPE_SHEATH_17, // empty sheath?
    /* 0x12 */ PLAYER_MODELTYPE_SHEATH_18, // sword sheathed and shield on back?
    /* 0x13 */ PLAYER_MODELTYPE_SHEATH_19, // empty sheath and shield on back?
    // waist
    /* 0x14 */ PLAYER_MODELTYPE_WAIST,
    /* 0x15 */ PLAYER_MODELTYPE_MAX,
    /* 0xFF */ PLAYER_MODELTYPE_RH_FF = 0xFF // disable shield collider, cutscene-specific
} PlayerModelType;

typedef enum {
    /* 0x00 */ PLAYER_ANIMTYPE_DEFAULT,
    /* 0x01 */ PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON,
    /* 0x02 */ PLAYER_ANIMTYPE_HOLDING_SHIELD,
    /* 0x03 */ PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON,
    /* 0x04 */ PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND,
    /* 0x05 */ PLAYER_ANIMTYPE_USED_EXPLOSIVE,
    /* 0x06 */ PLAYER_ANIMTYPE_MAX
} PlayerAnimType;

typedef enum {
    /* 0x00 */ PLAYER_ANIMGROUP_STANDING_STILL,
    /* 0x01 */ PLAYER_ANIMGROUP_WALKING,
    /* 0x02 */ PLAYER_ANIMGROUP_RUNNING,
    /* 0x03 */ PLAYER_ANIMGROUP_RUNNING_DAMAGED,
    /* 0x04 */ PLAYER_ANIMGROUP_IRON_BOOTS,
    /* 0x05 */ PLAYER_ANIMGROUP_FIGHTING_LEFT_OF_ENEMY,
    /* 0x06 */ PLAYER_ANIMGROUP_FIGHTING_RIGHT_OF_ENEMY,
    /* 0x07 */ PLAYER_ANIMGROUP_START_FIGHTING,
    /* 0x08 */ PLAYER_ANIMGROUP_8,
    /* 0x09 */ PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_LEFT,
    /* 0x0A */ PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_LEFT,
    /* 0x0B */ PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_RIGHT,
    /* 0x0C */ PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_RIGHT,
    /* 0x0D */ PLAYER_ANIMGROUP_HOLDING_OBJECT,
    /* 0x0E */ PLAYER_ANIMGROUP_TALL_JUMP_LANDING,
    /* 0x0F */ PLAYER_ANIMGROUP_SHORT_JUMP_LANDING,
    /* 0x10 */ PLAYER_ANIMGROUP_ROLLING,
    /* 0x11 */ PLAYER_ANIMGROUP_17,
    /* 0x12 */ PLAYER_ANIMGROUP_END_WALK_ON_LEFT_FOOT,
    /* 0x13 */ PLAYER_ANIMGROUP_END_WALK_ON_RIGHT_FOOT,
    /* 0x14 */ PLAYER_ANIMGROUP_START_DEFENDING,
    /* 0x15 */ PLAYER_ANIMGROUP_DEFENDING,
    /* 0x16 */ PLAYER_ANIMGROUP_END_DEFENDING,
    /* 0x17 */ PLAYER_ANIMGROUP_END_SIDEWALKING,
    /* 0x18 */ PLAYER_ANIMGROUP_SIDEWALKING_LEFT,
    /* 0x19 */ PLAYER_ANIMGROUP_SIDEWALKING_RIGHT,
    /* 0x1A */ PLAYER_ANIMGROUP_SHUFFLE_TURN,
    /* 0x1B */ PLAYER_ANIMGROUP_END_FIGHTING_LEFT_OF_ENEMY,
    /* 0x1C */ PLAYER_ANIMGROUP_END_FIGHTING_RIGHT_OF_ENEMY,
    /* 0x1D */ PLAYER_ANIMGROUP_THROWING_OBJECT,
    /* 0x1E */ PLAYER_ANIMGROUP_PUTTING_DOWN_OBJECT,
    /* 0x1F */ PLAYER_ANIMGROUP_BACKWALKING,
    /* 0x20 */ PLAYER_ANIMGROUP_BEGIN_CHECKING_OR_SPEAKING,
    /* 0x21 */ PLAYER_ANIMGROUP_CHECKING_OR_SPEAKING,
    /* 0x22 */ PLAYER_ANIMGROUP_END_CHECKING_OR_SPEAKING,
    /* 0x23 */ PLAYER_ANIMGROUP_END_PULL_OBJECT,
    /* 0x24 */ PLAYER_ANIMGROUP_PULL_OBJECT,
    /* 0x25 */ PLAYER_ANIMGROUP_PUSH_OBJECT,
    /* 0x26 */ PLAYER_ANIMGROUP_KNOCKED_FROM_CLIMBING,
    /* 0x27 */ PLAYER_ANIMGROUP_HANGING_FROM_LEDGE,
    /* 0x28 */ PLAYER_ANIMGROUP_CLIMBING_IDLE,
    /* 0x29 */ PLAYER_ANIMGROUP_CLIMBING,
    /* 0x2A */ PLAYER_ANIMGROUP_SLIDING_DOWN_SLOPE,
    /* 0x2B */ PLAYER_ANIMGROUP_END_SLIDING_DOWN_SLOPE,
    /* 0x2C */ PLAYER_ANIMGROUP_RELAX,
    /* 0x2D */ PLAYER_ANIMGROUP_MAX
} PlayerAnimGroup;

typedef enum {
    /* 0x00 */ PLAYER_ATTENTIONMODE_NONE,
    /* 0x01 */ PLAYER_ATTENTIONMODE_C_UP,
    /* 0x02 */ PLAYER_ATTENTIONMODE_AIMING,
    /* 0x03 */ PLAYER_ATTENTIONMODE_CUTSCENE,
    /* 0x04 */ PLAYER_ATTENTIONMODE_ITEM_CUTSCENE,
    /* 0x05*/ PLAYER_ATTENTIONMODE_MAX
} PlayerAttentionMode;

typedef enum {
    PLAYER_CSMODE_NONE,
    PLAYER_CSMODE_IDLE,
    PLAYER_CSMODE_TURN_AROUND_SURPRISED_SHORT,
    PLAYER_CSMODE_UNK_3,
    PLAYER_CSMODE_UNK_4,
    PLAYER_CSMODE_SURPRISED,
    PLAYER_CSMODE_UNK_6,
    PLAYER_CSMODE_END,
    PLAYER_CSMODE_WAIT,
    PLAYER_CSMODE_TURN_AROUND_SURPRISED_LONG,
    PLAYER_CSMODE_ENTER_WARP,
    PLAYER_CSMODE_RAISED_BY_WARP,
    PLAYER_CSMODE_FIGHT_STANCE,
    PLAYER_CSMODE_START_GET_SPIRITUAL_STONE,
    PLAYER_CSMODE_GET_SPIRITUAL_STONE,
    PLAYER_CSMODE_END_GET_SPIRITUAL_STONE,
    PLAYER_CSMODE_GET_UP_FROM_DEKU_TREE_STORY,
    PLAYER_CSMODE_SIT_LISTENING_TO_DEKU_TREE_STORY,
    PLAYER_CSMODE_SWORD_INTO_PEDESTAL,
    PLAYER_CSMODE_REACT_TO_QUAKE,
    PLAYER_CSMODE_END_REACT_TO_QUAKE,
    PLAYER_CSMODE_UNK_21,
    PLAYER_CSMODE_WARP_TO_SAGES,
    PLAYER_CSMODE_LOOK_AT_SELF,
    PLAYER_CSMODE_KNOCKED_TO_GROUND,
    PLAYER_CSMODE_GET_UP_FROM_GROUND,
    PLAYER_CSMODE_START_PLAY_OCARINA,
    PLAYER_CSMODE_END_PLAY_OCARINA,
    PLAYER_CSMODE_GET_ITEM,
    PLAYER_CSMODE_IDLE_2,
    PLAYER_CSMODE_DRAW_AND_BRANDISH_SWORD,
    PLAYER_CSMODE_CLOSE_EYES,
    PLAYER_CSMODE_OPEN_EYES,
    PLAYER_CSMODE_SURPRIED_STUMBLE_BACK_AND_FALL,
    PLAYER_CSMODE_SURFACE_FROM_DIVE,
    PLAYER_CSMODE_GET_ITEM_IN_WATER,
    PLAYER_CSMODE_GENTLE_KNOCKBACK_INTO_SIT,
    PLAYER_CSMODE_GRABBED_AND_CARRIED_BY_NECK,
    PLAYER_CSMODE_SLEEPING_RESTLESS,
    PLAYER_CSMODE_SLEEPING,
    PLAYER_CSMODE_AWAKEN,
    PLAYER_CSMODE_GET_OFF_BED,
    PLAYER_CSMODE_BLOWN_BACKWARD,
    PLAYER_CSMODE_STAND_UP_AND_WATCH,
    PLAYER_CSMODE_IDLE_3,
    PLAYER_CSMODE_STOP,
    PLAYER_CSMODE_STOP_2,
    PLAYER_CSMODE_LOOK_THROUGH_PEEPHOLE,
    PLAYER_CSMODE_STEP_BACK_CAUTIOUSLY,
    PLAYER_CSMODE_IDLE_4,
    PLAYER_CSMODE_DRAW_SWORD_CHILD,
    PLAYER_CSMODE_JUMP_TO_ZELDAS_CRYSTAL,
    PLAYER_CSMODE_DESPERATE_LOOKING_AT_ZELDAS_CRYSTAL,
    PLAYER_CSMODE_LOOK_UP_AT_ZELDAS_CRYSTAL_VANISHING,
    PLAYER_CSMODE_TURN_AROUND_SLOWLY,
    PLAYER_CSMODE_END_SHIELD_EYES_WITH_HAND,
    PLAYER_CSMODE_SHIELD_EYES_WITH_HAND,
    PLAYER_CSMODE_LOOK_AROUND_SURPRISED,
    PLAYER_CSMODE_INSPECT_GROUND_CAREFULLY,
    PLAYER_CSMODE_STARTLED_BY_GORONS_FALLING,
    PLAYER_CSMODE_FALL_TO_KNEE,
    PLAYER_CSMODE_FLAT_ON_BACK,
    PLAYER_CSMODE_RAISE_FROM_FLAT_ON_BACK,
    PLAYER_CSMODE_START_SPIN_ATTACK,
    PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_IDLE,
    PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_START_PASS_OCARINA,
    PLAYER_CSMODE_ZELDA_CLOUDS_CUTSCENE_END_PASS_OCARINA,
    PLAYER_CSMODE_START_LOOK_AROUND_AFTER_SWORD_WARP,
    PLAYER_CSMODE_END_LOOK_AROUND_AFTER_SWORD_WARP,
    PLAYER_CSMODE_LOOK_AROUND_AND_AT_SELF_QUICKLY,
    PLAYER_CSMODE_START_LEARN_OCARINA_SONG_ADULT,
    PLAYER_CSMODE_END_LEARN_OCARINA_SONG_ADULT,
    PLAYER_CSMODE_START_LEARN_OCARINA_SONG_CHILD,
    PLAYER_CSMODE_END_LEARN_OCARINA_SONG_CHILD,
    PLAYER_CSMODE_RESIST_DARK_MAGIC,
    PLAYER_CSMODE_TRIFORCE_HAND_RESONATES,
    PLAYER_CSMODE_STARE_DOWN_STARTLED,
    PLAYER_CSMODE_LOOK_UP_STARTLED,
    PLAYER_CSMODE_LOOK_TO_CHARACTER_AT_SIDE_SMILING,
    PLAYER_CSMODE_LOOK_TO_CHARACTER_ABOVE_SMILING,
    PLAYER_CSMODE_SURPRISED_DEFENSE,
    PLAYER_CSMODE_START_HALF_TURN_SURPRISED,
    PLAYER_CSMODE_END_HALF_TURN_SURPRISED,
    PLAYER_CSMODE_START_LOOK_UP_DEFENSE,
    PLAYER_CSMODE_LOOK_UP_DEFENSE_IDLE,
    PLAYER_CSMODE_END_LOOK_UP_DEFENSE,
    PLAYER_CSMODE_START_SWORD_KNOCKED_FROM_HAND,
    PLAYER_CSMODE_SWORD_KNOCKED_FROM_HAND_IDLE,
    PLAYER_CSMODE_END_SWORD_KNOCKED_FROM_HAND,
    PLAYER_CSMODE_SPIN_ATTACK_IDLE,
    PLAYER_CSMODE_INSPECT_WEAPON,
    PLAYER_CSMODE_UNK_91,
    PLAYER_CSMODE_KNOCKED_TO_GROUND_WITH_DAMAGE_EFFECT,
    PLAYER_CSMODE_REACT_TO_HEAT,
    PLAYER_CSMODE_GET_SWORD_BACK,
    PLAYER_CSMODE_CAUGHT_BY_GUARD,
    PLAYER_CSMODE_GET_SWORD_BACK_2,
    PLAYER_CSMODE_START_GANON_KILL_COMBO,
    PLAYER_CSMODE_END_GANON_KILL_COMBO,
    PLAYER_CSMODE_WATCH_ZELDA_STUN_GANON,
    PLAYER_CSMODE_START_LOOK_AT_SWORD_GLOW,
    PLAYER_CSMODE_LOOK_AT_SWORD_GLOW_IDLE,
    PLAYER_CSMODE_END_LOOK_AT_SWORD_GLOW,
    PLAYER_CSMODE_MAX
} PlayerCutsceneMode;

typedef enum {
    /* 0x00 */ PLAYER_MOVESFXTYPE_DEFAULT,
    /* 0x01 */ PLAYER_MOVESFXTYPE_SAND,
    /* 0x02 */ PLAYER_MOVESFXTYPE_STONE,
    /* 0x03 */ PLAYER_MOVESFXTYPE_DIRT,
    /* 0x04 */ PLAYER_MOVESFXTYPE_SHALLOW_WATER,
    /* 0x05 */ PLAYER_MOVESFXTYPE_DEEP_WATER,
    /* 0x06 */ PLAYER_MOVESFXTYPE_TALL_GRASS,
    /* 0x07 */ PLAYER_MOVESFXTYPE_MAGMA,
    /* 0x08 */ PLAYER_MOVESFXTYPE_GRASS,
    /* 0x09 */ PLAYER_MOVESFXTYPE_WOOD_PLANK,
    /* 0x0A */ PLAYER_MOVESFXTYPE_WOOD,
    /* 0x0B */ PLAYER_MOVESFXTYPE_GRASS_2,
    /* 0x0C */ PLAYER_MOVESFXTYPE_ICE,
    /* 0x0D */ PLAYER_MOVESFXTYPE_CARPET,
    /* 0x0E */ PLAYER_MOVESFXTYPE_MAX
} PlayerMoveSfxType;

typedef enum {
    /* -1 */ PLAYER_CLIMBSTATUS_MOVING_DOWN = -1,
    /*  0 */ PLAYER_CLIMBSTATUS_MOVING_UP,
    /*  1 */ PLAYER_CLIMBSTATUS_KNOCKED_DOWN
} PlayerClimbStatus;

typedef enum {
    /* 0x00 */ PLAYER_MELEEWEAPON_NONE,
    /* 0x01 */ PLAYER_MELEEWEAPON_SWORD_MASTER,
    /* 0x02 */ PLAYER_MELEEWEAPON_SWORD_KOKIRI,
    /* 0x03 */ PLAYER_MELEEWEAPON_SWORD_BGS,
    /* 0x04 */ PLAYER_MELEEWEAPON_STICK,
    /* 0x05 */ PLAYER_MELEEWEAPON_HAMMER,
    /* 0x06 */ PLAYER_MELEEWEAPON_MAX
} PlayerMeleeWeapon;

typedef enum {
    /* 0x00 */ PLAYER_BOTTLECONTENTS_NONE,
    /* 0x01 */ PLAYER_BOTTLECONTENTS_FISH,
    /* 0x02 */ PLAYER_BOTTLECONTENTS_FIRE,
    /* 0x03 */ PLAYER_BOTTLECONTENTS_BUG,
    /* 0x04 */ PLAYER_BOTTLECONTENTS_POE,
    /* 0x05 */ PLAYER_BOTTLECONTENTS_BIG_POE,
    /* 0x06 */ PLAYER_BOTTLECONTENTS_LETTER,
    /* 0x07 */ PLAYER_BOTTLECONTENTS_POTION_RED,
    /* 0x08 */ PLAYER_BOTTLECONTENTS_POTION_BLUE,
    /* 0x09 */ PLAYER_BOTTLECONTENTS_POTION_GREEN,
    /* 0x0A */ PLAYER_BOTTLECONTENTS_MILK,
    /* 0x0B */ PLAYER_BOTTLECONTENTS_MILK_HALF,
    /* 0x0C */ PLAYER_BOTTLECONTENTS_FAIRY,
    /* 0x0D */ PLAYER_BOTTLECONTENTS_MAX
} PlayerBottleContents;

typedef enum {
    /* 0x00 */ PLAYER_MAGICSPELL_UNUSED_15,
    /* 0x01 */ PLAYER_MAGICSPELL_UNUSED_16,
    /* 0x02 */ PLAYER_MAGICSPELL_UNUSED_17,
    /* 0x03 */ PLAYER_MAGICSPELL_FARORES_WIND,
    /* 0x04 */ PLAYER_MAGICSPELL_NAYRUS_LOVE,
    /* 0x05 */ PLAYER_MAGICSPELL_DINS_FIRE,
    /* 0x06 */ PLAYER_MAGICSPELLS_MAX
} PlayerMagicSpells;

typedef enum {
    /* -1 */ PLAYER_RELATIVESTICKINPUT_NONE = -1,
    /*  0 */ PLAYER_RELATIVESTICKINPUT_FORWARD,
    /*  1 */ PLAYER_RELATIVESTICKINPUT_LEFT,
    /*  2 */ PLAYER_RELATIVESTICKINPUT_BACKWARD,
    /*  3 */ PLAYER_RELATIVESTICKINPUT_RIGHT
} PlayerRelativeAnalogStickInputs;

typedef enum {
    /* 0x00 */ PLAYER_WALLJUMPTYPE_NONE,
    /* 0x01 */ PLAYER_WALLJUMPTYPE_HOP_UP,
    /* 0x02 */ PLAYER_WALLJUMPTYPE_SMALL_CLIMB_UP,
    /* 0x03 */ PLAYER_WALLJUMPTYPE_LARGE_CLIMB_UP,
    /* 0x04 */ PLAYER_WALLJUMPTYPE_JUMP_UP_TO_LEDGE
} PlayerTouchedWallJumpTypes;

typedef enum {
    /* 0x00 */ PLAYER_SPAWNMODE_NO_UPDATE_OR_DRAW,
    /* 0x01 */ PLAYER_SPAWNMODE_FROM_TIME_TRAVEL,
    /* 0x02 */ PLAYER_SPAWNMODE_FROM_BLUE_WARP,
    /* 0x03 */ PLAYER_SPAWNMODE_OPENING_DOOR,
    /* 0x04 */ PLAYER_SPAWNMODE_EXITING_GROTTO,
    /* 0x05 */ PLAYER_SPAWNMODE_FROM_WARP_SONG,
    /* 0x06 */ PLAYER_SPAWNMODE_FROM_FARORES_WIND,
    /* 0x07 */ PLAYER_SPAWNMODE_WITH_KNOCKBACK,
    /* 0x08 */ PLAYER_SPAWNMODE_UNK_8,
    /* 0x09 */ PLAYER_SPAWNMODE_UNK_9,
    /* 0x0A */ PLAYER_SPAWNMODE_UNK_10,
    /* 0x0B */ PLAYER_SPAWNMODE_UNK_11,
    /* 0x0C */ PLAYER_SPAWNMODE_UNK_12,
    /* 0x0D */ PLAYER_SPAWNMODE_NO_MOMENTUM,
    /* 0x0E */ PLAYER_SPAWNMODE_WALKING_SLOW,
    /* 0x0F */ PLAYER_SPAWNMODE_WALKING_PRESERVE_MOMENTUM
} PlayerSpawnModes;

#define PLAYER_ANIMSFXFLAGS_0 (1 << 11) // 0x0800
#define PLAYER_ANIMSFXFLAGS_1 (1 << 12) // 0x1000
#define PLAYER_ANIMSFXFLAGS_2 (1 << 13) // 0x2000
#define PLAYER_ANIMSFXFLAGS_3 (1 << 14) // 0x4000

#define PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_X (1 << 0) // 0x00001
#define PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Y (1 << 1) // 0x00002
#define PLAYER_LOOKFLAGS_OVERRIDE_FOCUS_ROT_Z (1 << 2) // 0x00004
#define PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_X (1 << 3) // 0x00008
#define PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Y (1 << 4) // 0x00010
#define PLAYER_LOOKFLAGS_OVERRIDE_HEAD_ROT_Z (1 << 5) // 0x00020
#define PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_X (1 << 6) // 0x00040
#define PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Y (1 << 7) // 0x00080
#define PLAYER_LOOKFLAGS_OVERRIDE_UPPERBODY_ROT_Z (1 << 8) // 0x00100

#define PLAYER_ANIMMOVEFLAGS_UPDATE_XZ (1 << 0)
#define PLAYER_ANIMMOVEFLAGS_UPDATE_Y (1 << 1)
#define PLAYER_ANIMMOVEFLAGS_NO_AGE_Y_TRANSLATION_SCALE (1 << 2)
#define PLAYER_ANIMMOVEFLAGS_KEEP_ANIM_Y_TRANSLATION (1 << 3)
#define PLAYER_ANIMMOVEFLAGS_NO_MOVE (1 << 4)
#define PLAYER_ANIMMOVEFLAGS_5 (1 << 5)
#define PLAYER_ANIMMOVEFLAGS_6 (1 << 6)
#define PLAYER_ANIMMOVEFLAGS_7 (1 << 7)
#define PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT (1 << 8)
#define PLAYER_ANIMMOVEFLAGS_UPDATE_PREV_TRANSL_ROT_APPLY_AGE_SCALE (1 << 9)

#define LIMB_BUF_COUNT(limbCount) ((ALIGN16((limbCount) * sizeof(Vec3s)) + sizeof(Vec3s) - 1) / sizeof(Vec3s))
#define PLAYER_LIMB_BUF_COUNT LIMB_BUF_COUNT(PLAYER_LIMB_MAX)

typedef struct {
    /* 0x00 */ f32 ceilingCheckHeight;
    /* 0x04 */ f32 unk_04;
    /* 0x08 */ f32 translationScale;
    /* 0x0C */ f32 unk_0C;
    /* 0x10 */ f32 unk_10;
    /* 0x14 */ f32 unk_14;
    /* 0x18 */ f32 unk_18;
    /* 0x1C */ f32 unk_1C;
    /* 0x20 */ f32 unk_20;
    /* 0x24 */ f32 unk_24;
    /* 0x28 */ f32 waterSurface;
    /* 0x2C */ f32 diveWaterSurface;
    /* 0x30 */ f32 unk_30;
    /* 0x34 */ f32 unk_34;
    /* 0x38 */ f32 wallCheckRadius;
    /* 0x3C */ f32 unk_3C;
    /* 0x40 */ f32 unk_40;
    /* 0x44 */ Vec3s unk_44;
    /* 0x4A */ Vec3s unk_4A[4];
    /* 0x62 */ Vec3s unk_62[4];
    /* 0x7A */ Vec3s unk_7A[2];
    /* 0x86 */ Vec3s unk_86[2];
    /* 0x92 */ u16 ageVoiceSfxOffset;
    /* 0x94 */ u16 ageMoveSfxOffset;
    /* 0x98 */ LinkAnimationHeader* chestOpenAnim;
    /* 0x9C */ LinkAnimationHeader* timeTravelStartAnim;
    /* 0xA0 */ LinkAnimationHeader* timeTravelEndAnim;
    /* 0xA4 */ LinkAnimationHeader* startClimb1Anim;
    /* 0xA8 */ LinkAnimationHeader* startClimb2Anim;
    /* 0xAC */ LinkAnimationHeader* verticalClimbAnim[4];
    /* 0xBC */ LinkAnimationHeader* horizontalClimbAnim[2];
    /* 0xC4 */ LinkAnimationHeader* endClimb1Anim[2];
    /* 0xCC */ LinkAnimationHeader* endClimb2Anim[2];
} PlayerAgeProperties; // size = 0xD4

typedef struct {
    /* 0x00 */ s32 active;
    /* 0x04 */ Vec3f tip;
    /* 0x10 */ Vec3f base;
} WeaponInfo; // size = 0x1C

#define PLAYER_STATE1_EXITING_SCENE (1 << 0)
#define PLAYER_STATE1_SWINGING_BOTTLE (1 << 1)
#define PLAYER_STATE1_END_HOOKSHOT_MOVE (1 << 2)
#define PLAYER_STATE1_AIMING_FPS_ITEM (1 << 3)
#define PLAYER_STATE1_TARGETING (1 << 4)
#define PLAYER_STATE1_INPUT_DISABLED (1 << 5)
#define PLAYER_STATE1_TALKING (1 << 6)
#define PLAYER_STATE1_IN_DEATH_CUTSCENE (1 << 7)
#define PLAYER_STATE1_USE_ITEM_IMMEDIATELY (1 << 8)
#define PLAYER_STATE1_PREPARED_TO_SHOOT (1 << 9)
#define PLAYER_STATE1_GETTING_ITEM (1 << 10)
#define PLAYER_STATE1_HOLDING_ACTOR (1 << 11)
#define PLAYER_STATE1_CHARGING_SPIN_ATTACK (1 << 12)
#define PLAYER_STATE1_HANGING_FROM_LEDGE_SLIP (1 << 13)
#define PLAYER_STATE1_CLIMBING_ONTO_LEDGE (1 << 14)
#define PLAYER_STATE1_UNUSED_TARGETING_FLAG (1 << 15)
#define PLAYER_STATE1_FORCE_STRAFING (1 << 16)
#define PLAYER_STATE1_Z_PARALLEL_MODE (1 << 17)
#define PLAYER_STATE1_JUMPING (1 << 18)
#define PLAYER_STATE1_FREEFALLING (1 << 19)
#define PLAYER_STATE1_IN_FIRST_PERSON_MODE (1 << 20)
#define PLAYER_STATE1_CLIMBING (1 << 21)
#define PLAYER_STATE1_SHIELDING (1 << 22)
#define PLAYER_STATE1_RIDING_HORSE (1 << 23)
#define PLAYER_STATE1_AIMING_BOOMERANG (1 << 24)
#define PLAYER_STATE1_AWAITING_THROWN_BOOMERANG (1 << 25)
#define PLAYER_STATE1_TAKING_DAMAGE (1 << 26)
#define PLAYER_STATE1_SWIMMING (1 << 27)
#define PLAYER_STATE1_SKIP_OTHER_ACTORS_UPDATE (1 << 28)
#define PLAYER_STATE1_IN_CUTSCENE (1 << 29)
#define PLAYER_STATE1_UNK_Z_PARALLEL_STATE (1 << 30)
#define PLAYER_STATE1_FALLING_INTO_GROTTO_OR_VOID (1 << 31)

#define PLAYER_STATE2_CAN_GRAB_PUSH_PULL_WALL (1 << 0)
#define PLAYER_STATE2_CAN_SPEAK_OR_CHECK (1 << 1)
#define PLAYER_STATE2_CAN_CLIMB_PUSH_PULL_WALL (1 << 2)
#define PLAYER_STATE2_MAKING_REACTABLE_NOISE (1 << 3)
#define PLAYER_STATE2_MOVING_PUSH_PULL_WALL (1 << 4)
#define PLAYER_STATE2_DISABLE_MOVE_ROTATION_WHILE_TARGETING (1 << 5)
#define PLAYER_STATE2_ALWAYS_DISABLE_MOVE_ROTATION (1 << 6)
#define PLAYER_STATE2_RESTRAINED_BY_ENEMY (1 << 7)
#define PLAYER_STATE2_ENABLE_PUSH_PULL_CAM (1 << 8)
#define PLAYER_STATE2_SPAWN_DUST_AT_FEET (1 << 9)
#define PLAYER_STATE2_DIVING (1 << 10)
#define PLAYER_STATE2_ENABLE_DIVE_CAMERA_AND_TIMER (1 << 11)
#define PLAYER_STATE2_IDLE_WHILE_CLIMBING (1 << 12)
#define PLAYER_STATE2_USING_SWITCH_TARGETING (1 << 13)
#define PLAYER_STATE2_FROZEN_IN_ICE (1 << 14)
#define PLAYER_STATE2_PAUSE_MOST_UPDATING (1 << 15)
#define PLAYER_STATE2_CAN_ENTER_CRAWLSPACE (1 << 16)
#define PLAYER_STATE2_RELEASING_SPIN_ATTACK (1 << 17)
#define PLAYER_STATE2_INSIDE_CRAWLSPACE (1 << 18)
#define PLAYER_STATE2_BACKFLIPPING_OR_SIDEHOPPING (1 << 19)
#define PLAYER_STATE2_NAVI_IS_ACTIVE (1 << 20)
#define PLAYER_STATE2_NAVI_REQUESTING_TALK (1 << 21)
#define PLAYER_STATE2_CAN_DISMOUNT_HORSE (1 << 22)
#define PLAYER_STATE2_NEAR_OCARINA_ACTOR (1 << 23)
#define PLAYER_STATE2_ATTEMPT_PLAY_OCARINA_FOR_ACTOR (1 << 24)
#define PLAYER_STATE2_PLAYING_OCARINA_FOR_ACTOR (1 << 25)
#define PLAYER_STATE2_ENABLE_REFLECTION (1 << 26)
#define PLAYER_STATE2_PLAYING_OCARINA_GENERAL (1 << 27)
#define PLAYER_STATE2_IDLING (1 << 28)
#define PLAYER_STATE2_DISABLE_DRAW (1 << 29)
#define PLAYER_STATE2_ENABLE_FORWARD_SLIDE_FROM_ATTACK (1 << 30)
#define PLAYER_STATE2_FORCE_VOID_OUT (1 << 31)

#define PLAYER_STATE3_IGNORE_CEILING_FLOOR_AND_WATER (1 << 0)
#define PLAYER_STATE3_MIDAIR (1 << 1)
#define PLAYER_STATE3_2 (1 << 2)
#define PLAYER_STATE3_ENDING_MELEE_ATTACK (1 << 3)
#define PLAYER_STATE3_CHECKING_FLOOR_AND_WATER_COLLISION (1 << 4)
#define PLAYER_STATE3_FORCE_PULL_OCARINA (1 << 5)
#define PLAYER_STATE3_RESTORE_NAYRUS_LOVE (1 << 6)
#define PLAYER_STATE3_MOVING_ALONG_HOOKSHOT_PATH (1 << 7)

typedef void (*PlayerActionFunc)(struct Player*, struct PlayState*);
typedef s32 (*PlayerUpperActionFunc)(struct Player*, struct PlayState*);
typedef void (*PlayerMiniCsFunc)(struct PlayState*, struct Player*);

typedef struct Player {
    /* 0x0000 */ Actor      actor;
    /* 0x014C */ s8         currentTunic; // current tunic from `PlayerTunic`
    /* 0x014D */ s8         currentSwordItemId;
    /* 0x014E */ s8         currentShield; // current shield from `PlayerShield`
    /* 0x014F */ s8         currentBoots; // current boots from `PlayerBoots`
    /* 0x0150 */ s8         heldItemButton; // Button index for the item currently used
    /* 0x0151 */ s8         heldItemActionParam; // Action param for the item currently used
    /* 0x0152 */ u8         heldItemId; // Item id for the item currently used
    /* 0x0153 */ s8         prevBoots; // previous boots from `PlayerBoots`
    /* 0x0154 */ s8         itemActionParam; // the difference between this and heldItemActionParam is unclear
    /* 0x0155 */ char       unk_155[0x003];
    /* 0x0158 */ u8         modelGroup;
    /* 0x0159 */ u8         nextModelGroup;
    /* 0x015A */ s8         unk_15A;
    /* 0x015B */ u8         modelAnimType;
    /* 0x015C */ u8         leftHandType;
    /* 0x015D */ u8         rightHandType;
    /* 0x015E */ u8         sheathType;
    /* 0x015F */ u8         currentMask; // current mask equipped from `PlayerMask`
    /* 0x0160 */ Gfx**      rightHandDLists;
    /* 0x0164 */ Gfx**      leftHandDLists;
    /* 0x0168 */ Gfx**      sheathDLists;
    /* 0x016C */ Gfx**      waistDLists;
    /* 0x0170 */ u8         giObjectLoading;
    /* 0x0174 */ DmaRequest giObjectDmaRequest;
    /* 0x0194 */ OSMesgQueue giObjectLoadQueue;
    /* 0x01AC */ OSMesg     giObjectLoadMsg;
    /* 0x01B0 */ void*      giObjectSegment; // also used for title card textures
    /* 0x01B4 */ SkelAnime  skelAnime;
    /* 0x01F8 */ Vec3s      jointTable[PLAYER_LIMB_BUF_COUNT];
    /* 0x0288 */ Vec3s      morphTable[PLAYER_LIMB_BUF_COUNT];
    /* 0x0318 */ Vec3s      blendTable[PLAYER_LIMB_BUF_COUNT];
    /* 0x03A8 */ s16        unk_3A8[2];
    /* 0x03AC */ Actor*     heldActor;
    /* 0x03B0 */ Vec3f      leftHandPos;
    /* 0x03BC */ Vec3s      unk_3BC;
    /* 0x03C4 */ Actor*     unk_3C4;
    /* 0x03C8 */ Vec3f      unk_3C8;
    /* 0x03D4 */ char       unk_3D4[0x058];
    /* 0x042C */ s8         doorType;
    /* 0x042D */ s8         doorDirection;
    /* 0x042E */ s16        doorTimer;
    /* 0x0430 */ Actor*     doorActor;
    /* 0x0434 */ s8         getItemId;
    /* 0x0436 */ u16        getItemDirection;
    /* 0x0438 */ Actor*     interactRangeActor;
    /* 0x043C */ s8         mountSide;
    /* 0x043D */ char       unk_43D[0x003];
    /* 0x0440 */ Actor*     rideActor;
    /* 0x0444 */ u8         csMode;
    /* 0x0445 */ u8         prevCsMode;
    /* 0x0446 */ u8         unk_446;
    /* 0x0447 */ u8         unk_447;
    /* 0x0448 */ Actor*     csTargetActor;
    /* 0x044C */ char       unk_44C[0x004];
    /* 0x0450 */ Vec3f      unk_450;
    /* 0x045C */ Vec3f      unk_45C;
    /* 0x0468 */ char       unk_468[0x002];
    union {
        /* 0x046A */ s16 csFlag;
        /* 0x046A */ s16 doorBgCamIndex;
    };
    /* 0x046C */ s16        subCamId;
    /* 0x046E */ char       unk_46E[0x02A];
    /* 0x0498 */ ColliderCylinder cylinder;
    /* 0x04E4 */ ColliderQuad meleeWeaponQuads[2];
    /* 0x05E4 */ ColliderQuad shieldQuad;
    /* 0x0664 */ Actor*     targetActor;
    /* 0x0668 */ char       unk_668[0x004];
    /* 0x066C */ s32        targetSwitchTimer;
    /* 0x0670 */ s32        meleeWeaponEffectIndex;
    /* 0x0674 */ PlayerActionFunc actionFunc;
    /* 0x0678 */ PlayerAgeProperties* ageProperties;
    /* 0x067C */ u32        stateFlags1;
    /* 0x0680 */ u32        stateFlags2;
    /* 0x0684 */ Actor*     forcedTargetActor;
    /* 0x0688 */ Actor*     boomerangActor;
    /* 0x068C */ Actor*     naviActor;
    /* 0x0690 */ s16        naviTextId;
    /* 0x0692 */ u8         stateFlags3;
    /* 0x0693 */ s8         exchangeItemId;
    /* 0x0694 */ Actor*     talkActor;
    /* 0x0698 */ f32        talkActorDistance;
    /* 0x069C */ char       unk_69C[0x004];
    /* 0x06A0 */ f32        unk_6A0;
    /* 0x06A4 */ f32        unk_6A4;
    /* 0x06A8 */ Actor*     ocarinaActor;
    /* 0x06AC */ s8         unk_6AC;
    /* 0x06AD */ u8         attentionMode;
    /* 0x06AE */ u16        lookFlags;
    /* 0x06B0 */ s16        unk_6B0;
    /* 0x06B2 */ char       unk_6B4[0x004];
    /* 0x06B6 */ Vec3s      headRot;
    /* 0x06BC */ Vec3s      upperBodyRot;
    /* 0x06C2 */ s16        unk_6C2;
    /* 0x06C4 */ f32        sinkingOffsetY;
    /* 0x06C8 */ SkelAnime  skelAnime2;
    /* 0x070C */ Vec3s      jointTable2[PLAYER_LIMB_BUF_COUNT];
    /* 0x079C */ Vec3s      morphTable2[PLAYER_LIMB_BUF_COUNT];
    /* 0x082C */ PlayerUpperActionFunc upperActionFunc;
    /* 0x0830 */ f32        unk_830;
    /* 0x0834 */ s16        fpsItemTimer;
    /* 0x0836 */ s8         unk_836;
    /* 0x0837 */ u8         unk_837;
    /* 0x0838 */ f32        linearVelocity;
    /* 0x083C */ s16        currentYaw;
    /* 0x083E */ s16        targetYaw;
    /* 0x0840 */ u16        unk_840;
    /* 0x0842 */ s8         meleeWeaponAnimation;
    /* 0x0843 */ s8         isMeleeWeaponAttacking;
    /* 0x0844 */ s8         comboTimer;
    /* 0x0845 */ u8         slashCounter;
    /* 0x0846 */ u8         inputFrameCounter;
    /* 0x0847 */ s8         analogStickInputs[4];
    /* 0x084B */ s8         relativeAnalogStickInputs[4];
    union {
        /* 0x084F */ s8 unk_84F;
        /* 0x084F */ s8 relativeStickInput;
        /* 0x084F */ s8 climbStatus;
    };
    union {
        /* 0x0850 */ s16        genericTimer; // multipurpose timer
        /* 0x0850 */ s16        mashTimer;
    };
    /* 0x0854 */ f32        rippleTimer;
    union {
        /* 0x0858 */ f32        spinAttackTimer;
        /* 0x0858 */ f32        reelPullDir;
    };
    union {
        /* 0x085C */ f32        unk_85C; // stick length among other things
        /* 0x085C */ f32        dekuStickLength;
    };
    union {
        /* 0x0860 */ s16 fpsItemType;
        /* 0x0860 */ s16 stickFlameTimer;
        /* 0x0860 */ s16 fishingState;
    };
    /* 0x0862 */ s8         unk_862; // get item draw ID + 1
    /* 0x0864 */ f32        unk_864;
    /* 0x0868 */ f32        walkFrame;
    /* 0x086C */ f32        unk_86C;
    /* 0x0870 */ f32        unk_870;
    /* 0x0874 */ f32        unk_874;
    /* 0x0878 */ f32        unk_878;
    /* 0x087C */ s16        unk_87C;
    /* 0x087E */ s16        unk_87E;
    /* 0x0880 */ f32        speedLimit;
    /* 0x0884 */ f32        wallHeight; // height used to determine whether link can climb or grab a ledge at the top
    /* 0x0888 */ f32        wallDistance; // distance to the colliding wall plane
    /* 0x088C */ u8         touchedWallJumpType;
    /* 0x088D */ u8         wallTouchTimer;
    /* 0x088E */ u8         unk_88E;
    /* 0x088F */ u8         unk_88F;
    /* 0x0890 */ u8         runDamageTimer;
    /* 0x0891 */ u8         shockTimer;
    /* 0x0892 */ u8         unk_892;
    /* 0x0893 */ u8         hoverBootsTimer;
    /* 0x0894 */ s16        fallStartHeight; // last truncated Y position before falling
    /* 0x0896 */ s16        fallDistance; // truncated Y distance the player has fallen so far (positive is down)
    /* 0x0898 */ s16        angleToFloorX;
    /* 0x089A */ s16        angleToFloorY;
    /* 0x089C */ s16        unk_89C;
    /* 0x089E */ u16        moveSfxType;
    /* 0x08A0 */ u8         unk_8A0;
    /* 0x08A1 */ u8         unk_8A1;
    /* 0x08A2 */ s16        unk_8A2;
    /* 0x08A4 */ f32        unk_8A4;
    /* 0x08A8 */ f32        unk_8A8;
    /* 0x08AC */ f32        pushedSpeed; // Pushing player, examples include water currents, floor conveyors, climbing sloped surfaces
    /* 0x08B0 */ s16        pushedYaw; // Yaw direction of player being pushed
    /* 0x08B4 */ WeaponInfo meleeWeaponInfo[3];
    /* 0x0908 */ Vec3f      bodyPartsPos[PLAYER_BODYPART_MAX];
    /* 0x09E0 */ MtxF       mf_9E0;
    /* 0x0A20 */ MtxF       shieldMf;
    /* 0x0A60 */ u8         isBurning;
    /* 0x0A61 */ u8         flameTimers[PLAYER_BODYPART_MAX]; // one flame per body part
    /* 0x0A73 */ u8         unk_A73;
    /* 0x0A74 */ PlayerMiniCsFunc miniCsFunc;
    /* 0x0A78 */ s8         invincibilityTimer; // prevents damage when nonzero (positive = visible, counts towards zero each frame)
    /* 0x0A79 */ u8         unk_A79;
    /* 0x0A7A */ u8         floorProperty;
    /* 0x0A7B */ u8         unk_A7B;
    /* 0x0A7C */ f32        analogStickDistance;
    /* 0x0A80 */ s16        analogStickAngle;
    /* 0x0A82 */ u16        prevMoveSfxType;
    /* 0x0A84 */ s16        unk_A84;
    /* 0x0A86 */ s8         unk_A86;
    /* 0x0A87 */ u8         unk_A87;
    /* 0x0A88 */ Vec3f      prevWaistPos; // previous body part 0 position
} Player; // size = 0xA94

#endif

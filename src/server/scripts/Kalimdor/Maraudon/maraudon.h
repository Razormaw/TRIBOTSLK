/*
 * Maraudon Dungeon - Header File
 * TrinityCore 3.3.5a
 */

#ifndef DEF_MARAUDON_H
#define DEF_MARAUDON_H

#define MAX_ENCOUNTER 9

enum MaraudonData
{
    DATA_VYLETONGUE         = 0,
    DATA_NOXXION            = 1,
    DATA_RAZORLASH          = 2,
    DATA_MESHLOK            = 3,
    DATA_GIZLOCK            = 4,
    DATA_CELEBRAS           = 5,
    DATA_LANDSLIDE          = 6,
    DATA_ROTGRIP            = 7,
    DATA_THERADRAS          = 8
};

enum MaraudonNPCs
{
    NPC_VYLETONGUE          = 3672,
    NPC_NOXXION             = 13280,
    NPC_NOXXION_SPAWN       = 13282,
    NPC_RAZORLASH           = 12257,
    NPC_MESHLOK             = 12258,
    NPC_GIZLOCK             = 12256,
    NPC_PUNY_GIZLOCK        = 14354,
    NPC_CELEBRAS            = 12236,
    NPC_CELEBRAS_REDEEMED   = 13716,
    NPC_LANDSLIDE           = 12259,
    NPC_ROTGRIP             = 12237,
    NPC_THERADRAS           = 12225,
    NPC_ZAETAR_SPIRIT       = 13742
};

enum MaraudonGameObjects
{
    GO_THERADRAS_DOOR       = 178165
};

enum MaraudonSpells
{
    // Noxxion
    SPELL_NOXXION_TOXIC_VOLLEY      = 21667,
    SPELL_NOXXION_UPPERCUT          = 12739,
    SPELL_NOXXION_SUMMON_SPAWNS     = 21708,
    SPELL_NOXXION_FRENZY            = 21709,
    SPELL_NOXXION_SPLIT             = 21710,
    SPELL_NOXXION_MERGE             = 21711,
    
    // Celebras
    SPELL_CELEBRAS_WRATH            = 21807,
    SPELL_CELEBRAS_ROOTS            = 12747,
    SPELL_CELEBRAS_CORRUPT_FORCES   = 21968,
    SPELL_CELEBRAS_SEED             = 21969,
    SPELL_CELEBRAS_TRANSFORM        = 21970,
    
    // Landslide
    SPELL_LANDSLIDE_KNOCK_AWAY      = 18670,
    SPELL_LANDSLIDE_TRAMPLE         = 5568,
    SPELL_LANDSLIDE_ENRAGE          = 21808,
    SPELL_LANDSLIDE_EARTHQUAKE      = 21809,
    
    // Theradras
    SPELL_THERADRAS_DUST_FIELD      = 21810,
    SPELL_THERADRAS_BOULDER         = 21811,
    SPELL_THERADRAS_THRASH          = 21812,
    SPELL_THERADRAS_REPULSIVE_GAZE  = 21813,
    SPELL_THERADRAS_CRYSTAL_PRISON  = 21814,
    SPELL_THERADRAS_CRYSTAL_NOVA    = 21815,
    SPELL_THERADRAS_ENRAGE          = 21816,
    
    // Vyletongue
    SPELL_VYLETONGUE_FRENZY         = 21817,
    SPELL_VYLETONGUE_POISON_CLOUD   = 21818,
    SPELL_VYLETONGUE_TAIL_LASH      = 21819,
    SPELL_VYLETONGUE_SUMMON_ADDS    = 21820,
    
    // Razorlash
    SPELL_RAZORLASH_THRASH          = 21821,
    SPELL_RAZORLASH_ROOT            = 21822,
    SPELL_RAZORLASH_POISON          = 21823,
    SPELL_RAZORLASH_ENRAGE          = 21824,
    
    // Meshlok
    SPELL_MESHLOK_EARTHQUAKE        = 21825,
    SPELL_MESHLOK_STONE_FIST        = 21826,
    SPELL_MESHLOK_ENRAGE            = 21827,
    SPELL_MESHLOK_SUMMON_ADDS       = 21828,
    
    // Gizlock
    SPELL_GIZLOCK_HYPERSPEED        = 21829,
    SPELL_GIZLOCK_BOMB              = 21830,
    SPELL_GIZLOCK_SHIELD            = 21831,
    SPELL_GIZLOCK_ENRAGE            = 21832,
    SPELL_PUNY_GIZLOCK_FLEE         = 21833,
    
    // Rotgrip
    SPELL_ROTGRIP_BITE              = 21834,
    SPELL_ROTGRIP_TAIL_SWIPE        = 21835,
    SPELL_ROTGRIP_ENRAGE            = 21836,
    SPELL_ROTGRIP_SUBMERGE          = 21837,
    SPELL_ROTGRIP_WHIRLPOOL         = 21838
};

#endif


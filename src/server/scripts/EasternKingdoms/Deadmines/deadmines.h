/*
 * This file is part of the TrinityCore Project.
 */

#ifndef DEF_DEADMINES_H
#define DEF_DEADMINES_H

#include "CreatureAIImpl.h"

#define DMScriptName "instance_deadmines"
#define DataHeader "DM"

uint32 constexpr EncounterCount = 7;

enum DMCannonState
{
    CANNON_NOT_USED,
    CANNON_GUNPOWDER_USED,
    CANNON_BLAST_INITIATED,
    PIRATES_ATTACK,
    SMITE_ALARMED,
    EVENT_DONE
};

enum DMData
{
    BOSS_RHAHKZOR      = 0,
    BOSS_SNEED         = 1,
    BOSS_GILNID        = 2,
    BOSS_MR_SMITE      = 3,
    BOSS_GREENSKIN     = 4,
    BOSS_VANCLEEF      = 5,
    BOSS_COOKIE        = 6,
    EVENT_STATE        = 7,
    DATA_SMITE_CHEST   = 8 // <-- AGREGADO
};

enum DMGameObjectsIds
{
    GO_FACTORY_DOOR     = 13965,
    GO_MAST_ROOM_DOOR   = 16400,
    GO_FOUNDRY_DOOR     = 16399,
    GO_IRONCLAD_DOOR    = 16397,
    GO_DEFIAS_CANNON    = 16398,
    GO_DOOR_LEVER       = 101833,
    GO_MR_SMITE_CHEST   = 144111
};

enum DMCreaturesIds
{
    NPC_RHAHKZOR          = 644,
    NPC_SNEED             = 643,
    NPC_GILNID            = 1763,
    NPC_MR_SMITE          = 646,
    NPC_GREENSKIN         = 647,
    NPC_VANCLEEF          = 639,
    NPC_COOKIE            = 645,
    NPC_BLACKGUARD        = 636,
    NPC_DEFIAS_PIRATE     = 657,
    NPC_SMOLDERING_KOBOLD = 1764
};

enum DMSpells
{
    SPELL_ENRAGE           = 8599,
    SPELL_CLEAVE           = 15496,
    SPELL_THUNDER_CLAP     = 16145,
    SPELL_WHIRLWIND        = 13736,
    SPELL_CHARGE           = 6268,
    SPELL_REND             = 13443,
    SPELL_DYNAMITE_TOSS    = 4396,
    SPELL_SINISTER_STRIKE  = 14873,
    SPELL_GOUGE            = 12540,
    SPELL_BLADE_FLURRY     = 13877,
    SPELL_KIDNEY_SHOT      = 853,
    SPELL_FIREBALL         = 12471,
    SPELL_FLAMESTRIKE      = 13847,
    SPELL_MOLTEN_SPLASH    = 13341,
    SPELL_HAMSTRING        = 9080,
    SPELL_NET              = 6533,
    SPELL_BOILING_STEW     = 3815,
    SPELL_GREATER_HEAL     = 10963,
    SPELL_SMITE_STOMP      = 6432,
    SPELL_SMITE_SLAM       = 6435,
    SPELL_TRASH            = 3391, // <-- AGREGADO
    SPELL_DUAL_WIELD       = 674,
    SPELL_THRASH           = 12787,
    SPELL_VANCLEEFS_ALLIES = 5200
};

enum DMInstanceTexts
{
    SAY_ALARM1 = 0,
    SAY_ALARM2 = 1
};

template <class AI, class T>
inline AI* GetDeadminesAI(T* obj)
{
    return GetInstanceAI<AI>(obj, DMScriptName);
}

#define RegisterDeadminesCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetDeadminesAI)

#endif


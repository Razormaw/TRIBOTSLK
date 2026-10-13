/*
 * This file is part of the TrinityCore Project.
 */

#ifndef DEF_SHADOWFANG_H
#define DEF_SHADOWFANG_H

#include "CreatureAIImpl.h"

#define SFKScriptName "instance_shadowfang_keep"
#define DataHeader "SK"

enum SKDataTypes
{
    TYPE_FREE_NPC               = 1,
    TYPE_RETHILGORE             = 2,
    TYPE_FENRUS                 = 3,
    TYPE_NANDOS                 = 4,
    BOSS_ARUGAL                 = 5,
    DATA_APOTHECARY_HUMMEL      = 6,
    DATA_SPAWN_VALENTINE_ADDS   = 7
};

enum SKCreatures
{
    NPC_ASH                             = 3850,
    NPC_ADA                             = 3849,
    NPC_ARCHMAGE_ARUGAL                 = 4275,
    NPC_ARUGAL_VOIDWALKER               = 4627,
    NPC_DND_CRAZED_APOTHECARY_GENERATOR = 36212,
    // Nuevos jefes intermedios
    NPC_RETHILGORE                      = 3914,
    NPC_RAZORCLAW                       = 3886,
    NPC_ODO                             = 4279,
    NPC_SILVERLAINE                     = 3887,
    NPC_SPRINGVALE                      = 4278,
    NPC_FENRUS                          = 4274,
    NPC_NANDOS                          = 3927,
    NPC_WOLF                            = 3928
};

enum SKGameObjects
{
    GO_COURTYARD_DOOR   = 18895,
    GO_SORCERER_DOOR    = 18972,
    GO_ARUGAL_DOOR      = 18971
};

enum SKBossSpells
{
    SPELL_ENRAGE            = 8599,
    SPELL_CLEAVE            = 15496,
    SPELL_THUNDERCLAP       = 16145,
    SPELL_FRENZY            = 26835,
    SPELL_BLOODLUST         = 6742,
    SPELL_HOLY_LIGHT        = 11642,
    SPELL_HAMMER_JUSTICE    = 853,
    SPELL_CURSE_OF_BLOOD    = 7621
};

enum SKBossTexts
{
    SAY_BOSS_AGGRO      = 0,
    SAY_BOSS_DEATH      = 1,
    SAY_BOSS_SPECIAL    = 2
};

template <class AI, class T>
inline AI* GetShadowfangKeepAI(T* obj)
{
    return GetInstanceAI<AI>(obj, SFKScriptName);
}

#define RegisterShadowfangKeepCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetShadowfangKeepAI)

#endif


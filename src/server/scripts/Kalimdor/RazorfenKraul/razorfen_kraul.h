/*
 * This file is part of the TrinityCore Project.
 */

#ifndef DEF_RAZORFEN_KRAUL_H
#define DEF_RAZORFEN_KRAUL_H

#include "CreatureAIImpl.h"

#define RFKScriptName "instance_razorfen_kraul"
#define DataHeader "RFK"

#define EVENT_WARD_KEEPER 1

enum RFKData
{
    BOSS_AGATHELOS      = 0,
    BOSS_ROOGUG         = 1,
    BOSS_RAMTUSK        = 2,
    BOSS_GROYAT         = 3,
    BOSS_CHARLGA        = 4
};

enum RFKCreatures
{
    NPC_AGATHELOS       = 4424,
    NPC_ROOGUG          = 4425,
    NPC_RAMTUSK         = 4420,
    NPC_GROYAT          = 4427,
    NPC_CHARLGA         = 4421,
    // Adds
    NPC_GEOMANCER       = 4426,
    NPC_WARDER          = 4422,
    NPC_SHAMAN          = 4423
};

enum RFKSpells
{
    SPELL_ENRAGE        = 8599,
    SPELL_CLEAVE        = 15496,
    SPELL_FRENZY        = 26835,
    SPELL_FIREBALL      = 12471,
    SPELL_HEAL          = 11642
};

enum RFKTexts
{
    SAY_AGGRO       = 0,
    SAY_DEATH       = 1,
    SAY_SPECIAL     = 2
};

template <class AI, class T>
inline AI* GetRazorfenKraulAI(T* obj)
{
    return GetInstanceAI<AI>(obj, RFKScriptName);
}

#define RegisterRazorfenKraulCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetRazorfenKraulAI)

#endif


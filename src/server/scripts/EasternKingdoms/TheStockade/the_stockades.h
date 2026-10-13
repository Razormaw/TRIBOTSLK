/*
 * This file is part of the TrinityCore Project.
 */

#ifndef DEF_THE_STOCKADES_H
#define DEF_THE_STOCKADES_H

#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"

#define DataHeader "TS"

enum StockadesData
{
    DATA_TARGORR            = 0,
    DATA_KAM_DEEPFURY       = 1,
    DATA_HAMHOCK            = 2,
    DATA_DEXTREN_WARD       = 3,
    DATA_BAZIL_THREDD       = 4,
    DATA_BRUEGAL_IRONKNUCKLE= 5,
    EncounterCount          = 6
};

enum StockadesCreatureIds
{
    NPC_TARGORR             = 1718,
    NPC_KAM_DEEPFURY        = 1720,
    NPC_HAMHOCK             = 1719,
    NPC_DEXTREN_WARD        = 1717,
    NPC_BAZIL_THREDD        = 1716,
    NPC_BRUEGAL_IRONKNUCKLE = 1722,
	NPC_DEFIAS_SQUATTER     = 1714, // Para Hamhock
	NPC_DEFIAS_EVOKER       = 1725, // Para Bazil
    NPC_DEFIAS_INSURGENT    = 1715
};

enum StockadesSpells
{
    SPELL_ENRAGE            = 8599,
    SPELL_CLEAVE            = 15496,
    SPELL_HAMSTRING         = 9080,
    SPELL_WHIRLWIND         = 13736,
    SPELL_CHARGE            = 6268,
    SPELL_MORTAL_STRIKE     = 12294,
    SPELL_SHIELD_BASH       = 1671,
    SPELL_HAMMER_JUSTICE    = 853,
    SPELL_SINISTER_STRIKE   = 14873,
    SPELL_GOUGE             = 12540,
    SPELL_BLADE_FLURRY      = 13877,
    SPELL_WOUND_POISON      = 2823,
    SPELL_SLICE_AND_DICE    = 6774,
    SPELL_SHADOW_WORD_PAIN  = 10892,
    SPELL_MIND_BLAST        = 10947,
    SPELL_PSYCHIC_SCREAM    = 13704,
    SPELL_GREATER_HEAL      = 10963,
	SPELL_SHADOW_BOLT_VOLLEY = 15305, // Para Kam
	SPELL_VANISH            = 1856,  // Para Dextren (Visual)
	SPELL_BACKSTAB          = 15698, // Para Dextren
	SPELL_KIDNEY_SHOT       = 853,   // Para Dextren (Stun)
};

enum StockadesTexts
{
    SAY_AGGRO   = 0,
    SAY_DEATH   = 1,
    SAY_SPECIAL = 2
};

#endif


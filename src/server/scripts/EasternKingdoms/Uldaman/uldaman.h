#ifndef DEF_ULDAMAN_H
#define DEF_ULDAMAN_H

#include "CreatureAIImpl.h"

#define UldamanScriptName "instance_uldaman"
#define DataHeader "UD"

// 8 Encuentros: Ironaya, Stone Keepers, Archaedas, Revelosh, Obsidian Sentinel, Ancient Stone Keeper, Galgann, Grimlok
uint32 constexpr EncounterCount = 8;

enum UDDataTypes
{
    DATA_IRONAYA                        = 0,
    DATA_STONE_KEEPERS                  = 1,
    DATA_ARCHAEDAS                      = 2,
    DATA_REVELOSH                       = 3,
    DATA_OBSIDIAN_SENTINEL              = 4,
    DATA_ANCIENT_STONE_KEEPER           = 5,
    DATA_GALGANN                        = 6,
    DATA_GRIMLOK                        = 7,

    DATA_IRONAYA_INTRO,
    DATA_IRONAYA_SEAL_DOOR
};

enum UDCreatureIds
{
    NPC_ARCHAEDAS                       = 2748,
    NPC_STONE_KEEPER                    = 4857,
    NPC_EARTHEN_GUARDIAN                = 7076,
    NPC_EARTHEN_HALLSHAPER              = 7077,
    NPC_EARTHEN_CUSTODIAN               = 7309,
    NPC_VAULT_WARDER                    = 10120,
    NPC_IRONAYA                         = 7228,
    NPC_REVELOSH                        = 2747,
    NPC_OBSIDIAN_SENTINEL               = 4853,
    NPC_ANCIENT_STONE_KEEPER            = 4855,
    NPC_GALGANN                         = 7290,
    NPC_GRIMLOK                         = 4854
};

enum UDGameObjectIds
{
    GO_IRONAYA_SEAL_DOOR                = 124372,
    GO_KEYSTONE                         = 124371,
    GO_TEMPLE_DOOR_TO_KEEPERS           = 124368,
    GO_TEMPLE_DOOR_TO_ARCHAEDAS         = 124367,
    GO_TEMPLE_DOOR_ARCHAEDAS            = 141869,
    GO_ANCIENT_VAULT_DOOR               = 124369
};

enum UDSpawnGroups
{
    SPAWN_GROUP_IRONAYA                 = 334
};

enum UDActions
{
    ACTION_KEEPER_ACTIVATED             = 0,
    ACTION_ARCHAEDAS_AWAKEN             = 1,
    ACTION_ACTIVATE_VAULT_WARDERS       = 2
};

enum UDGameEvents
{
    EVENT_SUB_BOSS_AGGRO                = 2228,
    EVENT_BOSS_AGGRO                    = 2268
};

enum UDFactions
{
    FACTION_TITAN_NEUTRAL               = 416,
    FACTION_TITAN_HOSTILE               = 470
};

template <class AI, class T>
inline AI* GetUldamanAI(T* obj)
{
    return GetInstanceAI<AI>(obj, UldamanScriptName);
}

#define RegisterUldamanCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetUldamanAI)
#define RegisterUldamanGameObjectAI(ai_name) RegisterGameObjectAIWithFactory(ai_name, GetUldamanAI)

#endif


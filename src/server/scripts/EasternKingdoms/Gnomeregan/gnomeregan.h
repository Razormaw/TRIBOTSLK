/*
 * This file is part of the TrinityCore Project.
 */

#ifndef DEF_GNOMEREGAN_H
#define DEF_GNOMEREGAN_H

#include "CreatureAIImpl.h"

#define GNOScriptName "instance_gnomeregan"
#define DataHeader "GNO"

enum GNOGameObjectIds
{
    GO_CAVE_IN_LEFT     = 146085,
    GO_CAVE_IN_RIGHT    = 146086,
    GO_RED_ROCKET       = 103820
};

enum GNOCreatureIds
{
    NPC_BLASTMASTER_EMI_SHORTFUSE   = 7998,
    NPC_CAVERNDEEP_AMBUSHER         = 6207,
    NPC_GRUBBIS                     = 7361,
    NPC_VICIOUS_FALLOUT             = 7079,
    NPC_CHOMPER                     = 6215,
    NPC_ELECTROCUTIONER             = 6235,
    NPC_CROWD_PUMMELER              = 6229,
    NPC_MEKGINEER                   = 7800,
    NPC_MECHANICAL_YETI             = 7801 // Add para Thermaplugg
};

enum GNOData
{
    DATA_BLASTMASTER_EVENT = 0,
    DATA_VICIOUS_FALLOUT   = 1,
    DATA_ELECTROCUTIONER   = 2,
    DATA_CROWD_PUMMELER    = 3,
    DATA_THERMAPLUGG       = 4,
    MAX_ENCOUNTER          = 5
};

enum GNOData64
{
    DATA_GO_CAVE_IN_LEFT,
    DATA_GO_CAVE_IN_RIGHT,
    DATA_NPC_BASTMASTER_EMI_SHORTFUSE
};

template <class AI, class T>
inline AI* GetGnomereganAI(T* obj)
{
    return GetInstanceAI<AI>(obj, GNOScriptName);
}

#define RegisterGnomereganCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetGnomereganAI)

#endif


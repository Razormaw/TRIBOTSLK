/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "blackfathom_deeps.h"
#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Random.h"

Position const LorgusPosition[4] =
{
    { -458.500610f, -38.343079f, -33.474445f, 0.0f },
    { -469.423615f, -88.400513f, -39.265102f, 0.0f },
    { -622.354980f, -10.350100f, -22.777000f, 0.0f },
    { -759.640564f,  16.658913f, -29.159529f, 0.0f }
};

Position const SpawnsLocation[] =
{
    { -768.949f, -174.413f, -25.87f, 3.09f },  // Left side
    { -768.888f, -164.238f, -25.87f, 3.09f },
    { -768.951f, -153.911f, -25.88f, 3.09f },
    { -774.400f, -169.405f, -25.86f, 3.11f },
    { -774.491f, -159.371f, -25.86f, 3.21f },
    { -867.782f, -174.352f, -25.87f, 6.27f },  // Right side
    { -867.875f, -164.089f, -25.87f, 6.27f },
    { -867.859f, -153.927f, -25.88f, 6.27f },
    { -861.823f, -159.018f, -25.87f, 0.00f },
    { -861.524f, -169.387f, -25.87f, 0.18f },
    { -859.827f, -468.425f, -33.88f, 5.63f }   // Morridune
};

class instance_blackfathom_deeps : public InstanceMapScript
{
public:
    instance_blackfathom_deeps() : InstanceMapScript(BFDScriptName, 48) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_blackfathom_deeps_InstanceMapScript(map);
    }

    struct instance_blackfathom_deeps_InstanceMapScript : public InstanceScript
    {
        instance_blackfathom_deeps_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            countFires = 0;
            _requiredDeaths = 0;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_TWILIGHT_LORD_KELRIS:
                    twilightLordKelrisGUID = creature->GetGUID();
                    break;
                case NPC_LORGUS_JETT:
                    creature->SetHomePosition(LorgusPosition[urand(0, 3)]);
                    break;
                // Conteo dinámico de adds del evento al ser invocados
                case NPC_BLACKFATHOM_MIRMIDON:
                case NPC_AKU_MAI_SNAPJAW:
                case NPC_MURKSHALLOW_SOFTSHELL:
                case NPC_AKU_MAI_SERVANT:
                case NPC_BARBED_CRUSTACEAN:
                    if (creature->IsSummon())
                        ++_requiredDeaths;
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            // Cuando muere un add del evento, restamos del contador
            if (unit->IsSummon() && (unit->GetEntry() == NPC_BLACKFATHOM_MIRMIDON ||
                                     unit->GetEntry() == NPC_AKU_MAI_SNAPJAW ||
                                     unit->GetEntry() == NPC_MURKSHALLOW_SOFTSHELL ||
                                     unit->GetEntry() == NPC_AKU_MAI_SERVANT ||
                                     unit->GetEntry() == NPC_BARBED_CRUSTACEAN))
            {
                if (--_requiredDeaths == 0)
                {
                    // Si ya no quedan adds y las 4 fogatas están encendidas, abrimos la puerta
                    if (countFires == 4)
                    {
                        HandleGameObject(mainDoorGUID, true);
                    }
                }
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_FIRE_OF_AKU_MAI_1:
                    shrine1GUID = go->GetGUID();
                    break;
                case GO_FIRE_OF_AKU_MAI_2:
                    shrine2GUID = go->GetGUID();
                    break;
                case GO_FIRE_OF_AKU_MAI_3:
                    shrine3GUID = go->GetGUID();
                    break;
                case GO_FIRE_OF_AKU_MAI_4:
                    shrine4GUID = go->GetGUID();
                    break;
                case GO_SHRINE_OF_GELIHAST:
                    shrineOfGelihastGUID = go->GetGUID();
                    if (GetBossState(DATA_GELIHAST) != DONE)
                        go->SetFlag(GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_ALTAR_OF_THE_DEEPS:
                    altarOfTheDeepsGUID = go->GetGUID();
                    if (GetBossState(DATA_AKU_MAI) != DONE)
                        go->SetFlag(GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_AKU_MAI_DOOR:
                    mainDoorGUID = go->GetGUID();
                    // Si por alguna razón ya se cumplió todo al crear el objeto, abrimos la puerta
                    if (countFires == 4 && _requiredDeaths == 0)
                        HandleGameObject(mainDoorGUID, true);
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_FIRE:
                    countFires = data;
                    
                    // Invocación de oleadas con NPCs correctos (Mirmidones Brazanegra incluidos)
                    switch (countFires)
                    {
                        case 1:
                            // Oleada 1: Mirmidones Brazanegra (los que atacan activamente)
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_BLACKFATHOM_MIRMIDON, SpawnsLocation[0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_BLACKFATHOM_MIRMIDON, SpawnsLocation[2], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_BLACKFATHOM_MIRMIDON, SpawnsLocation[5], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_BLACKFATHOM_MIRMIDON, SpawnsLocation[7], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                            }
                            break;
                        case 2:
                            // Oleada 2: Caparazones blandos
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[1], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[2], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[4], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[5], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[6], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[7], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[8], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_MURKSHALLOW_SOFTSHELL, SpawnsLocation[9], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                            }
                            break;
                        case 3:
                            // Oleada 3: Sirvientes de Aku'mai
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_AKU_MAI_SERVANT, SpawnsLocation[1], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_AKU_MAI_SERVANT, SpawnsLocation[6], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                            }
                            break;
                        case 4:
                            // Oleada 4: Crustáceos con púas
                            if (GameObject* go = instance->GetGameObject(shrine1GUID))
                            {
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[2], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[5], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                                go->SummonCreature(NPC_BARBED_CRUSTACEAN, SpawnsLocation[7], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                            }
                            break;
                    }

                    // Si se encienden las 4 fogatas y ya no hay adds vivos, abrir puerta
                    if (countFires == 4 && _requiredDeaths == 0)
                    {
                        HandleGameObject(mainDoorGUID, true);
                    }
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case DATA_GELIHAST:
                    if (state == DONE)
                        if (GameObject* go = instance->GetGameObject(shrineOfGelihastGUID))
                            go->RemoveFlag(GO_FLAG_NOT_SELECTABLE);
                    break;
                case DATA_AKU_MAI:
                    if (state == DONE)
                        if (GameObject* go = instance->GetGameObject(altarOfTheDeepsGUID))
                        {
                            go->RemoveFlag(GO_FLAG_NOT_SELECTABLE);
                            go->SummonCreature(NPC_MORRIDUNE, SpawnsLocation[10], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5min);
                        }
                    break;
                default:
                    break;
            }

            return true;
        }

        uint32 GetData(uint32 type) const override
        {
            if (type == DATA_FIRE)
                return countFires;
            return 0;
        }

        ObjectGuid GetGuidData(uint32 data) const override
        {
            switch (data)
            {
                case DATA_TWILIGHT_LORD_KELRIS:
                    return twilightLordKelrisGUID;
                case DATA_SHRINE1:
                    return shrine1GUID;
                case DATA_SHRINE2:
                    return shrine2GUID;
                case DATA_SHRINE3:
                    return shrine3GUID;
                case DATA_SHRINE4:
                    return shrine4GUID;
                case DATA_SHRINE_OF_GELIHAST:
                    return shrineOfGelihastGUID;
                case DATA_MAINDOOR:
                    return mainDoorGUID;
            }

            return ObjectGuid::Empty;
        }

    private:
        ObjectGuid twilightLordKelrisGUID;
        ObjectGuid shrine1GUID;
        ObjectGuid shrine2GUID;
        ObjectGuid shrine3GUID;
        ObjectGuid shrine4GUID;
        ObjectGuid shrineOfGelihastGUID;
        ObjectGuid altarOfTheDeepsGUID;
        ObjectGuid mainDoorGUID;
        uint8 countFires;
        uint32 _requiredDeaths;
    };
};

void AddSC_instance_blackfathom_deeps()
{
    new instance_blackfathom_deeps();
}


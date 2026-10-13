#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "uldaman.h"
#include <algorithm>

static constexpr DoorData doorData[] =
{
    { GO_TEMPLE_DOOR_TO_KEEPERS,    DATA_STONE_KEEPERS,  DOOR_TYPE_ROOM    },
    { GO_TEMPLE_DOOR_TO_ARCHAEDAS,  DATA_STONE_KEEPERS,  DOOR_TYPE_PASSAGE },
    { GO_TEMPLE_DOOR_ARCHAEDAS,     DATA_ARCHAEDAS,      DOOR_TYPE_ROOM    },
    { GO_ANCIENT_VAULT_DOOR,        DATA_ARCHAEDAS,      DOOR_TYPE_PASSAGE },
    { 0,                            0,                   DOOR_TYPE_ROOM    }
};

static constexpr ObjectData creatureData[] =
{
    { NPC_ARCHAEDAS,                DATA_ARCHAEDAS                },
    { NPC_IRONAYA,                  DATA_IRONAYA                  },
    { NPC_REVELOSH,                 DATA_REVELOSH                 },
    { NPC_OBSIDIAN_SENTINEL,        DATA_OBSIDIAN_SENTINEL        },
    { NPC_ANCIENT_STONE_KEEPER,     DATA_ANCIENT_STONE_KEEPER     },
    { NPC_GALGANN,                  DATA_GALGANN                  },
    { NPC_GRIMLOK,                  DATA_GRIMLOK                  },
    { 0,                            0                             }
};

static constexpr ObjectData gameObjectData[] =
{
    { GO_IRONAYA_SEAL_DOOR,         DATA_IRONAYA_SEAL_DOOR        },
    { 0,                            0                             }
};

class instance_uldaman : public InstanceMapScript
{
public:
    instance_uldaman() : InstanceMapScript(UldamanScriptName, 70) { }

    struct instance_uldaman_InstanceMapScript : public InstanceScript
    {
        instance_uldaman_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadObjectData(creatureData, gameObjectData);
            LoadDoorData(doorData);
            IronayaIntroState = NOT_STARTED;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            InstanceScript::OnCreatureCreate(creature);
            switch (creature->GetEntry())
            {
                case NPC_STONE_KEEPER:
                {
                    static constexpr std::array<std::string_view, 4> KeeperStringId = {{ "StoneKeeper1", "StoneKeeper2", "StoneKeeper3", "StoneKeeper4" }};
                    auto stringId = std::ranges::find_if(KeeperStringId, [creature](std::string_view stringId) { return creature->HasStringId(stringId); });
                    if (stringId != KeeperStringId.end())
                        StoneKeeperGuid[std::ranges::distance(KeeperStringId.begin(), stringId)] = creature->GetGUID();
                    break;
                }
                case NPC_EARTHEN_GUARDIAN:
                case NPC_EARTHEN_HALLSHAPER:
                case NPC_EARTHEN_CUSTODIAN:
                case NPC_VAULT_WARDER:
                    ArchaedasMinionsGuidSet.insert(creature->GetGUID());
                    break;
                case NPC_IRONAYA: IronayaGuid = creature->GetGUID(); break;
                default: break;
            }
        }

        void OnCreatureRemove(Creature* creature) override
        {
            InstanceScript::OnCreatureRemove(creature);
            switch (creature->GetEntry())
            {
                case NPC_EARTHEN_GUARDIAN:
                case NPC_EARTHEN_HALLSHAPER:
                case NPC_EARTHEN_CUSTODIAN:
                case NPC_VAULT_WARDER:
                    ArchaedasMinionsGuidSet.erase(creature->GetGUID());
                    break;
                case NPC_IRONAYA:
                    if (IronayaGuid == creature->GetGUID()) IronayaGuid.Clear();
                    break;
                default: break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            InstanceScript::OnGameObjectCreate(go);
            switch (go->GetEntry())
            {
                case GO_IRONAYA_SEAL_DOOR:
                    if (GetData(DATA_IRONAYA_INTRO) == DONE) HandleGameObject(ObjectGuid::Empty, true, go);
                    break;
                case GO_KEYSTONE:
                    if (GetData(DATA_IRONAYA_INTRO) == DONE)
                    {
                        HandleGameObject(ObjectGuid::Empty, true, go);
                        go->SetFlag(GO_FLAG_INTERACT_COND);
                    }
                    break;
                default: break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            InstanceScript::OnUnitDeath(unit);
            Creature* creature = unit->ToCreature();
            if (!creature) return;

            switch (creature->GetEntry())
            {
                case NPC_STONE_KEEPER:
                    for (std::size_t i = 0; i < StoneKeeperGuid.size() - 1; ++i)
                    {
                        if (creature->GetGUID() == StoneKeeperGuid[i])
                        {
                            if (Creature* keeper = instance->GetCreature(StoneKeeperGuid[i + 1]))
                            {
                                keeper->AI()->DoAction(ACTION_KEEPER_ACTIVATED);
                                break;
                            }
                        }
                    }
                    if (creature->GetGUID() == StoneKeeperGuid[3]) SetBossState(DATA_STONE_KEEPERS, DONE);
                    break;
                case NPC_IRONAYA: if (creature->GetGUID() == IronayaGuid) SetBossState(DATA_IRONAYA, DONE); break;
                case NPC_REVELOSH: SetBossState(DATA_REVELOSH, DONE); break;
                case NPC_OBSIDIAN_SENTINEL: SetBossState(DATA_OBSIDIAN_SENTINEL, DONE); break;
                case NPC_ANCIENT_STONE_KEEPER: SetBossState(DATA_ANCIENT_STONE_KEEPER, DONE); break;
                case NPC_GALGANN: SetBossState(DATA_GALGANN, DONE); break;
                case NPC_GRIMLOK: SetBossState(DATA_GRIMLOK, DONE); break;
                default: break;
            }
        }

        bool SetBossState(uint32 bossId, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(bossId, state)) return false;
            switch (bossId)
            {
                case DATA_ARCHAEDAS:
                    if (state == FAIL)
                        for (ObjectGuid guid : ArchaedasMinionsGuidSet)
                            if (Creature* minion = instance->GetCreature(guid))
                                if (minion->isDead()) minion->Respawn();
                    break;
                case DATA_STONE_KEEPERS:
                    if (state == FAIL)
                    {
                        for (std::size_t i = 0; i < StoneKeeperGuid.size() - 1; ++i)
                        {
                            if (Creature* keeper = instance->GetCreature(StoneKeeperGuid[i]))
                            {
                                if (!keeper->IsAlive()) { StoneKeeperGuid[i].Clear(); keeper->Respawn(); }
                            }
                        }
                        SetBossState(DATA_STONE_KEEPERS, NOT_STARTED);
                    }
                    break;
                default: break;
            }
            return true;
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_IRONAYA_INTRO)
            {
                IronayaIntroState = data;
                if (data == IN_PROGRESS) instance->SpawnGroupSpawn(SPAWN_GROUP_IRONAYA);
                else if (data == DONE) { HandleGameObject(ObjectGuid::Empty, true, GetGameObject(DATA_IRONAYA_SEAL_DOOR)); SaveToDB(); }
            }
        }

        void ProcessEvent(WorldObject* /*gameObject*/, uint32 eventId) override
        {
            switch (eventId)
            {
                case EVENT_SUB_BOSS_AGGRO: if (Creature* keeper = instance->GetCreature(StoneKeeperGuid[0])) keeper->AI()->DoAction(ACTION_KEEPER_ACTIVATED); break;
                case EVENT_BOSS_AGGRO: if (Creature* archaedas = GetCreature(DATA_ARCHAEDAS)) archaedas->AI()->DoAction(ACTION_ARCHAEDAS_AWAKEN); break;
                default: break;
            }
        }

        void WriteSaveDataMore(std::ostringstream& stream) override { stream << IronayaIntroState; }
        void ReadSaveDataMore(std::istringstream& stream) override { stream >> IronayaIntroState; }
        uint32 GetData(uint32 type) const override { return type == DATA_IRONAYA_INTRO ? IronayaIntroState : 0; }

    protected:
        std::array<ObjectGuid, 4> StoneKeeperGuid;
        GuidSet ArchaedasMinionsGuidSet;
        ObjectGuid IronayaGuid;
        uint32 IronayaIntroState;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override { return new instance_uldaman_InstanceMapScript(map); }
};

void AddSC_instance_uldaman() { new instance_uldaman(); }


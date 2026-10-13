#include "ScriptMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum StoneKeeper { SPELL_STONED = 10255, SPELL_DETECTION = 18950, SPELL_TRAMPLE = 5568, SPELL_SELF_DESTRUCT = 9874 };

struct npc_stone_keeper : public ScriptedAI
{
    npc_stone_keeper(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()) { }
    void JustAppeared() override { me->SetFaction(FACTION_TITAN_NEUTRAL); me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); DoCastSelf(SPELL_STONED); DoCastSelf(SPELL_DETECTION); }
    void JustEngagedWith(Unit* /*who*/) override { _instance->SetBossState(DATA_STONE_KEEPERS, IN_PROGRESS); _scheduler.Schedule(10s, 15s, [this](TaskContext task) { DoCastSelf(SPELL_TRAMPLE); task.Repeat(10s, 15s); }); }
    void DoAction(int32 action) override { if (action == ACTION_KEEPER_ACTIVATED) { me->RemoveAurasDueToSpell(SPELL_STONED); me->SetFaction(FACTION_TITAN_HOSTILE); me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(false); DoZoneInCombat(); } }
    void JustReachedHome() override { me->SetFaction(FACTION_TITAN_NEUTRAL); me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); DoCastSelf(SPELL_STONED); DoCastSelf(SPELL_DETECTION); _scheduler.CancelAll(); }
    void EnterEvadeMode(EvadeReason why) override { ScriptedAI::EnterEvadeMode(why); _instance->SetBossState(DATA_STONE_KEEPERS, FAIL); }
    void JustDied(Unit* /*killer*/) override { DoCastSelf(SPELL_SELF_DESTRUCT, true); }
    void UpdateAI(uint32 diff) override { if (!UpdateVictim()) return; _scheduler.Update(diff, [this] { DoMeleeAttackIfReady(); }); }
private: InstanceScript* _instance; TaskScheduler _scheduler;
};

struct go_keystone_chamber : public GameObjectAI
{
    go_keystone_chamber(GameObject* go) : GameObjectAI(go), _instance(go->GetInstanceScript()) { }
    bool OnGossipHello(Player* /*player*/) override { _instance->SetData(DATA_IRONAYA_INTRO, IN_PROGRESS); return false; }
private: InstanceScript* _instance;
};

void AddSC_uldaman() { RegisterUldamanCreatureAI(npc_stone_keeper); RegisterUldamanGameObjectAI(go_keystone_chamber); }


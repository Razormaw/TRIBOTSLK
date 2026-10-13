#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum ArchaedasTexts { SAY_AGGRO = 0, SAY_SUMMON_1 = 1, SAY_SUMMON_2 = 2, SAY_SLAY = 3, EMOTE_AWAKEN = 4 };
enum ArchaedasSpells { SPELL_FREEZE_ANIM = 16245, SPELL_ARCHAEDAS_AWAKEN = 10347, SPELL_GROUND_TREMOR = 6524, SPELL_AWAKEN_DWARF = 10259, SPELL_AWAKEN_GUARDIANS = 10252, SPELL_AWAKEN_WALKERS = 10258, SPELL_DESTROY_EARTHEN_GUARDS = 10604, SPELL_STONED = 10255, SPELL_STONE_DWARF_AWAKEN_VISUAL = 10254, SPELL_EARTHEN_GUARDS_DESTROYED = 10666, SPELL_RECONSTRUCT = 10260, SPELL_WHIRLWIND = 17207, SPELL_TRAMPLE = 5568 };
enum ArchaedasEvents { EVENT_INTRO_1 = 1, EVENT_INTRO_2, EVENT_INTRO_3, EVENT_INTRO_4, EVENT_GROUND_TREMOR, EVENT_AWAKEN_DWARF, EVENT_AWAKEN_GUARDIANS, EVENT_AWAKEN_WALKERS };
enum ArchaedasPhases : uint8 { PHASE_NONE = 0, PHASE_HEALTH_66, PHASE_HEALTH_33 };

struct boss_archaedas : public BossAI
{
    boss_archaedas(Creature* creature) : BossAI(creature, DATA_ARCHAEDAS), _phase(PHASE_NONE), _introStarted(false) { }
    void JustAppeared() override { me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); DoCastSelf(SPELL_FREEZE_ANIM); }
    void Reset() override { _Reset(); _phase = PHASE_NONE; _introStarted = false; }
    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(EVENT_GROUND_TREMOR, 20s, 30s);
        events.ScheduleEvent(EVENT_AWAKEN_DWARF, 10s);
        std::vector<Creature*> vaultWarders;
        GetCreatureListWithOptionsInGrid(vaultWarders, me, 100.0f, { .StringId = "VaultWarderOutside" });
        for (Creature* vaultWarder : vaultWarders) vaultWarder->AI()->DoAction(ACTION_ACTIVATE_VAULT_WARDERS);
    }
    void DamageTaken(Unit* /*killer*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_phase < PHASE_HEALTH_66 && me->HealthBelowPctDamaged(66, damage)) { _phase++; events.ScheduleEvent(EVENT_AWAKEN_GUARDIANS, 0s); }
        if (_phase < PHASE_HEALTH_33 && me->HealthBelowPctDamaged(33, damage)) { _phase++; events.ScheduleEvent(EVENT_AWAKEN_WALKERS, 0s); }
    }
    void DoAction(int32 action) override { if (action == ACTION_ARCHAEDAS_AWAKEN && me->IsAlive() && !_introStarted) events.ScheduleEvent(EVENT_INTRO_1, 0s); }
    void JustReachedHome() override { _JustReachedHome(); me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); DoCastSelf(SPELL_FREEZE_ANIM); }
    void EnterEvadeMode(EvadeReason why) override { instance->SetBossState(DATA_ARCHAEDAS, FAIL); BossAI::EnterEvadeMode(why); }
    void KilledUnit(Unit* /*victim*/) override { Talk(SAY_SLAY); }
    void JustDied(Unit* /*killer*/) override { _JustDied(); DoCastSelf(SPELL_DESTROY_EARTHEN_GUARDS, true); }

    void UpdateOutOfCombatEvents(uint32 diff)
    {
        events.Update(diff);
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_INTRO_1: _introStarted = true; DoCastSelf(SPELL_ARCHAEDAS_AWAKEN); events.ScheduleEvent(EVENT_INTRO_2, 1500ms); break;
                case EVENT_INTRO_2: Talk(EMOTE_AWAKEN); events.ScheduleEvent(EVENT_INTRO_3, 3500ms); break;
                case EVENT_INTRO_3: Talk(SAY_AGGRO); me->RemoveAurasDueToSpell(SPELL_FREEZE_ANIM); events.ScheduleEvent(EVENT_INTRO_4, 3s); break;
                case EVENT_INTRO_4: me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(false); DoZoneInCombat(); break;
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) { UpdateOutOfCombatEvents(diff); return; }
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_GROUND_TREMOR: DoCastSelf(SPELL_GROUND_TREMOR); events.Repeat(20s, 30s); break;
                case EVENT_AWAKEN_DWARF: DoCastSelf(SPELL_AWAKEN_DWARF); events.Repeat(10s); break;
                case EVENT_AWAKEN_GUARDIANS: Talk(SAY_SUMMON_1); DoCastSelf(SPELL_AWAKEN_GUARDIANS); break;
                case EVENT_AWAKEN_WALKERS: Talk(SAY_SUMMON_2); DoCastSelf(SPELL_AWAKEN_WALKERS); break;
            }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase; bool _introStarted;
};

struct npc_earthen_hallshaper_custodian : public ScriptedAI
{
    npc_earthen_hallshaper_custodian(Creature* creature) : ScriptedAI(creature) { }
    void Reset() override { me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); _scheduler.CancelAll(); }
    void JustEngagedWith(Unit* /*who*/) override { if (me->GetEntry() == NPC_EARTHEN_HALLSHAPER) _scheduler.Schedule(15s, 25s, [this](TaskContext task) { DoCastSelf(SPELL_RECONSTRUCT); task.Repeat(30s, 45s); }); }
    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_DESTROY_EARTHEN_GUARDS) { DoCastSelf(SPELL_EARTHEN_GUARDS_DESTROYED, true); me->DespawnOrUnsummon(); }
        else if (spellInfo->Id == SPELL_AWAKEN_DWARF) _scheduler.Schedule(3s, [this](TaskContext task) { if (task.GetRepeatCounter() == 0) { DoCastSelf(SPELL_STONE_DWARF_AWAKEN_VISUAL); task.Repeat(4s); } else { me->RemoveAurasDueToSpell(SPELL_STONED); me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(false); DoZoneInCombat(); } });
    }
    void UpdateAI(uint32 diff) override { _scheduler.Update(diff); if (UpdateVictim()) DoMeleeAttackIfReady(); }
private: TaskScheduler _scheduler;
};

struct npc_earthen_guardian : public ScriptedAI
{
    npc_earthen_guardian(Creature* creature) : ScriptedAI(creature) { }
    void Reset() override { me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); _scheduler.CancelAll(); }
    void JustEngagedWith(Unit* /*who*/) override { _scheduler.Schedule(15s, 25s, [this](TaskContext task) { DoCastSelf(SPELL_WHIRLWIND); task.Repeat(15s, 25s); }); }
    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_DESTROY_EARTHEN_GUARDS) { DoCastSelf(SPELL_EARTHEN_GUARDS_DESTROYED, true); me->DespawnOrUnsummon(); }
        else if (spellInfo->Id == SPELL_AWAKEN_GUARDIANS) _scheduler.Schedule(7s, [this](TaskContext task) { if (task.GetRepeatCounter() == 0) { DoCastSelf(SPELL_STONE_DWARF_AWAKEN_VISUAL); task.Repeat(4s); } else { me->RemoveAurasDueToSpell(SPELL_STONED); me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(false); DoZoneInCombat(); } });
    }
    void UpdateAI(uint32 diff) override { _scheduler.Update(diff); if (UpdateVictim()) DoMeleeAttackIfReady(); }
private: TaskScheduler _scheduler;
};

struct npc_vault_warder : public ScriptedAI
{
    npc_vault_warder(Creature* creature) : ScriptedAI(creature) { }
    void Reset() override { _scheduler.CancelAll(); if (me->HasStringId("VaultWarderOutside")) return; me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(true); }
    void JustEngagedWith(Unit* /*who*/) override { _scheduler.Schedule(10s, 15s, [this](TaskContext task) { DoCastSelf(SPELL_TRAMPLE); task.Repeat(10s, 15s); }); }
    void DoAction(int32 action) override { if (action == ACTION_ACTIVATE_VAULT_WARDERS && me->IsAlive()) _scheduler.Schedule(5s, [this](TaskContext task) { if (task.GetRepeatCounter() == 0) { DoCastSelf(SPELL_STONE_DWARF_AWAKEN_VISUAL); task.Repeat(5s); } else { me->SetFaction(FACTION_TITAN_HOSTILE); me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(false); } }); }
    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_DESTROY_EARTHEN_GUARDS) { DoCastSelf(SPELL_EARTHEN_GUARDS_DESTROYED, true); me->DespawnOrUnsummon(); }
        else if (spellInfo->Id == SPELL_AWAKEN_WALKERS && !me->HasStringId("VaultWarderOutside")) _scheduler.Schedule(2s, [this](TaskContext task) { if (task.GetRepeatCounter() == 0) { DoCastSelf(SPELL_STONE_DWARF_AWAKEN_VISUAL); task.Repeat(4s); } else { me->SetFaction(FACTION_TITAN_NEUTRAL); me->RemoveAurasDueToSpell(SPELL_STONED); me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE); me->SetImmuneToAll(false); DoZoneInCombat(); } });
    }
    void UpdateAI(uint32 diff) override { _scheduler.Update(diff); if (UpdateVictim()) DoMeleeAttackIfReady(); }
private: TaskScheduler _scheduler;
};

void AddSC_boss_archaedas() { RegisterUldamanCreatureAI(boss_archaedas); RegisterUldamanCreatureAI(npc_earthen_hallshaper_custodian); RegisterUldamanCreatureAI(npc_earthen_guardian); RegisterUldamanCreatureAI(npc_vault_warder); }


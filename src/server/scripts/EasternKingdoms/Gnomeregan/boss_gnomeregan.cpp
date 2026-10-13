/*
 * TRIBOTSLK - Gnomeregan: Jefes Principales
 * Fases dinámicas, mecánicas activas y audio verificado.
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "gnomeregan.h"

using namespace std::chrono_literals;

// ================= 1) VICIOUS FALLOUT (Slime Tóxico) =================
struct boss_vicious_fallout : public BossAI
{
    boss_vicious_fallout(Creature* c) : BossAI(c, DATA_VICIOUS_FALLOUT), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(0); events.ScheduleEvent(1, 5s, 8s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(30)) {
            _phase = 2; Talk(1); DoCastSelf(8269); // Enrage
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(2); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == 1) { DoCastAOE(7991); events.ScheduleEvent(1, 8s, 12s); } // Toxic Cloud
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 2) ELECTROCUTIONER 6000 (Mecánico) =================
struct boss_electrocutioner_6000 : public BossAI
{
    boss_electrocutioner_6000(Creature* c) : BossAI(c, DATA_ELECTROCUTIONER), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(0); events.ScheduleEvent(1, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(30)) {
            _phase = 2; Talk(1); DoCastSelf(8269); // Enrage
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(2); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == 1) { DoCastVictim(10605); events.ScheduleEvent(1, 8s, 12s); } // Chain Lightning
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 3) CROWD PUMMELER 9-60 (Mecánico Pesado) =================
struct boss_crowd_pummeler_9_60 : public BossAI
{
    boss_crowd_pummeler_9_60(Creature* c) : BossAI(c, DATA_CROWD_PUMMELER), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(0); events.ScheduleEvent(1, 5s, 8s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(30)) {
            _phase = 2; Talk(1); DoCastSelf(8269); // Enrage
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(2); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == 1) { DoCastVictim(15496); events.ScheduleEvent(1, 6s, 10s); } // Cleave
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 4) MEKGINEER THERMAPLUGG (Jefe Final) =================
struct boss_mekgineer_thermaplugg : public BossAI
{
    boss_mekgineer_thermaplugg(Creature* c) : BossAI(c, DATA_THERMAPLUGG), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(0); events.ScheduleEvent(1, 5s, 8s); events.ScheduleEvent(2, 10s, 15s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) {
            _phase = 2; Talk(1); 
            me->SummonCreature(NPC_MECHANICAL_YETI, me->GetPositionX()+3.0f, me->GetPositionY()+3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            me->SummonCreature(NPC_MECHANICAL_YETI, me->GetPositionX()-3.0f, me->GetPositionY()-3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
        }
        if (_phase == 2 && me->HealthBelowPct(20)) {
            _phase = 3; Talk(1); DoCastSelf(8269); // Enrage
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(2); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == 1) { DoCastVictim(11831); events.ScheduleEvent(1, 6s, 9s); } // Frostbolt
            if (eventId == 2) { DoCastAOE(15531); events.ScheduleEvent(2, 15s, 20s); } // Frost Nova
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

void AddSC_bosses_gnomeregan()
{
    RegisterGnomereganCreatureAI(boss_vicious_fallout);
    RegisterGnomereganCreatureAI(boss_electrocutioner_6000);
    RegisterGnomereganCreatureAI(boss_crowd_pummeler_9_60);
    RegisterGnomereganCreatureAI(boss_mekgineer_thermaplugg);
}


/*
 * TRIBOTSLK - Horado Rajacieno (Razorfen Kraul, map 47)
 * Jefes con fases, invocación de adds y mecánicas activas.
 * VERSIÓN CORREGIDA - Sin errores de criaturas/hechizos inexistentes
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "razorfen_kraul.h"

using namespace std::chrono_literals;

// ================= 1) AGATHELOS THE RAGING (Jabalí Furioso) =================
struct boss_agathelos : public BossAI
{
    boss_agathelos(Creature* c) : BossAI(c, BOSS_AGATHELOS), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 5s, 8s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) {
            _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE);
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CLEAVE) { DoCastVictim(SPELL_CLEAVE); events.ScheduleEvent(SPELL_CLEAVE, _phase == 1 ? 8s : 4s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 2) ROOGUG (Ogro/Bruto) =================
struct boss_roogug : public BossAI
{
    boss_roogug(Creature* c) : BossAI(c, BOSS_ROOGUG), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) {
            _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE);
            // Usamos NPC_WARDER (4422) que SÍ existe en lugar de NPC_GEOMANCER (4426)
            me->SummonCreature(NPC_WARDER, me->GetPositionX()+2.0f, me->GetPositionY()+2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            me->SummonCreature(NPC_WARDER, me->GetPositionX()-2.0f, me->GetPositionY()-2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CLEAVE) { DoCastVictim(SPELL_CLEAVE); events.ScheduleEvent(SPELL_CLEAVE, _phase == 1 ? 8s : 4s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 3) OVERLORD RAMTUSK (Quilboar) =================
struct boss_ramtusk : public BossAI
{
    boss_ramtusk(Creature* c) : BossAI(c, BOSS_RAMTUSK), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) {
            _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE);
            me->SummonCreature(NPC_WARDER, me->GetPositionX()+3.0f, me->GetPositionY()+3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            me->SummonCreature(NPC_WARDER, me->GetPositionX()-3.0f, me->GetPositionY()-3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CLEAVE) { DoCastVictim(SPELL_CLEAVE); events.ScheduleEvent(SPELL_CLEAVE, _phase == 1 ? 8s : 4s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 4) GROYAT THE BLIND HUNTER (Quilboar Cazador) =================
struct boss_groyat : public BossAI
{
    boss_groyat(Creature* c) : BossAI(c, BOSS_GROYAT), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_FIREBALL, 5s, 8s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) {
            _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE); // Reemplazamos SPELL_FRENZY por SPELL_ENRAGE
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_FIREBALL) { DoCastVictim(SPELL_FIREBALL); events.ScheduleEvent(SPELL_FIREBALL, _phase == 1 ? 7s : 3s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 5) CHARLGA RAZORFLANK (Jefe Final - Chamana Quilboar) =================
struct boss_charlga : public BossAI
{
    boss_charlga(Creature* c) : BossAI(c, BOSS_CHARLGA), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_FIREBALL, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(60)) {
            _phase = 2; Talk(SAY_SPECIAL);
            me->SummonCreature(NPC_SHAMAN, me->GetPositionX()+3.0f, me->GetPositionY()+3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            me->SummonCreature(NPC_SHAMAN, me->GetPositionX()-3.0f, me->GetPositionY()-3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
        }
        if (_phase == 2 && me->HealthBelowPct(30)) {
            _phase = 3; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE);
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_FIREBALL) { DoCastVictim(SPELL_FIREBALL); events.ScheduleEvent(SPELL_FIREBALL, _phase == 1 ? 8s : 4s); }
            if (eventId == SPELL_HEAL && _phase == 2) { DoCastSelf(SPELL_HEAL); events.ScheduleEvent(SPELL_HEAL, 15s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

void AddSC_boss_razorfen_kraul()
{
    RegisterRazorfenKraulCreatureAI(boss_agathelos);
    RegisterRazorfenKraulCreatureAI(boss_roogug);
    RegisterRazorfenKraulCreatureAI(boss_ramtusk);
    RegisterRazorfenKraulCreatureAI(boss_groyat);
    RegisterRazorfenKraulCreatureAI(boss_charlga);
}


/*
 * TRIBOTSLK - Castillo del Colmillo Oscuro (Jefes Intermedios) - CORREGIDO
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "shadowfang_keep.h"

using namespace std::chrono_literals;

// ================= 1) RETHILGORE (Murciélago Gigante) =================
struct boss_rethilgore : public BossAI
{
    boss_rethilgore(Creature* c) : BossAI(c, TYPE_RETHILGORE), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 5s, 8s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); DoCastSelf(SPELL_ENRAGE); events.ScheduleEvent(SPELL_THUNDERCLAP, 2s); }
    }
    void JustDied(Unit* /*killer*/) override { 
        Talk(SAY_BOSS_DEATH); 
        // FIX: Actualiza el script de instancia antiguo para que Ashcrombe sepa que murió
        instance->SetData(TYPE_RETHILGORE, DONE); 
    }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CLEAVE) { DoCastVictim(SPELL_CLEAVE); events.ScheduleEvent(SPELL_CLEAVE, _phase == 1 ? 8s : 4s); }
            else if (eventId == SPELL_THUNDERCLAP && _phase == 2) { DoCastAOE(SPELL_THUNDERCLAP); events.ScheduleEvent(SPELL_THUNDERCLAP, 10s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 2) RAZORCLAW THE BUTCHER (Worgen) =================
struct boss_razorclaw : public BossAI
{
    boss_razorclaw(Creature* c) : BossAI(c, 0), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); DoCastSelf(SPELL_FRENZY); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_BOSS_DEATH); _JustDied(); }
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

// ================= 3) ODO THE BLINDWATCHER (No-muerto) =================
struct boss_odo : public BossAI
{
    boss_odo(Creature* c) : BossAI(c, 0), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_CURSE_OF_BLOOD, 8s, 12s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(60)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); me->SummonCreature(NPC_SILVERLAINE, me->GetPositionX()+2.0f, me->GetPositionY()+2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_BOSS_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CURSE_OF_BLOOD) { DoCastVictim(SPELL_CURSE_OF_BLOOD); events.ScheduleEvent(SPELL_CURSE_OF_BLOOD, 10s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 4) BARON SILVERLAINE (No-muerto) =================
struct boss_silverlaine : public BossAI
{
    boss_silverlaine(Creature* c) : BossAI(c, 0), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_CURSE_OF_BLOOD, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); me->SummonCreature(NPC_SILVERLAINE, me->GetPositionX()+3.0f, me->GetPositionY()+3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); me->SummonCreature(NPC_SILVERLAINE, me->GetPositionX()-3.0f, me->GetPositionY()-3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_BOSS_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CURSE_OF_BLOOD) { DoCastVictim(SPELL_CURSE_OF_BLOOD); events.ScheduleEvent(SPELL_CURSE_OF_BLOOD, 8s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 5) COMMANDER SPRINGVALE (Humano) =================
struct boss_springvale : public BossAI
{
    boss_springvale(Creature* c) : BossAI(c, 0), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_HAMMER_JUSTICE, 8s, 12s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); DoCastSelf(SPELL_HOLY_LIGHT); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_BOSS_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_HAMMER_JUSTICE) { DoCastVictim(SPELL_HAMMER_JUSTICE); events.ScheduleEvent(SPELL_HAMMER_JUSTICE, 10s); }
            if (eventId == SPELL_HOLY_LIGHT && _phase == 2) { DoCastSelf(SPELL_HOLY_LIGHT); events.ScheduleEvent(SPELL_HOLY_LIGHT, 15s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 6) FENRUS THE DEVOURER (Lobo Gigante) =================
struct boss_fenrus : public BossAI
{
    boss_fenrus(Creature* c) : BossAI(c, TYPE_FENRUS), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 5s, 8s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); DoCastSelf(SPELL_FRENZY); }
    }
    void JustDied(Unit* /*killer*/) override { 
        Talk(SAY_BOSS_DEATH); 
        // FIX: Actualiza el script de instancia antiguo
        instance->SetData(TYPE_FENRUS, DONE); 
    }
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

// ================= 7) WOLF MASTER NANDOS (Humano) =================
struct boss_nandos : public BossAI
{
    boss_nandos(Creature* c) : BossAI(c, TYPE_NANDOS), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_BOSS_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(60)) { _phase = 2; Talk(SAY_BOSS_SPECIAL); DoCastSelf(SPELL_BLOODLUST); me->SummonCreature(NPC_WOLF, me->GetPositionX()+2.0f, me->GetPositionY()+2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); me->SummonCreature(NPC_WOLF, me->GetPositionX()-2.0f, me->GetPositionY()-2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); }
    }
    void JustDied(Unit* /*killer*/) override { 
        Talk(SAY_BOSS_DEATH); 
        // FIX: Actualiza el script de instancia antiguo
        instance->SetData(TYPE_NANDOS, DONE); 
    }
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

void AddSC_bosses_shadowfang_keep()
{
    RegisterShadowfangKeepCreatureAI(boss_rethilgore);
    RegisterShadowfangKeepCreatureAI(boss_razorclaw);
    RegisterShadowfangKeepCreatureAI(boss_odo);
    RegisterShadowfangKeepCreatureAI(boss_silverlaine);
    RegisterShadowfangKeepCreatureAI(boss_springvale);
    RegisterShadowfangKeepCreatureAI(boss_fenrus);
    RegisterShadowfangKeepCreatureAI(boss_nandos);
}


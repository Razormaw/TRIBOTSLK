/*
 * TRIBOTSLK - Minas de la Muerte (Deadmines, map 36)
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

using namespace std::chrono_literals;

enum ClassicTexts { SAY_AGGRO = 0, SAY_DEATH = 1, SAY_SPECIAL = 2 };

// ================= 1) RHAHK'ZOR =================
struct boss_rhahkzor : public BossAI
{
    boss_rhahkzor(Creature* c) : BossAI(c, BOSS_RHAHKZOR), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_THUNDER_CLAP, 8s, 12s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) { _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE); events.ScheduleEvent(SPELL_WHIRLWIND, 2s); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_THUNDER_CLAP) { DoCastAOE(SPELL_THUNDER_CLAP); events.ScheduleEvent(SPELL_THUNDER_CLAP, _phase == 1 ? 10s : 6s); }
            else if (eventId == SPELL_WHIRLWIND && _phase == 2) { DoCastSelf(SPELL_WHIRLWIND); events.ScheduleEvent(SPELL_WHIRLWIND, 8s, 12s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 2) MINER JOHNSON =================
struct boss_miner_johnson : public BossAI
{
    boss_miner_johnson(Creature* c) : BossAI(c, 0) { }
    void Reset() override { _Reset(); }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_CHARGE, 10s, 15s); events.ScheduleEvent(SPELL_REND, 5s, 8s); events.ScheduleEvent(SPELL_DYNAMITE_TOSS, 12s, 18s); }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CHARGE) { if (me->GetDistance(me->GetVictim()) > 10.0f) DoCastVictim(SPELL_CHARGE); events.ScheduleEvent(SPELL_CHARGE, 12s, 18s); }
            else if (eventId == SPELL_REND) { DoCastVictim(SPELL_REND); events.ScheduleEvent(SPELL_REND, 8s, 12s); }
            else if (eventId == SPELL_DYNAMITE_TOSS) { DoCastAOE(SPELL_DYNAMITE_TOSS); events.ScheduleEvent(SPELL_DYNAMITE_TOSS, 15s, 20s); }
        }
        DoMeleeAttackIfReady();
    }
};

// ================= 3) SNEED'S SHREDDER =================
struct boss_sneed_shredder : public BossAI
{
    boss_sneed_shredder(Creature* c) : BossAI(c, BOSS_SNEED), _said(false) { }
    void Reset() override { _Reset(); _said = false; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_REND, 6s, 9s); events.ScheduleEvent(SPELL_THUNDER_CLAP, 12s, 16s); }
    void JustDied(Unit* /*killer*/) override {
        Talk(SAY_DEATH);
        me->SummonCreature(NPC_SNEED, me->GetPositionX() + 2.0f, me->GetPositionY() + 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); // <-- 0ms
        _JustDied();
    }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_REND) { DoCastVictim(SPELL_REND); events.ScheduleEvent(SPELL_REND, 8s, 12s); }
            else if (eventId == SPELL_THUNDER_CLAP) { if (!_said) { _said = true; Talk(SAY_SPECIAL); } DoCastAOE(SPELL_THUNDER_CLAP); events.ScheduleEvent(SPELL_THUNDER_CLAP, 15s, 20s); }
        }
        DoMeleeAttackIfReady();
    }
private: bool _said;
};

// ================= 4) SNEED =================
struct boss_sneed : public BossAI
{
    boss_sneed(Creature* c) : BossAI(c, BOSS_SNEED), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); DoCastSelf(SPELL_BLADE_FLURRY); events.ScheduleEvent(SPELL_SINISTER_STRIKE, 4s, 6s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) { _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE); events.ScheduleEvent(SPELL_GOUGE, 5s); events.ScheduleEvent(SPELL_KIDNEY_SHOT, 10s); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_SINISTER_STRIKE) { DoCastVictim(SPELL_SINISTER_STRIKE); events.ScheduleEvent(SPELL_SINISTER_STRIKE, 4s, 6s); }
            else if (eventId == SPELL_GOUGE && _phase == 2) { DoCastVictim(SPELL_GOUGE); events.ScheduleEvent(SPELL_GOUGE, 10s, 15s); }
            else if (eventId == SPELL_KIDNEY_SHOT && _phase == 2) { DoCastVictim(SPELL_KIDNEY_SHOT); events.ScheduleEvent(SPELL_KIDNEY_SHOT, 15s, 20s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 5) GILNID =================
struct boss_gilnid : public BossAI
{
    boss_gilnid(Creature* c) : BossAI(c, BOSS_GILNID), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_FIREBALL, 5s, 8s); events.ScheduleEvent(SPELL_MOLTEN_SPLASH, 12s, 16s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(50)) {
            _phase = 2; Talk(SAY_SPECIAL);
            me->SummonCreature(NPC_SMOLDERING_KOBOLD, me->GetPositionX() + 3.0f, me->GetPositionY() + 3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); // <-- 0ms
            me->SummonCreature(NPC_SMOLDERING_KOBOLD, me->GetPositionX() - 3.0f, me->GetPositionY() - 3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms); // <-- 0ms
        }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_FIREBALL) { DoCastVictim(SPELL_FIREBALL); events.ScheduleEvent(SPELL_FIREBALL, 6s, 9s); }
            else if (eventId == SPELL_MOLTEN_SPLASH) { DoCastAOE(SPELL_MOLTEN_SPLASH); events.ScheduleEvent(SPELL_MOLTEN_SPLASH, _phase == 1 ? 15s : 8s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 6) CAPITÁN GREENSKIN =================
struct boss_captain_greenskin : public BossAI
{
    boss_captain_greenskin(Creature* c) : BossAI(c, BOSS_GREENSKIN), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s); events.ScheduleEvent(SPELL_HAMSTRING, 10s, 14s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) { _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_ENRAGE); events.ScheduleEvent(SPELL_NET, 5s); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CLEAVE) { DoCastVictim(SPELL_CLEAVE); events.ScheduleEvent(SPELL_CLEAVE, _phase == 1 ? 8s : 4s); }
            else if (eventId == SPELL_HAMSTRING) { DoCastVictim(SPELL_HAMSTRING); events.ScheduleEvent(SPELL_HAMSTRING, 12s, 16s); }
            else if (eventId == SPELL_NET && _phase == 2) { DoCastVictim(SPELL_NET); events.ScheduleEvent(SPELL_NET, 10s, 15s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

// ================= 7) COOKIE =================
struct boss_cookie : public BossAI
{
    boss_cookie(Creature* c) : BossAI(c, BOSS_COOKIE), _phase(1) { }
    void Reset() override { _Reset(); _phase = 1; }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); Talk(SAY_AGGRO); events.ScheduleEvent(SPELL_FIREBALL, 6s, 9s); events.ScheduleEvent(SPELL_BOILING_STEW, 12s, 16s); }
    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 1 && me->HealthBelowPct(40)) { _phase = 2; Talk(SAY_SPECIAL); DoCastSelf(SPELL_GREATER_HEAL); events.ScheduleEvent(SPELL_BOILING_STEW, 5s); }
    }
    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }
    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_FIREBALL) { DoCastVictim(SPELL_FIREBALL); events.ScheduleEvent(SPELL_FIREBALL, 8s, 12s); }
            else if (eventId == SPELL_BOILING_STEW) { DoCastAOE(SPELL_BOILING_STEW); events.ScheduleEvent(SPELL_BOILING_STEW, _phase == 1 ? 15s : 6s); }
        }
        DoMeleeAttackIfReady();
    }
private: uint8 _phase;
};

void AddSC_deadmines_classic_bosses()
{
    RegisterDeadminesCreatureAI(boss_rhahkzor);
    RegisterDeadminesCreatureAI(boss_miner_johnson);
    RegisterDeadminesCreatureAI(boss_sneed_shredder);
    RegisterDeadminesCreatureAI(boss_sneed);
    RegisterDeadminesCreatureAI(boss_gilnid);
    RegisterDeadminesCreatureAI(boss_captain_greenskin);
    RegisterDeadminesCreatureAI(boss_cookie);
}


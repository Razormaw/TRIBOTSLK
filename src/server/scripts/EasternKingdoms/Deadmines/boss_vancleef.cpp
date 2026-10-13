/*
 * TRIBOTSLK - Minas de la Muerte: Edwin VanCleef
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

using namespace std::chrono_literals;

enum VanCleefTexts {
    SAY_AGGRO  = 0,
    SAY_KILL   = 1,
    SAY_SUMMON = 2,
    SAY_ONE    = 3,
    SAY_TWO    = 4,
    SAY_THREE  = 5,
    SAY_DEATH  = 6
};

Position const BlackguardPositions[] = {
    { -78.2791f, -824.784f, 40.0007f, 2.93215f },
    { -77.8071f, -815.097f, 40.0188f, 3.26377f }
};

struct boss_vancleef : public BossAI
{
    boss_vancleef(Creature* creature) : BossAI(creature, BOSS_VANCLEEF), _guardsCalled(false), _health66(false), _health33(false), _health25(false) { }

    void Reset() override {
        _Reset(); _guardsCalled = false; _health66 = false; _health33 = false; _health25 = false;
        DoCastSelf(SPELL_DUAL_WIELD, true);
        DoCastSelf(SPELL_THRASH, true);
    }

    void JustEngagedWith(Unit* victim) override {
        BossAI::JustEngagedWith(victim);
        summons.DoZoneInCombat();
        Talk(SAY_AGGRO);
        events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s);
    }

    void KilledUnit(Unit* victim) override {
        if (victim->GetTypeId() == TYPEID_PLAYER) Talk(SAY_KILL);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override {
        summons.DespawnAll();
        _DespawnAtEvade();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override {
        if (!_guardsCalled && me->HealthBelowPct(50)) {
            _guardsCalled = true;
            Talk(SAY_SUMMON);
            DoCastSelf(SPELL_VANCLEEFS_ALLIES);
            for (Position pos : BlackguardPositions)
                me->SummonCreature(NPC_BLACKGUARD, pos, TEMPSUMMON_DEAD_DESPAWN, 0ms); // <-- CORREGIDO con 0ms
        }
        if (me->HealthBelowPct(66) && !_health66) { _health66 = true; Talk(SAY_ONE); }
        if (me->HealthBelowPct(33) && !_health33) { _health33 = true; Talk(SAY_TWO); }
        if (me->HealthBelowPct(25) && !_health25) { _health25 = true; Talk(SAY_THREE); }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent()) {
            if (eventId == SPELL_CLEAVE) {
                DoCastVictim(SPELL_CLEAVE);
                events.ScheduleEvent(SPELL_CLEAVE, 8s, 12s);
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _guardsCalled, _health66, _health33, _health25;
};

void AddSC_boss_vancleef()
{
    RegisterDeadminesCreatureAI(boss_vancleef);
}


/*
 * TRIBOTSLK - RazorfenDowns: Glutton
 * Script mejorado con diálogos inmersivos.
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "razorfen_downs.h"

enum GluttonYells
{
    SAY_AGGRO      = 0,
    SAY_SLAY       = 1,
    SAY_HEALTH_50  = 2,
    SAY_HEALTH_15  = 3,
    SAY_DEATH      = 4
};

enum GluttonSpells
{
    SPELL_THRASH_15     = 8876,
    SPELL_DISEASE_CLOUD = 12627,
    SPELL_THRASH_35     = 12787,
    SPELL_FRENZY        = 12795
};

enum GluttonEvents
{
    EVENT_HEALTH_50 = 1,
    EVENT_HEALTH_15
};

enum GluttonPhases : uint8
{
    PHASE_NONE      = 0,
    PHASE_HEALTH_50,
    PHASE_HEALTH_15
};

struct boss_glutton : public BossAI
{
    boss_glutton(Creature* creature) : BossAI(creature, DATA_GLUTTON), _phase(PHASE_NONE) { }

    void Reset() override
    {
        _Reset();
        _phase = PHASE_NONE;
        DoCastSelf(SPELL_THRASH_15);
        DoCastSelf(SPELL_DISEASE_CLOUD);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_phase < PHASE_HEALTH_50 && me->HealthBelowPctDamaged(50, damage))
        {
            _phase++;
            events.ScheduleEvent(EVENT_HEALTH_50, 0s);
        }

        if (_phase < PHASE_HEALTH_15 && me->HealthBelowPctDamaged(15, damage))
        {
            _phase++;
            events.ScheduleEvent(EVENT_HEALTH_15, 0s);
        }
    }

    void KilledUnit(Unit* /*victim*/) override
    {
        Talk(SAY_SLAY);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_DEATH);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_HEALTH_50:
                    Talk(SAY_HEALTH_50);
                    me->RemoveAurasDueToSpell(SPELL_THRASH_15);
                    DoCastSelf(SPELL_THRASH_35);
                    break;
                case EVENT_HEALTH_15:
                    Talk(SAY_HEALTH_15);
                    DoCastSelf(SPELL_FRENZY);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_glutton()
{
    RegisterRazorfenDownsCreatureAI(boss_glutton);
}


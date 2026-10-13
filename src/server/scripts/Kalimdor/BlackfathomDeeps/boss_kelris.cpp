/*
 * TRIBOTSLK - Blackfathom Deeps: Twilight Lord Kelris (Mejorado)
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackfathom_deeps.h"

enum KelrisSpells
{
    SPELL_MIND_BLAST             = 15587,
    SPELL_SLEEP                  = 8399,
    SPELL_SHADOW_WORD_PAIN       = 589,
    SPELL_BLACKFATHOM_CHANNELING = 8734,
    SPELL_ENRAGE                 = 8599
};

enum KelrisTexts
{
    SAY_AGGRO    = 0,
    SAY_SLEEP    = 1,
    SAY_DEATH    = 2
};

enum KelrisEvents
{
    EVENT_MIND_BLAST = 1,
    EVENT_SLEEP,
    EVENT_SHADOW_WORD_PAIN
};

struct boss_kelris : public BossAI
{
    boss_kelris(Creature* creature) : BossAI(creature, DATA_KELRIS), _isEnraged(false) { }

    void Reset() override
    {
        _Reset();
        _isEnraged = false;
        DoCastSelf(SPELL_BLACKFATHOM_CHANNELING);
    }

    void JustReachedHome() override
    {
        _JustReachedHome();
        DoCastSelf(SPELL_BLACKFATHOM_CHANNELING);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        me->RemoveAurasDueToSpell(SPELL_BLACKFATHOM_CHANNELING);
        
        events.ScheduleEvent(EVENT_SHADOW_WORD_PAIN, 2s);
        events.ScheduleEvent(EVENT_MIND_BLAST, 4s, 6s);
        events.ScheduleEvent(EVENT_SLEEP, 10s, 14s);
    }

    void DamageTaken(Unit*, uint32& damage, DamageEffectType, SpellInfo const*) override
    {
        if (!_isEnraged && me->HealthBelowPctDamaged(20, damage))
        {
            _isEnraged = true;
            DoCastSelf(SPELL_ENRAGE);
            // Acelera los eventos al entrar en enrage
            events.RescheduleEvent(EVENT_MIND_BLAST, 2s, 3s);
        }
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
                case EVENT_SHADOW_WORD_PAIN:
                    DoCastVictim(SPELL_SHADOW_WORD_PAIN);
                    events.ScheduleEvent(EVENT_SHADOW_WORD_PAIN, 15s, 20s);
                    break;
                case EVENT_MIND_BLAST:
                    DoCastVictim(SPELL_MIND_BLAST);
                    events.ScheduleEvent(EVENT_MIND_BLAST, _isEnraged ? 3s : 7s, _isEnraged ? 4s : 9s);
                    break;
                case EVENT_SLEEP:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                    {
                        Talk(SAY_SLEEP);
                        DoCast(target, SPELL_SLEEP);
                    }
                    events.ScheduleEvent(EVENT_SLEEP, 12s, 18s);
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    bool _isEnraged;
};

void AddSC_boss_kelris()
{
    RegisterBlackfathomDeepsCreatureAI(boss_kelris);
}


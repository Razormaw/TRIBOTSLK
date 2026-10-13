/*
 * TRIBOTSLK - Blackfathom Deeps: Gelihast (Mejorado)
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackfathom_deeps.h"

enum GelihastSpells
{
    SPELL_FROSTBOLT      = 15497,
    SPELL_LIGHTNING_BOLT = 9532,
    SPELL_NET            = 6533,
    SPELL_HEALING_WAVE   = 8005
};

enum GelihastEvents
{
    EVENT_FROSTBOLT = 1,
    EVENT_LIGHTNING_BOLT,
    EVENT_DEFENSIVE_NET,
    EVENT_HEAL
};

struct boss_gelihast : public BossAI
{
    boss_gelihast(Creature* creature) : BossAI(creature, DATA_GELIHAST), _didHeal(false) { }

    void Reset() override
    {
        _Reset();
        _didHeal = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(EVENT_FROSTBOLT, 2s, 4s);
        events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 6s, 9s);
    }

    void DamageTaken(Unit*, uint32& damage, DamageEffectType, SpellInfo const*) override
    {
        if (!_didHeal && me->HealthBelowPctDamaged(40, damage))
        {
            _didHeal = true;
            // Mecánica defensiva: Lanza red para alejar al tanque y curarse
            events.ScheduleEvent(EVENT_DEFENSIVE_NET, 1s);
            events.ScheduleEvent(EVENT_HEAL, 2s);
        }
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
                case EVENT_FROSTBOLT:
                    DoCastVictim(SPELL_FROSTBOLT);
                    events.ScheduleEvent(EVENT_FROSTBOLT, 4s, 6s);
                    break;
                case EVENT_LIGHTNING_BOLT:
                    DoCastVictim(SPELL_LIGHTNING_BOLT);
                    events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 7s, 10s);
                    break;
                case EVENT_DEFENSIVE_NET:
                    DoCastVictim(SPELL_NET);
                    break;
                case EVENT_HEAL:
                    DoCastSelf(SPELL_HEALING_WAVE);
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    bool _didHeal;
};

void AddSC_boss_gelihast()
{
    RegisterBlackfathomDeepsCreatureAI(boss_gelihast);
}


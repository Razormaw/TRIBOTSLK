/*
 * TRIBOTSLK - Monasterio Escarlata: Alto Inquisidor Fairbanks
 * Script mejorado con diálogos inmersivos y corrección de eventos.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "Timer.h"

enum HighInquisitorFairbanksYells
{
    SAY_AGGRO = 0,
    SAY_HEAL  = 1,
    SAY_DEATH = 2
};

enum HighInquisitorFairbanksSpells
{
    SPELL_CURSEOFBLOOD      = 8282,
    SPELL_DISPEL_MAGIC      = 15090,
    SPELL_FEAR              = 12096,
    SPELL_HEAL              = 12039,
    SPELL_POWERWORDSHIELD   = 11647,
    SPELL_SLEEP             = 8399
};

enum HighInquisitorFairbanksEvents
{
    EVENT_CURSE_BLOOD = 1,
    EVENT_DISPEL_MAGIC, // Corregido: antes decía DIPEL
    EVENT_FEAR,
    EVENT_SLEEP
};

class HighInquisitorFairbanksDispelMagicTargetSelector
{
public:
    HighInquisitorFairbanksDispelMagicTargetSelector(Unit* owner) : _me(owner) { }

    bool operator()(Unit* unit) const
    {
        if (unit->GetTypeId() != TYPEID_PLAYER || _me->GetDistance(unit) > 30.f)
            return false;

        DispelChargesList dispelList;
        unit->GetDispellableAuraList(_me, DISPEL_MAGIC, dispelList);
        if (dispelList.empty())
            return false;

        return true;
    }

private:
    Unit const* _me;
};

struct boss_high_inquisitor_fairbanks : public BossAI
{
    boss_high_inquisitor_fairbanks(Creature* creature) : BossAI(creature, DATA_HIGH_INQUISITOR_FAIRBANKS), _healTimer(0s), _powerWordShield(false) { }

    void Reset() override
    {
        _Reset();
        _healTimer.Reset(0s);
        _powerWordShield = false;
        me->SetStandState(UNIT_STAND_STATE_DEAD);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_CURSE_BLOOD, 10s);
        events.ScheduleEvent(EVENT_DISPEL_MAGIC, 30s);
        events.ScheduleEvent(EVENT_FEAR, 40s);
        events.ScheduleEvent(EVENT_SLEEP, 25s);
        
        me->SetStandState(UNIT_STAND_STATE_STAND);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (me->HealthBelowPctDamaged(25, damage))
        {
            if (!_powerWordShield)
            {
                Talk(SAY_HEAL); // Grita al activar su defensa desesperada
                DoCastSelf(SPELL_POWERWORDSHIELD);
                _powerWordShield = true;
            }

            if (!me->IsNonMeleeSpellCast(false) && _healTimer.Passed())
            {
                _healTimer.Reset(30s);
                DoCastSelf(SPELL_HEAL);
            }
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
            ExecuteEvent(eventId);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (!_healTimer.Passed())
            _healTimer.Update(diff);

        DoMeleeAttackIfReady();
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CURSE_BLOOD:
                DoCastVictim(SPELL_CURSEOFBLOOD);
                events.Repeat(25s);
                break;
            case EVENT_DISPEL_MAGIC:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, HighInquisitorFairbanksDispelMagicTargetSelector(me)))
                    DoCast(target, SPELL_DISPEL_MAGIC);
                events.Repeat(30s);
                break;
            case EVENT_FEAR:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 20.f, true))
                    DoCast(target, SPELL_FEAR);
                events.Repeat(40s);
                break;
            case EVENT_SLEEP:
                if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat, 0, 30.f, true, false))
                    DoCast(target, SPELL_SLEEP);
                events.Repeat(30s);
                break;
            default:
                break;
        }
    }

private:
    TimeTracker _healTimer;
    bool _powerWordShield;
};

void AddSC_boss_high_inquisitor_fairbanks()
{
    RegisterScarletMonasteryCreatureAI(boss_high_inquisitor_fairbanks);
}


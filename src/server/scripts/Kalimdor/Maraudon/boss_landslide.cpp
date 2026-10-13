/*
 * Landslide - Maraudon
 * Entry: 12259
 * TrinityCore 3.3.5a - Versión Epic Heroic Corregida (Fix Chrono)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_KNOCK_AWAY            = 18651, 
    SPELL_TRAMPLE               = 21864, 
    SPELL_EARTHQUAKE            = 21871, 
    SPELL_AVALANCHE_VISUAL      = 25055,
	SPELL_BOSS_ENRAGE           = 21915 
};

enum LandslideTexts
{
    SAY_AGGRO        = 0,
    SAY_ENRAGE       = 3,
    SAY_KILL         = 4,
    SAY_DEATH        = 5
};

enum LandslideEvents
{
    EVENT_KNOCK_AWAY    = 1,
    EVENT_TRAMPLE       = 2,
    EVENT_EARTHQUAKE    = 3,
    EVENT_SPAWN_ROCKS   = 4
};

enum BossPhases
{
    PHASE_NORMAL        = 1,
    PHASE_AVALANCHE     = 2
};

struct boss_landslide : public BossAI
{
    boss_landslide(Creature* creature) : BossAI(creature, DATA_LANDSLIDE)
    {
        _phase = PHASE_NORMAL;
    }

    void Reset() override
    {
        BossAI::Reset();
        _phase = PHASE_NORMAL;
        me->SetImmuneToAll(false);
    }

        void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_KNOCK_AWAY, Seconds(9)));
        events.ScheduleEvent(EVENT_TRAMPLE,    Seconds(13)));
        events.ScheduleEvent(EVENT_EARTHQUAKE, Seconds(22)));
    }

    void JustDied(Unit* killer) override
    {
        Talk(SAY_DEATH);
        BossAI::JustDied(killer);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_KILL);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (_phase == PHASE_NORMAL && HealthBelowPct(50))
        {
            _phase = PHASE_AVALANCHE;
            Talk(SAY_ENRAGE);
            DoCastSelf(SPELL_BOSS_ENRAGE);
            
            events.ScheduleEvent(EVENT_SPAWN_ROCKS, Seconds(1));
            events.RescheduleEvent(EVENT_EARTHQUAKE, Seconds(5));
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_KNOCK_AWAY:
                    DoCastVictim(SPELL_KNOCK_AWAY);
                    events.ScheduleEvent(EVENT_KNOCK_AWAY, Seconds(16)));
                    break;
                    
                case EVENT_TRAMPLE:
                    DoCastSelf(SPELL_TRAMPLE);
                    events.ScheduleEvent(EVENT_TRAMPLE, Seconds(18)));
                    break;
                    
                case EVENT_EARTHQUAKE:
                    DoCastSelf(SPELL_EARTHQUAKE);
                    events.ScheduleEvent(EVENT_EARTHQUAKE, Seconds(35)));
                    break;
                    
                case EVENT_SPAWN_ROCKS:
                    if (_phase == PHASE_AVALANCHE)
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        {
                            target->CastSpell(target, SPELL_AVALANCHE_VISUAL, true);
                        }
                        // Cambiado a Milliseconds(2500) para evitar el error del float con Seconds
                        events.ScheduleEvent(EVENT_SPAWN_ROCKS, Milliseconds(2500));
                    }
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_landslide()
{
    RegisterCreatureAI(boss_landslide);
}



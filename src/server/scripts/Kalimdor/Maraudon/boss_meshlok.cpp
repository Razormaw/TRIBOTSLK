/*
 * Meshlok the Harvester - Maraudon (Rare Boss)
 * Entry: 12258
 * TrinityCore 3.3.5a - Versión Epic Heroic Adaptada (Fix Position & Chrono)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_STONE_FIST            = 21864, 
    SPELL_EARTHQUAKE_MUD        = 21871, 
    SPELL_SLIME_AURA            = 21909  
};

enum MeshlokTexts
{
    SAY_AGGRO       = 0,
    SAY_EARTHQUAKE  = 3,
    SAY_ENRAGE      = 4,
    SAY_KILL        = 5,
    SAY_DEATH       = 6
};

enum MeshlokEvents
{
    EVENT_STONE_FIST    = 1,
    EVENT_EARTHQUAKE    = 2,
    EVENT_SUMMON_ADDS   = 3
};

enum BossPhases
{
    PHASE_NORMAL        = 1,
    PHASE_HARVEST       = 2
};

enum CustomAdds
{
    NPC_CAVERN_LARVA    = 12221 
};

struct boss_meshlok : public BossAI
{
    boss_meshlok(Creature* creature) : BossAI(creature, DATA_MESHLOK)
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
        
        events.ScheduleEvent(EVENT_STONE_FIST,   Seconds(urand(6, 10)));
        events.ScheduleEvent(EVENT_EARTHQUAKE,   Seconds(urand(15, 22)));
        events.ScheduleEvent(EVENT_SUMMON_ADDS,  Seconds(urand(20, 28)));
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

        if (_phase == PHASE_NORMAL && HealthBelowPct(30))
        {
            _phase = PHASE_HARVEST;
            Talk(SAY_ENRAGE);
            
            DoCastSelf(SPELL_MESHLOK_ENRAGE);
            DoCastSelf(SPELL_SLIME_AURA); 
            
            events.RescheduleEvent(EVENT_SUMMON_ADDS, Seconds(1));
            events.RescheduleEvent(EVENT_STONE_FIST, Seconds(4));
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_STONE_FIST:
                    DoCastVictim(SPELL_STONE_FIST);
                    events.ScheduleEvent(EVENT_STONE_FIST, Seconds(urand(12, 16)));
                    break;
                    
                case EVENT_EARTHQUAKE:
                    Talk(SAY_EARTHQUAKE);
                    DoCastSelf(SPELL_EARTHQUAKE_MUD);
                    events.ScheduleEvent(EVENT_EARTHQUAKE, Seconds(urand(25, 32)));
                    break;
                    
                                case EVENT_SUMMON_ADDS:
                    for (uint8 i = 0; i < 3; ++i)
                    {
                        if (Creature* larva = me->SummonCreature(NPC_CAVERN_LARVA, me->GetPositionX() + urand(-6, 6), me->GetPositionY() + urand(-6, 6), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(30000)))
                        {
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
                                larva->AI()->AttackStart(target);
                            else if (me->GetVictim())
                                larva->AI()->AttackStart(me->GetVictim());
                        }
                    }
                    events.ScheduleEvent(EVENT_SUMMON_ADDS, Seconds(_phase == PHASE_HARVEST ? 15 : 35));
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_meshlok()
{
    RegisterCreatureAI(boss_meshlok);
}



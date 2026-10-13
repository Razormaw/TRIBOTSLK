/*
 * Noxxion - Maraudon
 * Entry: 13280
 * TrinityCore 3.3.5a - TRIBOTSLK Epic Heroic Edition (Fixed Phase Gate)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_TOXIC_VOLLEY          = 21857, 
    SPELL_UPPERCUT              = 10966, 
    SPELL_POISON_CLOUD          = 21853  
};

enum NoxxionTexts
{
    SAY_AGGRO           = 0,
    SAY_SPLIT           = 1,
    SAY_MERGE           = 2,
    SAY_KILL            = 3,
    SAY_DEATH           = 4
};

enum NoxxionEvents
{
    EVENT_TOXIC_VOLLEY  = 1,
    EVENT_UPPERCUT      = 2,
    EVENT_CHECK_ADDS    = 3
};

enum NoxxionPhases
{
    PHASE_NORMAL        = 1,
    PHASE_SPLIT         = 2,
    PHASE_FINAL         = 3
};

struct boss_noxxion : public BossAI
{
    boss_noxxion(Creature* creature) : BossAI(creature, DATA_NOXXION)
    {
        _phase = PHASE_NORMAL;
    }

    void Reset() override
    {
        BossAI::Reset();
        _phase = PHASE_NORMAL;
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_AGGRESSIVE);
        summons.DespawnAll();
    }

        void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_TOXIC_VOLLEY,  Seconds(9)));
        events.ScheduleEvent(EVENT_UPPERCUT,      Seconds(13))));
    }

    void JustDied(Unit* killer) override
    {
        Talk(SAY_DEATH);
        summons.DespawnAll();
        BossAI::JustDied(killer);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
            summon->AI()->AttackStart(target);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    void EnterSplitPhase()
    {
        _phase = PHASE_SPLIT;
        Talk(SAY_SPLIT);
        
        me->AttackStop();
        me->InterruptNonMeleeSpells(false);
        me->SetImmuneToAll(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetVisible(false); 

        for (uint8 i = 0; i < 5; ++i)
        {
            me->SummonCreature(NPC_NOXXION_SPAWN, me->GetPositionX() + urand(-7, 7), me->GetPositionY() + urand(-7, 7), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(60000));
        }

        events.ScheduleEvent(EVENT_CHECK_ADDS, Seconds(1));
    }

    void ExitSplitPhase()
    {
        _phase = PHASE_FINAL;
        Talk(SAY_MERGE);
        
        me->SetVisible(true);
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_AGGRESSIVE);
        
        if (me->GetVictim())
            me->GetMotionMaster()->MoveChase(me->GetVictim());

        DoCastSelf(SPELL_NOXXION_FRENZY);
        DoCastSelf(SPELL_POISON_CLOUD); 

        events.RescheduleEvent(EVENT_TOXIC_VOLLEY, Seconds(2));
        events.RescheduleEvent(EVENT_UPPERCUT, Seconds(6));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        // GATILLO SEGURO: Salto forzado al 50% de salud
        if (_phase == PHASE_NORMAL && me->GetHealthPct() <= 50.0f)
        {
            EnterSplitPhase();
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TOXIC_VOLLEY:
                    DoCastSelf(SPELL_TOXIC_VOLLEY);
                    events.ScheduleEvent(EVENT_TOXIC_VOLLEY, Seconds(_phase == PHASE_FINAL ? 10 : 16));
                    break;
                    
                case EVENT_UPPERCUT:
                    DoCastVictim(SPELL_UPPERCUT);
                    events.ScheduleEvent(EVENT_UPPERCUT, Seconds(17)));
                    break;

                case EVENT_CHECK_ADDS:
                    if (_phase == PHASE_SPLIT)
                    {
                        if (summons.empty())
                        {
                            ExitSplitPhase();
                        }
                        else
                        {
                            events.ScheduleEvent(EVENT_CHECK_ADDS, Seconds(1));
                        }
                    }
                    break;
            }
        }

        if (_phase != PHASE_SPLIT)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_noxxion()
{
    RegisterCreatureAI(boss_noxxion);
}



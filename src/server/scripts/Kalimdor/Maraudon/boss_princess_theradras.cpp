/*
 * Princess Theradras - Maraudon (Jefe Final)
 * Entry: 12225
 * TrinityCore 3.3.5a - TRIBOTSLK Epic Heroic Edition (Fixed Phase Gate)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_DUST_FIELD             = 21909, 
    SPELL_BOULDER                = 21832, 
    SPELL_THRASH                 = 3391,  
    SPELL_REPULSIVE_GAZE         = 21869, 
    SPELL_EARTHQUAKE             = 21864, 
    SPELL_ENRAGE                 = 21915  
};

enum CustomEvents
{
    EVENT_DUST_FIELD             = 1,
    EVENT_BOULDER                = 2,
    EVENT_THRASH                 = 3,
    EVENT_REPULSIVE_GAZE         = 4,
    EVENT_END_TRANSITION         = 5
};

enum BossPhases
{
    PHASE_NORMAL                 = 1,
    PHASE_TRANSITION             = 2,
    PHASE_FINAL_BURST            = 3
};

enum CustomAdds
{
    NPC_THERADRAS_MINION         = 12222 
};

struct boss_princess_theradras : public BossAI
{
    boss_princess_theradras(Creature* creature) : BossAI(creature, DATA_THERADRAS)
    {
        _phase = PHASE_NORMAL;
    }

    void Reset() override
    {
        BossAI::Reset();
        _phase = PHASE_NORMAL;
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        
        events.ScheduleEvent(EVENT_DUST_FIELD,     Seconds(12));
        events.ScheduleEvent(EVENT_BOULDER,        Seconds(6));
        events.ScheduleEvent(EVENT_THRASH,         Seconds(8));
        events.ScheduleEvent(EVENT_REPULSIVE_GAZE, Seconds(18));
    }

    void JustDied(Unit* killer) override
    {
        me->SummonCreature(NPC_ZAETAR_SPIRIT, 28.1887f, 62.3964f, -123.161f, 4.31096f, TEMPSUMMON_MANUAL_DESPAWN);
        BossAI::JustDied(killer);
    }

    void StartEarthPhase()
    {
        _phase = PHASE_TRANSITION;
        me->AttackStop();
        me->InterruptNonMeleeSpells(false); // Interrumpe cualquier casteo para forzar el terremoto
        me->SetImmuneToAll(true);
        me->SetReactState(REACT_PASSIVE);
        
        Talk(3); 
        DoCastSelf(SPELL_EARTHQUAKE);

        me->SummonCreature(NPC_THERADRAS_MINION, me->GetPositionX() + 5.0f, me->GetPositionY() + 5.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(30000));
        me->SummonCreature(NPC_THERADRAS_MINION, me->GetPositionX() - 5.0f, me->GetPositionY() - 5.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(30000));

        events.CancelEvent(EVENT_DUST_FIELD);
        events.CancelEvent(EVENT_BOULDER);
        
        events.ScheduleEvent(EVENT_END_TRANSITION, Milliseconds(6000)); 
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        // GATILLO SEGURO ANTES DEL CASTING: Activación al 30% obligatorio
        if (_phase == PHASE_NORMAL && me->GetHealthPct() <= 30.0f)
        {
            StartEarthPhase();
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DUST_FIELD:
                    DoCastSelf(SPELL_DUST_FIELD);
                    events.ScheduleEvent(EVENT_DUST_FIELD, Seconds(_phase == PHASE_FINAL_BURST ? 10 : 18));
                    break;
                    
                case EVENT_BOULDER:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
                        DoCast(target, SPELL_BOULDER);
                    events.ScheduleEvent(EVENT_BOULDER, Seconds(12));
                    break;
                    
                case EVENT_THRASH:
                    DoCastSelf(SPELL_THRASH);
                    events.ScheduleEvent(EVENT_THRASH, Seconds(15));
                    break;
                    
                case EVENT_REPULSIVE_GAZE:
                    DoCastSelf(SPELL_REPULSIVE_GAZE);
                    events.ScheduleEvent(EVENT_REPULSIVE_GAZE, Seconds(25));
                    break;

                case EVENT_END_TRANSITION:
                    _phase = PHASE_FINAL_BURST;
                    me->SetImmuneToAll(false);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveAurasDueToSpell(SPELL_EARTHQUAKE);
                    
                    DoCastSelf(SPELL_ENRAGE);
                    
                    events.ScheduleEvent(EVENT_DUST_FIELD, Seconds(4));
                    events.ScheduleEvent(EVENT_BOULDER, Seconds(2));
                    break;
            }
        }

        if (_phase != PHASE_TRANSITION)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_ptheradras()
{
    RegisterCreatureAI(boss_princess_theradras);
}



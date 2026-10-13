/*
 * Rotgrip - Maraudon
 * Entry: 12237
 * TrinityCore 3.3.5a - TRIBOTSLK Epic Heroic Edition (Fixed Phase Gate)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_TAIL_SWIPE            = 21869, 
    SPELL_WHIRLPOOL_PULL        = 21909  
};

enum RotgripTexts
{
    SAY_AGGRO       = 0,
    SAY_SUBMERGE    = 1,
    SAY_SURFACE     = 2,
    SAY_KILL        = 3,
    SAY_DEATH       = 4
};

enum RotgripEvents
{
    EVENT_BITE          = 1,
    EVENT_TAIL_SWIPE    = 2,
    EVENT_CHECK_FISH    = 3
};

enum RotgripPhases
{
    PHASE_SURFACE_1     = 1,
    PHASE_DEEP_HUNTER_1 = 2,
    PHASE_SURFACE_2     = 3,
    PHASE_DEEP_HUNTER_2 = 4,
    PHASE_FINAL_BURST   = 5
};

enum CustomAdds
{
    NPC_MARAUDON_FRENZY = 12223 
};

struct boss_rotgrip : public BossAI
{
    boss_rotgrip(Creature* creature) : BossAI(creature, DATA_ROTGRIP)
    {
        _phase = PHASE_SURFACE_1;
    }

    void Reset() override
    {
        BossAI::Reset();
        _phase = PHASE_SURFACE_1;
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetVisible(true);
        summons.DespawnAll();
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_BITE,       Seconds(5));
        events.ScheduleEvent(EVENT_TAIL_SWIPE, Seconds(10));
    }

    void JustDied(Unit* killer) override
    {
        Talk(SAY_DEATH);
        summons.DespawnAll();
        BossAI::JustDied(killer);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_KILL);
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

    void EnterSubmergePhase(uint8 nextPhase)
    {
        _phase = nextPhase;
        Talk(SAY_SUBMERGE);
        
        me->AttackStop();
        me->InterruptNonMeleeSpells(false);
        me->SetImmuneToAll(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetVisible(false); 

        DoCastSelf(SPELL_WHIRLPOOL_PULL);

                // Invoca los 4 peces usando la dispersión nativa del núcleo
        for (uint8 i = 0; i < 4; ++i)
        {
            me->SummonCreature(NPC_MARAUDON_FRENZY, me->GetPositionX() + urand(-8, 8), me->GetPositionY() + urand(-8, 8), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(45000));
        }

        events.ScheduleEvent(EVENT_CHECK_FISH, Seconds(1));
    }

    void ExitSubmergePhase(uint8 nextSurfacePhase)
    {
        _phase = nextSurfacePhase;
        Talk(SAY_SURFACE);
        me->RemoveAurasDueToSpell(SPELL_WHIRLPOOL_PULL);

        me->SetVisible(true);
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_AGGRESSIVE);
        
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
            me->AI()->AttackStart(target);
        else if (me->GetVictim())
            me->GetMotionMaster()->MoveChase(me->GetVictim());

        if (_phase == PHASE_FINAL_BURST)
        {
            DoCastSelf(SPELL_ROTGRIP_ENRAGE); 
        }

        events.RescheduleEvent(EVENT_BITE, Seconds(2));
        events.RescheduleEvent(EVENT_TAIL_SWIPE, Seconds(6));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        // GATILLO SEGURO ANTES DEL CASTING: Control de las sumersiones dinámicas
        if (_phase == PHASE_SURFACE_1 && me->GetHealthPct() <= 60.0f)
        {
            EnterSubmergePhase(PHASE_DEEP_HUNTER_1);
            return;
        }
        else if (_phase == PHASE_SURFACE_2 && me->GetHealthPct() <= 25.0f)
        {
            EnterSubmergePhase(PHASE_DEEP_HUNTER_2);
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_BITE:
                    DoCastVictim(SPELL_ROTGRIP_BITE);
                    events.ScheduleEvent(EVENT_BITE, Seconds(urand(8, 13)));
                    break;
                    
                case EVENT_TAIL_SWIPE:
                    DoCastVictim(SPELL_TAIL_SWIPE);
                    events.ScheduleEvent(EVENT_TAIL_SWIPE, Seconds(urand(12, 17)));
                    break;

                case EVENT_CHECK_FISH:
                    if (_phase == PHASE_DEEP_HUNTER_1 || _phase == PHASE_DEEP_HUNTER_2)
                    {
                        if (summons.empty())
                        {
                            if (_phase == PHASE_DEEP_HUNTER_1)
                                ExitSubmergePhase(PHASE_SURFACE_2);
                            else
                                ExitSubmergePhase(PHASE_FINAL_BURST);
                        }
                        else
                        {
                            events.ScheduleEvent(EVENT_CHECK_FISH, Seconds(1));
                        }
                    }
                    break;
            }
        }

        if (_phase != PHASE_DEEP_HUNTER_1 && _phase != PHASE_DEEP_HUNTER_2)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_rotgrip()
{
    RegisterCreatureAI(boss_rotgrip);
}



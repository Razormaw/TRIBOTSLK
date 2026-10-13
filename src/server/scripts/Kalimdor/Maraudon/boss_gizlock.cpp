/*
 * Tinkerer Gizlock - Maraudon
 * Entry: 12256
 * TrinityCore 3.3.5a - Versión Epic Heroic Corregida
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_DRAGON_GUN             = 21876, 
    SPELL_GOBLIN_BOMB            = 11841, 
    SPELL_SHOOT                  = 15620, 
    SPELL_HYPERSPEED             = 24312, 
    SPELL_GOBLIN_SHIELD          = 15618  
};

enum GizlockTexts
{
    SAY_AGGRO       = 0, 
    SAY_HYPERSPEED  = 1,
    SAY_BOMB        = 2,
    SAY_ENRAGE      = 3,
    SAY_KILL        = 4,
    SAY_DEATH       = 5
};

enum GizlockEvents
{
    EVENT_DRAGON_GUN    = 1,
    EVENT_GOBLIN_BOMB   = 2,
    EVENT_GOBLIN_SHIELD = 3,
    EVENT_SHOOT         = 4
};

enum BossPhases
{
    PHASE_NORMAL        = 1,
    PHASE_OVERCLOCK     = 2
};

struct boss_gizlock : public BossAI
{
    boss_gizlock(Creature* creature) : BossAI(creature, DATA_GIZLOCK)
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
        
        events.ScheduleEvent(EVENT_DRAGON_GUN,    Seconds(urand(6, 10)));
        events.ScheduleEvent(EVENT_GOBLIN_BOMB,   Seconds(urand(10, 15)));
        events.ScheduleEvent(EVENT_GOBLIN_SHIELD, Seconds(urand(20, 25)));
        events.ScheduleEvent(EVENT_SHOOT,         Seconds(urand(3, 5)));
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

        if (_phase == PHASE_NORMAL && HealthBelowPct(35))
        {
            _phase = PHASE_OVERCLOCK;
            Talk(SAY_ENRAGE);
            DoCastSelf(SPELL_HYPERSPEED); 
            
            events.RescheduleEvent(EVENT_DRAGON_GUN, Seconds(2));
            events.RescheduleEvent(EVENT_GOBLIN_BOMB, Seconds(4));
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DRAGON_GUN:
                    DoCastVictim(SPELL_DRAGON_GUN);
                    events.ScheduleEvent(EVENT_DRAGON_GUN, Seconds(_phase == PHASE_OVERCLOCK ? 10 : 16));
                    break;
                    
                case EVENT_GOBLIN_BOMB:
                    Talk(SAY_BOMB);
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
                        DoCast(target, SPELL_GOBLIN_BOMB);
                    else if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                        DoCast(target, SPELL_GOBLIN_BOMB);

                    events.ScheduleEvent(EVENT_GOBLIN_BOMB, Seconds(urand(12, 18)));
                    break;
                    
                case EVENT_GOBLIN_SHIELD:
                    Talk(SAY_HYPERSPEED);
                    DoCastSelf(SPELL_GOBLIN_SHIELD);
                    events.ScheduleEvent(EVENT_GOBLIN_SHIELD, Seconds(urand(25, 35)));
                    break;
                    
                case EVENT_SHOOT:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    {
                        if (!me->IsWithinMeleeRange(target))
                            DoCast(target, SPELL_SHOOT);
                    }
                    events.ScheduleEvent(EVENT_SHOOT, Seconds(urand(5, 8)));
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_gizlock()
{
    RegisterCreatureAI(boss_gizlock);
}



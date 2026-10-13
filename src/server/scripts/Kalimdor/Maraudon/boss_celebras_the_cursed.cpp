/*
 * Celebras the Cursed - Maraudon
 * Entry: 12236
 * TrinityCore 3.3.5a - Versión Epic Heroic Adaptada
 */

#include "ScriptPCH.h"
#include "maraudon.h"

// Solo declaramos los hechizos que NO están en maraudon.h
enum Spells
{
    SPELL_CORRUPT_FORCES        = 21811, 
    SPELL_NATURE_RAGE           = 24312, 
    SPELL_HURRICANE             = 21807  
};

enum CelebrasTexts
{
    SAY_AGGRO           = 0, 
    SAY_SEED            = 1,
    SAY_TRANSFORM       = 2,
    SAY_KILL            = 3,
    SAY_DEATH           = 4,
    EMOTE_CORRUPTION    = 5
};

enum CelebrasEvents
{
    EVENT_WRATH         = 1,
    EVENT_ROOTS         = 2,
    EVENT_CORRUPT       = 3,
    EVENT_SUMMON_ADDS   = 4,
    EVENT_NATURE_RAGE   = 5
};

enum BossPhases
{
    PHASE_NORMAL        = 1,
    PHASE_BURST         = 2
};

enum CustomAdds
{
    NPC_CORRUPT_TREANT   = 12224 
};

struct boss_celebras_the_cursed : public BossAI
{
    boss_celebras_the_cursed(Creature* creature) : BossAI(creature, DATA_CELEBRAS)
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
        
        events.ScheduleEvent(EVENT_WRATH,       Seconds(2));
        events.ScheduleEvent(EVENT_ROOTS,       Seconds(urand(8, 12)));
        events.ScheduleEvent(EVENT_CORRUPT,     Seconds(urand(15, 20)));
    }

    void JustDied(Unit* killer) override
    {
        Talk(SAY_DEATH);
        me->SummonCreature(NPC_CELEBRAS_REDEEMED, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN);
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

        if (_phase == PHASE_NORMAL && HealthBelowPct(25))
        {
            _phase = PHASE_BURST;
            Talk(SAY_TRANSFORM);
            
            DoCastSelf(SPELL_NATURE_RAGE);
            DoCastSelf(SPELL_HURRICANE); 
            
            events.RescheduleEvent(EVENT_WRATH, Seconds(1));
            events.RescheduleEvent(EVENT_ROOTS, Seconds(3));
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_WRATH:
                    DoCastVictim(SPELL_CELEBRAS_WRATH);
                    events.ScheduleEvent(EVENT_WRATH, Seconds(_phase == PHASE_BURST ? 2 : 4));
                    break;
                    
                case EVENT_ROOTS:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 35.0f, true))
                        DoCast(target, SPELL_CELEBRAS_ROOTS);
                    
                    events.ScheduleEvent(EVENT_ROOTS, Seconds(urand(12, 18)));
                    break;
                    
                                case EVENT_CORRUPT:
                    Talk(EMOTE_CORRUPTION);
                    DoCastSelf(SPELL_CORRUPT_FORCES);
                    
                    for (uint8 i = 0; i < 3; ++i)
                    {
                        Position pos;
                        me->GetRandomNearPosition(5.0f); // Tu core calcula la posición aleatoria directamente sobre el objeto 'me'
                        if (Creature* treant = me->SummonCreature(NPC_CORRUPT_TREANT, me->GetPositionX() + urand(-5, 5), me->GetPositionY() + urand(-5, 5), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(45000)))
                        {
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
                                treant->AI()->AttackStart(target); 
                            else if (me->GetVictim())
                                treant->AI()->AttackStart(me->GetVictim());
                        }
                    }
                    events.ScheduleEvent(EVENT_CORRUPT, Seconds(urand(25, 30)));
                    break;

            }
        }

        if (_phase != PHASE_BURST)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_celebras_the_cursed()
{
    RegisterCreatureAI(boss_celebras_the_cursed);
}



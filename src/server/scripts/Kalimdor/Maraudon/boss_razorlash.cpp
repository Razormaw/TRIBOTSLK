/*
 * Razorlash - Maraudon
 * Entry: 12257
 * TrinityCore 3.3.5a - Versión Mejorada No-Piñata Corregida (Fix CastSpell Coordinate)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

// Eliminado SPELL_RAZORLASH_ENRAGE porque ya existe en maraudon.h
enum Spells
{
    SPELL_THRASH                = 3391,  // Otorga 2 ataques extras instantáneos al jefe
    SPELL_IMPALING_ROOTS        = 21814, // Inmoviliza y aturde temporalmente a un objetivo
    SPELL_NOXIOUS_POISON        = 21857, // Escupitajo de veneno natural directo y acumulable
    SPELL_POISON_CLOUD_VISUAL   = 21853  // Nube visual de esporas venenosas en el suelo
};

enum RazorlashTexts
{
    SAY_AGGRO       = 0,
    SAY_ENRAGE      = 1,
    SAY_KILL        = 2,
    SAY_DEATH       = 3
};

enum RazorlashEvents
{
    EVENT_THRASH        = 1,
    EVENT_ROOT          = 2,
    EVENT_POISON        = 3,
    EVENT_SPAWN_SPORES   = 4
};

enum BossPhases
{
    PHASE_NORMAL        = 1,
    PHASE_SPORES        = 2
};

struct boss_razorlash : public BossAI
{
    boss_razorlash(Creature* creature) : BossAI(creature, DATA_RAZORLASH)
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
        
        events.ScheduleEvent(EVENT_THRASH, Seconds(urand(5, 8)));
        events.ScheduleEvent(EVENT_ROOT,   Seconds(urand(12, 18)));
        events.ScheduleEvent(EVENT_POISON, Seconds(urand(8, 14)));
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

        // Activación de la Fase Épica de Esporas Venenosas (Al llegar al 25% de salud)
        if (_phase == PHASE_NORMAL && HealthBelowPct(25))
        {
            _phase = PHASE_SPORES;
            Talk(SAY_ENRAGE);
            
            DoCastSelf(SPELL_RAZORLASH_ENRAGE);
            
            // Programación caótica de la fase de ejecución final
            events.ScheduleEvent(EVENT_SPAWN_SPORES, Seconds(1));
            events.RescheduleEvent(EVENT_THRASH, Seconds(3));
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_THRASH:
                    DoCastVictim(SPELL_THRASH);
                    events.ScheduleEvent(EVENT_THRASH, Seconds(urand(10, 15)));
                    break;
                    
                case EVENT_ROOT:
                    // Busca preferentemente a un sanador o rango aleatorio alejado
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 35.0f, true))
                    {
                        DoCast(target, SPELL_IMPALING_ROOTS);
                    }
                    else if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                    {
                        DoCast(target, SPELL_IMPALING_ROOTS);
                    }
                    events.ScheduleEvent(EVENT_ROOT, Seconds(18)));
                    break;
                    
                case EVENT_POISON:
                    DoCastVictim(SPELL_NOXIOUS_POISON);
                    events.ScheduleEvent(EVENT_POISON, Seconds(16)));
                    break;

                case EVENT_SPAWN_SPORES:
                    if (_phase == PHASE_SPORES)
                    {
                        // Se corrigió el casteo pasándole el puntero target nativo sin desbordar argumentos
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        {
                            target->CastSpell(target, SPELL_POISON_CLOUD_VISUAL, true);
                        }
                        // Lanza esporas continuamente cada 4 segundos hasta caer derrotado
                        events.ScheduleEvent(EVENT_SPAWN_SPORES, Seconds(4));
                    }
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_razorlash()
{
    RegisterCreatureAI(boss_razorlash);
}



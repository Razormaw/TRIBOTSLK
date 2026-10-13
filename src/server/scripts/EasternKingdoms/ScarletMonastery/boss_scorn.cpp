/*
 * TRIBOTSLK - Monasterio Escarlata: Scorn
 * Script mejorado con fase de enrage, corrección de AOE y diálogos seguros.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum ScornYells
{
    SAY_AGGRO  = 0,
    SAY_ENRAGE = 1,
    SAY_KILL   = 2,
    SAY_DEATH  = 3
};

enum ScornSpells
{
    SPELL_LICHSLAP         = 28873,
    SPELL_FROSTBOLT_VOLLEY = 8398,
    SPELL_MINDFLAY         = 17313,
    SPELL_FROSTNOVA        = 15531
};

enum ScornEvents
{
    EVENT_LICH_SLAP = 1,
    EVENT_FROSTBOLT_VOLLEY,
    EVENT_MIND_FLAY,
    EVENT_FROST_NOVA,
    EVENT_CHECK_HEALTH
};

struct boss_scorn : public BossAI
{
    boss_scorn(Creature* creature) : BossAI(creature, DATA_SCORN), _enraged(false) { }

    void Reset() override
    {
        _Reset();
        _enraged = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_LICH_SLAP, 10s, 15s);
        events.ScheduleEvent(EVENT_FROSTBOLT_VOLLEY, 15s, 20s);
        events.ScheduleEvent(EVENT_MIND_FLAY, 20s, 25s);
        events.ScheduleEvent(EVENT_FROST_NOVA, 25s, 30s);
        events.ScheduleEvent(EVENT_CHECK_HEALTH, 1s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
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
            switch (eventId)
            {
                case EVENT_LICH_SLAP:
                    DoCastVictim(SPELL_LICHSLAP);
                    events.Repeat(10s, 15s);
                    break;
                case EVENT_FROSTBOLT_VOLLEY:
                    DoCastAOE(SPELL_FROSTBOLT_VOLLEY); // Corregido a AOE
                    events.Repeat(15s, 20s);
                    break;
                case EVENT_MIND_FLAY:
                    DoCastVictim(SPELL_MINDFLAY);
                    events.Repeat(_enraged ? 10s : 20s); // Más rápido en fase de enrage
                    break;
                case EVENT_FROST_NOVA:
                    DoCastAOE(SPELL_FROSTNOVA); // Corregido a AOE
                    events.Repeat(20s, 25s);
                    break;
                case EVENT_CHECK_HEALTH:
                    if (!_enraged && me->HealthBelowPct(30))
                    {
                        _enraged = true;
                        Talk(SAY_ENRAGE);
                        // Fase de desesperación: Nova inmediata y acelera ataques
                        DoCastAOE(SPELL_FROSTNOVA);
                    }
                    events.Repeat(1s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    bool _enraged;
};

void AddSC_boss_scorn()
{
    RegisterScarletMonasteryCreatureAI(boss_scorn);
}


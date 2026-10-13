/*
 * TRIBOTSLK - Monasterio Escarlata: Azshir el Insomne
 * Script mejorado con fases seguras, verificación de vida por eventos y diálogos.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum AzshirYells
{
    SAY_AGGRO       = 0,
    SAY_SIPHON      = 1,
    SAY_DEATH       = 2
};

enum AzshirSpells
{
    SPELL_CALL_OF_THE_GRAVE = 17831,
    SPELL_TERRIFY           = 7399,
    SPELL_SOUL_SIPHON       = 7290
};

enum AzshirEvents
{
    EVENT_CALL_OF_GRAVE = 1,
    EVENT_TERRIFY,
    EVENT_SOUL_SIPHON,
    EVENT_CHECK_HEALTH
};

struct boss_azshir_the_sleepless : public BossAI
{
    boss_azshir_the_sleepless(Creature* creature) : BossAI(creature, DATA_AZSHIR), _siphon(false) { }

    void Reset() override
    {
        _Reset();
        _siphon = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_CALL_OF_GRAVE, 30s);
        events.ScheduleEvent(EVENT_TERRIFY, 20s);
        events.ScheduleEvent(EVENT_CHECK_HEALTH, 1s); // Verificación segura de vida
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
                case EVENT_CALL_OF_GRAVE:
                    DoCastVictim(SPELL_CALL_OF_THE_GRAVE);
                    events.Repeat(30s);
                    break;
                case EVENT_TERRIFY:
                    DoCastVictim(SPELL_TERRIFY);
                    events.Repeat(20s);
                    break;
                case EVENT_CHECK_HEALTH:
                    if (!_siphon && HealthBelowPct(50))
                    {
                        _siphon = true;
                        Talk(SAY_SIPHON);
                        DoCastVictim(SPELL_SOUL_SIPHON);
                        events.ScheduleEvent(EVENT_SOUL_SIPHON, 20s);
                    }
                    events.Repeat(1s);
                    break;
                case EVENT_SOUL_SIPHON:
                    DoCastVictim(SPELL_SOUL_SIPHON);
                    events.Repeat(20s);
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
    bool _siphon;
};

void AddSC_boss_azshir_the_sleepless()
{
    RegisterScarletMonasteryCreatureAI(boss_azshir_the_sleepless);
}


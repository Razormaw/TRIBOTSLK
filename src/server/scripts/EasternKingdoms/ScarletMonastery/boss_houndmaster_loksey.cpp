/*
 * TRIBOTSLK - Monasterio Escarlata: Maestro de Sabuesos Loksey
 * Script mejorado con fases interactivas, optimización de eventos y diálogos seguros.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum HoundmasterLokseyYells
{
    SAY_AGGRO  = 0,
    SAY_HEALTH = 1,
    SAY_KILL   = 2,
    SAY_DEATH  = 3
};

enum HoundmasterLokseySpells
{
    SPELL_SUMMON_SCARLET_HOUND = 17164,
    SPELL_BLOODLUST            = 6742
};

enum HoundmasterLokseyEvents
{
    EVENT_CHECK_HEALTH = 1
};

struct boss_houndmaster_loksey : public BossAI
{
    boss_houndmaster_loksey(Creature* creature) : BossAI(creature, DATA_HOUNDMASTER_LOKSEY), _bloodlustUsed(false) { }

    void Reset() override
    {
        _Reset();
        _bloodlustUsed = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        // Invoca un sabueso al iniciar el combate
        DoCastSelf(SPELL_SUMMON_SCARLET_HOUND);
        
        // Programa la verificación de vida cada 1 segundo
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
                case EVENT_CHECK_HEALTH:
                    if (!_bloodlustUsed && me->HealthBelowPct(60))
                    {
                        _bloodlustUsed = true;
                        Talk(SAY_HEALTH);
                        
                        // Fase interactiva: Ansia de sangre + invoca un segundo sabueso para presionar al grupo
                        DoCastSelf(SPELL_BLOODLUST);
                        DoCastSelf(SPELL_SUMMON_SCARLET_HOUND);
                    }
                    events.Repeat(1s);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    bool _bloodlustUsed;
};

void AddSC_boss_houndmaster_loksey()
{
    RegisterScarletMonasteryCreatureAI(boss_houndmaster_loksey);
}


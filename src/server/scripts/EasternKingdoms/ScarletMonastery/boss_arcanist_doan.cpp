/*
 * TRIBOTSLK - Monasterio Escarlata: Arcanist Doan
 * Script mejorado con fases interactivas, corrección de AoE y diálogos seguros.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum ArcanistDoanYells
{
    SAY_AGGRO       = 0,
    SAY_SPECIALAE   = 1,
    SAY_KILL        = 2,
    SAY_DEATH       = 3
};

enum ArcanistDoanSpells
{
    SPELL_SILENCE          = 8988,
    SPELL_ARCANE_EXPLOSION = 9433,
    SPELL_DETONATION       = 9435,
    SPELL_ARCANE_BUBBLE    = 9438,
    SPELL_POLYMORPH        = 13323
};

enum ArcanistDoanEvents
{
    EVENT_SILENCE = 1,
    EVENT_ARCANE_EXPLOSION,
    EVENT_POLYMORPH,
    EVENT_CHECK_HEALTH,
    EVENT_DETONATION
};

struct boss_arcanist_doan : public BossAI
{
    boss_arcanist_doan(Creature* creature) : BossAI(creature, DATA_ARCANIST_DOAN), _healthAbove50Pct(true) { }

    void Reset() override
    {
        _Reset();
        _healthAbove50Pct = true;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_SILENCE, 15s, 20s);
        events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, 3s, 5s);
        events.ScheduleEvent(EVENT_POLYMORPH, 30s, 35s);
        events.ScheduleEvent(EVENT_CHECK_HEALTH, 1s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
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
                case EVENT_SILENCE:
                    DoCastVictim(SPELL_SILENCE);
                    events.Repeat(15s, 20s);
                    break;
                case EVENT_ARCANE_EXPLOSION:
                    DoCastAOE(SPELL_ARCANE_EXPLOSION); // Corregido a AoE
                    events.Repeat(8s, 12s);
                    break;
                case EVENT_POLYMORPH:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 30.0f, true))
                        DoCast(target, SPELL_POLYMORPH);
                    events.Repeat(20s, 25s);
                    break;
                case EVENT_CHECK_HEALTH:
                    if (_healthAbove50Pct && HealthBelowPct(50))
                    {
                        _healthAbove50Pct = false;
                        Talk(SAY_SPECIALAE);
                        DoCastSelf(SPELL_ARCANE_BUBBLE);
                        // Programamos la detonación con un ligero retraso para que la burbuja sea visible
                        events.ScheduleEvent(EVENT_DETONATION, 1500ms);
                    }
                    events.Repeat(1s);
                    break;
                case EVENT_DETONATION:
                    DoCastAOE(SPELL_DETONATION);
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
    bool _healthAbove50Pct;
};

void AddSC_boss_arcanist_doan()
{
    RegisterScarletMonasteryCreatureAI(boss_arcanist_doan);
}


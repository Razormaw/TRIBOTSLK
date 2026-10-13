/*
 * TRIBOTSLK - Monasterio Escarlata: Interrogador Vishas
 * Script con diálogos y audios 100% originales verificados.
 */
#include "scarlet_monastery.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum InterrogatorVishasSays
{
    SAY_AGGRO       = 0,
    SAY_HEALTH1     = 1,
    SAY_HEALTH2     = 2,
    SAY_KILL        = 3,
    SAY_TRIGGER_VORREL = 0 // Usado por Vorrel al morir Vishas
};

enum InterrogatorVishasSpells
{
    SPELL_SHADOW_WORD_PAIN = 2767
};

enum InterrogatorVishasEvents
{
    EVENT_SHADOW_WORD_PAIN = 1
};

struct boss_interrogator_vishas : public BossAI
{
    boss_interrogator_vishas(Creature* creature) : BossAI(creature, DATA_INTERROGATOR_VISHAS), _yellCount(0) { }

    void Reset() override
    {
        _yellCount = 0;
        _Reset();
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_AGGRO);
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(EVENT_SHADOW_WORD_PAIN, 5s, 10s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        
        // Desencadena el diálogo de Vorrel al morir Vishas (parte del evento de la Catedral)
        if (Creature* vorrel = instance->GetCreature(DATA_VORREL))
        {
            if (vorrel->IsAlive())
                vorrel->AI()->Talk(SAY_TRIGGER_VORREL);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (me->HealthBelowPctDamaged(60, damage) && _yellCount < 1)
        {
            Talk(SAY_HEALTH1);
            ++_yellCount;
        }

        if (me->HealthBelowPctDamaged(30, damage) && _yellCount < 2)
        {
            Talk(SAY_HEALTH2);
            ++_yellCount;
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_SHADOW_WORD_PAIN:
                DoCastVictim(SPELL_SHADOW_WORD_PAIN);
                events.Repeat(5s, 10s);
                break;
            default:
                break;
        }
    }

private:
    uint8 _yellCount;
};

void AddSC_boss_interrogator_vishas()
{
    RegisterScarletMonasteryCreatureAI(boss_interrogator_vishas);
}


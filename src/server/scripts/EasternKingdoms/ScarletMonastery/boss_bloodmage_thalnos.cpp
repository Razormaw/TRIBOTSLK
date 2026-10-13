/*
 * TRIBOTSLK - Monasterio Escarlata: Bloodmage Thalnos
 * Script mejorado con fases interactivas, invocación de adds y diálogos con audio original verificado.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

enum BloodmageThalnosYells
{
    SAY_AGGRO  = 0,
    SAY_HEALTH = 1,
    SAY_KILL   = 2,
    SAY_DEATH  = 3
};

enum BloodmageThalnosSpells
{
    SPELL_FLAMESHOCK      = 8053,
    SPELL_SHADOWBOLT      = 1106,
    SPELL_FLAMESPIKE      = 8814,
    SPELL_FIRENOVA        = 16079,
    SPELL_SUMMON_SKELETON = 11012 // Hechizo genérico de invocación de esqueletos
};

enum BloodmageThalnosEvents
{
    EVENT_FLAME_SHOCK = 1,
    EVENT_SHADOW_BOLT,
    EVENT_FLAME_SPIKE,
    EVENT_FIRE_NOVA,
    EVENT_CHECK_HEALTH
};

struct boss_bloodmage_thalnos : public BossAI
{
    boss_bloodmage_thalnos(Creature* creature) : BossAI(creature, DATA_BLOODMAGE_THALNOS), _hpYell(false), _addsSummoned(false) { }

    void Reset() override
    {
        _hpYell = false;
        _addsSummoned = false;
        _Reset();
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_AGGRO);
        BossAI::JustEngagedWith(who);
        
        events.ScheduleEvent(EVENT_FLAME_SHOCK, 10s, 15s);
        events.ScheduleEvent(EVENT_SHADOW_BOLT, 2s, 4s);
        events.ScheduleEvent(EVENT_FLAME_SPIKE, 8s, 12s);
        events.ScheduleEvent(EVENT_FIRE_NOVA, 40s, 45s);
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
                case EVENT_FLAME_SHOCK:
                    DoCastVictim(SPELL_FLAMESHOCK);
                    events.Repeat(10s, 15s);
                    break;
                case EVENT_SHADOW_BOLT:
                    DoCastVictim(SPELL_SHADOWBOLT);
                    events.Repeat(2s, 4s);
                    break;
                case EVENT_FLAME_SPIKE:
                    DoCastVictim(SPELL_FLAMESPIKE);
                    events.Repeat(30s, 35s);
                    break;
                case EVENT_FIRE_NOVA:
                    DoCastAOE(SPELL_FIRENOVA); // Corregido a AOE para que afecte a todos los jugadores
                    events.Repeat(40s, 45s);
                    break;
                case EVENT_CHECK_HEALTH:
                    if (!_hpYell && HealthBelowPct(35))
                    {
                        _hpYell = true;
                        Talk(SAY_HEALTH);
                        
                        // Fase interactiva: Invocar adds al bajar del 35% de vida
                        if (!_addsSummoned)
                        {
                            _addsSummoned = true;
                            me->SummonCreature(1538, me->GetPositionX() + 2.0f, me->GetPositionY() + 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30s);
                            me->SummonCreature(1538, me->GetPositionX() - 2.0f, me->GetPositionY() - 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30s);
                        }
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
    bool _hpYell;
    bool _addsSummoned;
};

void AddSC_boss_bloodmage_thalnos()
{
    RegisterScarletMonasteryCreatureAI(boss_bloodmage_thalnos);
}


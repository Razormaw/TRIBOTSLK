/*
 * TRIBOTSLK - Cavernas de Brazanegra (Blackfathom Deeps, map 48)
 * Jefes Clásicos con Estrategia Real: Fases, Adds, CC y Dinámica de Grupo
 * API TrinityCore 3.3.5a archivada
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackfathom_deeps.h"

using namespace std::chrono_literals;

// ================= 1) GHAMOO-RA (El Tanque Bestial) =================
enum GhamooRaSpells
{
    SPELL_GHAMOO_RA_THUNDER_CLAP = 16145,
    SPELL_GHAMOO_RA_SHELL_SPIN   = 16146,
    SPELL_GHAMOO_RA_ENRAGE       = 8599
};
enum GhamooRaEvents 
{ 
    EVENT_GHAMOO_RA_THUNDER_CLAP = 1, 
    EVENT_GHAMOO_RA_SHELL_SPIN
};
enum GhamooRaTexts { SAY_GHAMOO_RA_AGGRO = 0, SAY_GHAMOO_RA_PHASE2 = 1, SAY_GHAMOO_RA_DEATH = 2 };

struct boss_ghamoo_ra : public BossAI
{
    boss_ghamoo_ra(Creature* creature) : BossAI(creature, DATA_GHAMOO_RA), _phase2(false) { }

    void Reset() override 
    { 
        _Reset(); 
        _phase2 = false; 
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_GHAMOO_RA_AGGRO);
        events.ScheduleEvent(EVENT_GHAMOO_RA_THUNDER_CLAP, 8s, 12s);
        events.ScheduleEvent(EVENT_GHAMOO_RA_SHELL_SPIN, 15s, 20s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_phase2 && me->HealthBelowPct(60))
        {
            _phase2 = true;
            Talk(SAY_GHAMOO_RA_PHASE2);
            SummonAdds(2);
        }
        
        if (me->HealthBelowPct(20) && !me->HasAura(SPELL_GHAMOO_RA_ENRAGE))
        {
            DoCastSelf(SPELL_GHAMOO_RA_ENRAGE);
            SummonAdds(3);
        }
    }

    void SummonAdds(uint8 count)
    {
        float x = me->GetPositionX();
        float y = me->GetPositionY();
        float z = me->GetPositionZ();
        
        for (uint8 i = 0; i < count; ++i)
        {
            me->SummonCreature(4830, x + frand(-3.0f, 3.0f), y + frand(-3.0f, 3.0f), z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 45000ms);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_GHAMOO_RA_DEATH);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_GHAMOO_RA_THUNDER_CLAP:
                    DoCastAOE(SPELL_GHAMOO_RA_THUNDER_CLAP);
                    events.ScheduleEvent(EVENT_GHAMOO_RA_THUNDER_CLAP, 10s, 14s);
                    break;
                case EVENT_GHAMOO_RA_SHELL_SPIN:
                    DoCastAOE(SPELL_GHAMOO_RA_SHELL_SPIN);
                    events.ScheduleEvent(EVENT_GHAMOO_RA_SHELL_SPIN, 18s, 25s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _phase2;
};

// ================= 2) LADY SAREVESS (La Controladora de Hielo) =================
enum SarevessSpells
{
    SPELL_SAREVESS_FROSTBOLT      = 15044,
    SPELL_SAREVESS_FROST_NOVA     = 15531,
    SPELL_SAREVESS_BLIZZARD       = 15488,
    SPELL_SAREVESS_HEAL           = 11642,
    SPELL_SAREVESS_ENRAGE         = 8599
};
enum SarevessEvents { EVENT_SAREVESS_FROSTBOLT = 1, EVENT_SAREVESS_FROST_NOVA, EVENT_SAREVESS_BLIZZARD, EVENT_SAREVESS_HEAL };
enum SarevessTexts { SAY_SAREVESS_AGGRO = 0, SAY_SAREVESS_PHASE2 = 1, SAY_SAREVESS_DEATH = 2 };

struct boss_lady_sarevess : public BossAI
{
    boss_lady_sarevess(Creature* creature) : BossAI(creature, DATA_SAREVESS), _phase2(false) { }

    void Reset() override 
    { 
        _Reset(); 
        _phase2 = false; 
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_SAREVESS_AGGRO);
        events.ScheduleEvent(EVENT_SAREVESS_FROSTBOLT, 3s, 5s);
        events.ScheduleEvent(EVENT_SAREVESS_FROST_NOVA, 12s, 16s);
        events.ScheduleEvent(EVENT_SAREVESS_HEAL, 20s, 25s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_phase2 && me->HealthBelowPct(50))
        {
            _phase2 = true;
            Talk(SAY_SAREVESS_PHASE2);
            events.ScheduleEvent(EVENT_SAREVESS_BLIZZARD, 5s);
            
            float x = me->GetPositionX();
            float y = me->GetPositionY();
            float z = me->GetPositionZ();
            
            me->SummonCreature(4832, x + 6.0f, y + 2.0f, z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000ms);
            me->SummonCreature(4832, x + 6.0f, y - 2.0f, z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000ms);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_SAREVESS_DEATH);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SAREVESS_FROSTBOLT:
                    DoCastVictim(SPELL_SAREVESS_FROSTBOLT);
                    events.ScheduleEvent(EVENT_SAREVESS_FROSTBOLT, 4s, 6s);
                    break;
                case EVENT_SAREVESS_FROST_NOVA:
                    DoCastAOE(SPELL_SAREVESS_FROST_NOVA);
                    events.ScheduleEvent(EVENT_SAREVESS_FROST_NOVA, 15s, 20s);
                    break;
                case EVENT_SAREVESS_BLIZZARD:
                    if (_phase2)
                    {
                        DoCastAOE(SPELL_SAREVESS_BLIZZARD);
                        events.ScheduleEvent(EVENT_SAREVESS_BLIZZARD, 20s, 25s);
                    }
                    break;
                case EVENT_SAREVESS_HEAL:
                    if (me->HealthBelowPct(70))
                    {
                        DoCastSelf(SPELL_SAREVESS_HEAL);
                        events.ScheduleEvent(EVENT_SAREVESS_HEAL, 20s, 25s);
                    }
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _phase2;
};

// ================= 3) OLD SERRA'KIS (La Hidra Venenosa) =================
enum SerraKisSpells
{
    SPELL_SERRAKIS_POISON_BOLT = 7378,
    SPELL_SERRAKIS_THRASH      = 3391,
    SPELL_SERRAKIS_TOXIC_SPIT  = 51646
};
enum SerraKisEvents { EVENT_SERRAKIS_POISON_BOLT = 1, EVENT_SERRAKIS_THRASH, EVENT_SERRAKIS_TOXIC_SPIT };
enum SerraKisTexts { SAY_SERRAKIS_AGGRO = 0, SAY_SERRAKIS_LOST_HEAD = 1, SAY_SERRAKIS_DEATH = 2 };

struct boss_old_serra_kis : public BossAI
{
    boss_old_serra_kis(Creature* creature) : BossAI(creature, DATA_SERRAKIS), _headsLost(0) { }

    void Reset() override 
    { 
        _Reset(); 
        _headsLost = 0; 
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_SERRAKIS_AGGRO);
        events.ScheduleEvent(EVENT_SERRAKIS_POISON_BOLT, 4s, 6s);
        events.ScheduleEvent(EVENT_SERRAKIS_THRASH, 8s, 12s);
        events.ScheduleEvent(EVENT_SERRAKIS_TOXIC_SPIT, 15s, 20s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (me->HealthBelowPct(66) && _headsLost == 0)
        {
            _headsLost = 1;
            Talk(SAY_SERRAKIS_LOST_HEAD);
            SummonHead();
        }
        else if (me->HealthBelowPct(33) && _headsLost == 1)
        {
            _headsLost = 2;
            Talk(SAY_SERRAKIS_LOST_HEAD);
            SummonHead();
            SummonHead();
        }
    }

    void SummonHead()
    {
        float x = me->GetPositionX();
        float y = me->GetPositionY();
        float z = me->GetPositionZ();
        
        me->SummonCreature(4833, x + 4.0f, y, z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000ms);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_SERRAKIS_DEATH);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SERRAKIS_POISON_BOLT:
                    DoCastVictim(SPELL_SERRAKIS_POISON_BOLT);
                    events.ScheduleEvent(EVENT_SERRAKIS_POISON_BOLT, 5s, 7s);
                    break;
                case EVENT_SERRAKIS_THRASH:
                    DoCastVictim(SPELL_SERRAKIS_THRASH);
                    events.ScheduleEvent(EVENT_SERRAKIS_THRASH, 10s, 14s);
                    break;
                case EVENT_SERRAKIS_TOXIC_SPIT:
                    DoCastVictim(SPELL_SERRAKIS_TOXIC_SPIT);
                    events.ScheduleEvent(EVENT_SERRAKIS_TOXIC_SPIT, 18s, 22s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    uint8 _headsLost;
};

// ================= 4) LORGUS JETT (El Druida Corrupto) =================
enum LorgusSpells
{
    SPELL_LORGUS_WRATH        = 9739,
    SPELL_LORGUS_MOONFIRE     = 9835,
    SPELL_LORGUS_ENTANGLING   = 33813,
    SPELL_LORGUS_REJUVENATION = 12742,
    SPELL_LORGUS_SUMMON_TREANT = 11354
};
enum LorgusEvents { EVENT_LORGUS_WRATH = 1, EVENT_LORGUS_MOONFIRE, EVENT_LORGUS_ENTANGLING, EVENT_LORGUS_REJUVENATION };
enum LorgusTexts { SAY_LORGUS_AGGRO = 0, SAY_LORGUS_NATURE = 1, SAY_LORGUS_DEATH = 2 };

struct boss_lorgus_jett : public BossAI
{
    boss_lorgus_jett(Creature* creature) : BossAI(creature, DATA_LORGUS), _summoned(false) { }

    void Reset() override 
    { 
        _Reset(); 
        _summoned = false; 
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_LORGUS_AGGRO);
        events.ScheduleEvent(EVENT_LORGUS_WRATH, 3s, 5s);
        events.ScheduleEvent(EVENT_LORGUS_MOONFIRE, 6s, 9s);
        events.ScheduleEvent(EVENT_LORGUS_ENTANGLING, 12s, 16s);
        events.ScheduleEvent(EVENT_LORGUS_REJUVENATION, 20s, 25s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_summoned && me->HealthBelowPct(40))
        {
            _summoned = true;
            Talk(SAY_LORGUS_NATURE);
            
            float x = me->GetPositionX();
            float y = me->GetPositionY();
            float z = me->GetPositionZ();
            
            me->SummonCreature(4834, x + 5.0f, y + 3.0f, z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000ms);
            me->SummonCreature(4834, x + 5.0f, y - 3.0f, z, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000ms);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_LORGUS_DEATH);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_LORGUS_WRATH:
                    DoCastVictim(SPELL_LORGUS_WRATH);
                    events.ScheduleEvent(EVENT_LORGUS_WRATH, 4s, 6s);
                    break;
                case EVENT_LORGUS_MOONFIRE:
                    DoCastVictim(SPELL_LORGUS_MOONFIRE);
                    events.ScheduleEvent(EVENT_LORGUS_MOONFIRE, 8s, 11s);
                    break;
                case EVENT_LORGUS_ENTANGLING:
                    DoCastVictim(SPELL_LORGUS_ENTANGLING);
                    events.ScheduleEvent(EVENT_LORGUS_ENTANGLING, 15s, 20s);
                    break;
                case EVENT_LORGUS_REJUVENATION:
                    if (me->HealthBelowPct(60))
                    {
                        DoCastSelf(SPELL_LORGUS_REJUVENATION);
                        events.ScheduleEvent(EVENT_LORGUS_REJUVENATION, 20s, 25s);
                    }
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _summoned;
};

void AddSC_bfd_classic_bosses()
{
    RegisterCreatureAI(boss_ghamoo_ra);
    RegisterCreatureAI(boss_lady_sarevess);
    RegisterCreatureAI(boss_old_serra_kis);
    RegisterCreatureAI(boss_lorgus_jett);
}


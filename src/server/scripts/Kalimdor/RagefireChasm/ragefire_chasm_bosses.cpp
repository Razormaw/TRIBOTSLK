/*
 * TRIBOTSLK - Sima Ígnea (Ragefire Chasm, map 389)
 * Jefes con rutinas blizzlike: fases, adds, enrage, diálogos
 * Hechizos NPC válidos para WotLK 3.3.5a
 */
#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "SpellInfo.h"

using namespace std::chrono_literals;

// ================= HECHIZOS =================
enum RFCSpells
{
    // Oggleflint
    SPELL_OGGLE_CLEAVE     = 15496,
    SPELL_OGGLE_ENRAGE     = 8599,
    SPELL_OGGLE_FRENZY     = 28798,

    // Taragaman
    SPELL_TARA_FIREBALL    = 12471,
    SPELL_TARA_FLAMESTRIKE = 13847,
    SPELL_TARA_FIRE_SHIELD = 17471,
    SPELL_TARA_ENRAGE      = 8599,

    // Jergosh
    SPELL_JERG_LIGHTNING   = 9532,
    SPELL_JERG_CHAIN       = 10605,
    SPELL_JERG_HEAL        = 12039,
    SPELL_JERG_FRENZY      = 28798,

    // Bazzalan
    SPELL_BAZZ_SHADOW      = 9613,
    SPELL_BAZZ_CURSE       = 702,
    SPELL_BAZZ_FIREBALL    = 12471,

    // Lorthal
    SPELL_LORT_CLEAVE      = 15496,
    SPELL_LORT_KNOCK       = 10101,
    SPELL_LORT_ENRAGE      = 8599
};

// ================= TEXTOS =================
enum RFCTexts
{
    SAY_AGGRO      = 0,
    SAY_DEATH      = 1,
    SAY_SPECIAL    = 2,
    SAY_PHASE_2    = 3,
    SAY_SUMMON     = 4,
    SAY_KILL       = 5
};

// ================= 1) OGGLEFLINT (Trogg bruto) =================
struct boss_oggleflint : public CreatureAI
{
    boss_oggleflint(Creature* c) : CreatureAI(c), _enraged(false), _frenzied(false) { }

    void Reset() override 
    { 
        _events.Reset(); 
        _enraged = false; 
        _frenzied = false; 
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 5s);  // Cleave
        _events.ScheduleEvent(2, 12s); // Frenzy check
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_enraged && me->HealthBelowPct(20))
        {
            _enraged = true;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_OGGLE_ENRAGE);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: // Cleave
                    DoCastVictim(SPELL_OGGLE_CLEAVE);
                    _events.ScheduleEvent(1, 8s);
                    break;
                case 2: // Frenzy check
                    if (!_frenzied && me->HealthBelowPct(30))
                    {
                        _frenzied = true;
                        DoCastSelf(SPELL_OGGLE_FRENZY);
                    }
                    _events.ScheduleEvent(2, 10s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); }
    void KilledUnit(Unit* /*victim*/) override { Talk(SAY_KILL); }

private:
    EventMap _events;
    bool _enraged;
    bool _frenzied;
};

// ================= 2) TARAGAMAN THE HUNGERER (Elemental de fuego) =================
struct boss_taragaman : public CreatureAI
{
    boss_taragaman(Creature* c) : CreatureAI(c), _enraged(false), _phase2(false) { }

    void Reset() override 
    { 
        _events.Reset(); 
        _enraged = false; 
        _phase2 = false; 
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 4s);  // Fireball
        _events.ScheduleEvent(2, 10s); // Flamestrike
        _events.ScheduleEvent(3, 15s); // Fire Shield
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_enraged && me->HealthBelowPct(30))
        {
            _enraged = true;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_TARA_ENRAGE);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        // Fase 2: invoca adds cada 30s
        if (!_phase2 && me->HealthBelowPct(50))
        {
            _phase2 = true;
            Talk(SAY_PHASE_2);
        }

        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: // Fireball
                    DoCastVictim(SPELL_TARA_FIREBALL);
                    _events.ScheduleEvent(1, 6s);
                    break;
                case 2: // Flamestrike
                    DoCastAOE(SPELL_TARA_FLAMESTRIKE);
                    _events.ScheduleEvent(2, 12s);
                    break;
                case 3: // Fire Shield
                    DoCastSelf(SPELL_TARA_FIRE_SHIELD);
                    _events.ScheduleEvent(3, 20s);
                    break;
                case 4: // Summon adds (fase 2)
                    if (_phase2)
                    {
                        Talk(SAY_SUMMON);
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            me->SummonCreature(11522, // Firemaw Trogg
                                me->GetPositionX() + frand(-3.0f, 3.0f),
                                me->GetPositionY() + frand(-3.0f, 3.0f),
                                me->GetPositionZ(), 0.0f,
                                TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30s);
                        }
                    }
                    _events.ScheduleEvent(4, 30s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); }
    void KilledUnit(Unit* /*victim*/) override { Talk(SAY_KILL); }

private:
    EventMap _events;
    bool _enraged;
    bool _phase2;
};

// ================= 3) JERGOSH THE INVOKER (Trogg chamán) =================
struct boss_jergosh : public CreatureAI
{
    boss_jergosh(Creature* c) : CreatureAI(c), _frenzied(false) { }

    void Reset() override 
    { 
        _events.Reset(); 
        _frenzied = false; 
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 3s);  // Lightning Bolt
        _events.ScheduleEvent(2, 8s);  // Chain Lightning
        _events.ScheduleEvent(3, 15s); // Heal check
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: // Lightning Bolt
                    DoCastVictim(SPELL_JERG_LIGHTNING);
                    _events.ScheduleEvent(1, 5s);
                    break;
                case 2: // Chain Lightning
                    DoCastVictim(SPELL_JERG_CHAIN);
                    _events.ScheduleEvent(2, 10s);
                    break;
                case 3: // Heal (solo si < 40%)
                    if (me->HealthBelowPct(40))
                    {
                        DoCastSelf(SPELL_JERG_HEAL);
                        Talk(SAY_SPECIAL);
                    }
                    _events.ScheduleEvent(3, 18s);
                    break;
                case 4: // Frenzy check
                    if (!_frenzied && me->HealthBelowPct(30))
                    {
                        _frenzied = true;
                        DoCastSelf(SPELL_JERG_FRENZY);
                    }
                    _events.ScheduleEvent(4, 10s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); }
    void KilledUnit(Unit* /*victim*/) override { Talk(SAY_KILL); }

private:
    EventMap _events;
    bool _frenzied;
};

// ================= 4) BAZZALAN (Trogg brujo) =================
struct boss_bazzalan : public CreatureAI
{
    boss_bazzalan(Creature* c) : CreatureAI(c), _summoned(false) { }

    void Reset() override 
    { 
        _events.Reset(); 
        _summoned = false; 
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 4s);  // Shadow Bolt
        _events.ScheduleEvent(2, 9s);  // Curse of Weakness
        _events.ScheduleEvent(3, 14s); // Fireball
        _events.ScheduleEvent(4, 25s); // Summon Imp
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: // Shadow Bolt
                    DoCastVictim(SPELL_BAZZ_SHADOW);
                    _events.ScheduleEvent(1, 6s);
                    break;
                case 2: // Curse of Weakness
                    DoCastVictim(SPELL_BAZZ_CURSE);
                    _events.ScheduleEvent(2, 12s);
                    break;
                case 3: // Fireball
                    DoCastVictim(SPELL_BAZZ_FIREBALL);
                    _events.ScheduleEvent(3, 10s);
                    break;
                case 4: // Summon Imp (a 50% de vida)
                    if (!_summoned && me->HealthBelowPct(50))
                    {
                        _summoned = true;
                        Talk(SAY_SUMMON);
                        me->SummonCreature(416, // Imp
                            me->GetPositionX() + frand(-2.0f, 2.0f),
                            me->GetPositionY() + frand(-2.0f, 2.0f),
                            me->GetPositionZ(), 0.0f,
                            TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60s);
                    }
                    _events.ScheduleEvent(4, 30s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); }
    void KilledUnit(Unit* /*victim*/) override { Talk(SAY_KILL); }

private:
    EventMap _events;
    bool _summoned;
};

// ================= 5) LORTHAL (Jefe final trogg) =================
struct boss_lorthal : public CreatureAI
{
    boss_lorthal(Creature* c) : CreatureAI(c), _enraged(false), _summonedAdds(false) { }

    void Reset() override 
    { 
        _events.Reset(); 
        _enraged = false; 
        _summonedAdds = false; 
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 5s);  // Cleave
        _events.ScheduleEvent(2, 12s); // Knock Away
        _events.ScheduleEvent(3, 20s); // Summon adds
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_enraged && me->HealthBelowPct(25))
        {
            _enraged = true;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_LORT_ENRAGE);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: // Cleave
                    DoCastVictim(SPELL_LORT_CLEAVE);
                    _events.ScheduleEvent(1, 8s);
                    break;
                case 2: // Knock Away
                    DoCastVictim(SPELL_LORT_KNOCK);
                    _events.ScheduleEvent(2, 15s);
                    break;
                case 3: // Summon adds (a 60% de vida)
                    if (!_summonedAdds && me->HealthBelowPct(60))
                    {
                        _summonedAdds = true;
                        Talk(SAY_PHASE_2);
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            me->SummonCreature(11517, // Trogg
                                me->GetPositionX() + frand(-3.0f, 3.0f),
                                me->GetPositionY() + frand(-3.0f, 3.0f),
                                me->GetPositionZ(), 0.0f,
                                TEMPSUMMON_CORPSE_TIMED_DESPAWN, 45s);
                        }
                    }
                    _events.ScheduleEvent(3, 45s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); }
    void KilledUnit(Unit* /*victim*/) override { Talk(SAY_KILL); }

private:
    EventMap _events;
    bool _enraged;
    bool _summonedAdds;
};

void AddSC_ragefire_chasm_bosses()
{
    RegisterCreatureAI(boss_oggleflint);
    RegisterCreatureAI(boss_taragaman);
    RegisterCreatureAI(boss_jergosh);
    RegisterCreatureAI(boss_bazzalan);
    RegisterCreatureAI(boss_lorthal);
}


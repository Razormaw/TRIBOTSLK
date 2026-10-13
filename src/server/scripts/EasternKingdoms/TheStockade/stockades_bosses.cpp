/*
 * TRIBOTSLK - La Cárcel (The Stockades, map 34)
 * VERSIÓN FINAL: Todos los jefes con Fases 2 dinámicas.
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_stockades.h"

using namespace std::chrono_literals;

// ================= 1) TARGORR THE DREAD (Fase 2: Furia y Torbellino) =================
struct boss_targorr : public BossAI
{
    boss_targorr(Creature* c) : BossAI(c, DATA_TARGORR), _phase(1) { }

    void Reset() override { _Reset(); _phase = 1; }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        events.ScheduleEvent(SPELL_CLEAVE, 6s, 9s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (_phase == 1 && me->HealthBelowPct(50))
        {
            _phase = 2;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_ENRAGE);
            events.ScheduleEvent(SPELL_WHIRLWIND, 2s);
        }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case SPELL_CLEAVE:
                    DoCastVictim(SPELL_CLEAVE);
                    events.ScheduleEvent(SPELL_CLEAVE, _phase == 1 ? 8s : 4s);
                    break;
                case SPELL_WHIRLWIND:
                    if (_phase == 2)
                    {
                        DoCastAOE(SPELL_WHIRLWIND);
                        events.ScheduleEvent(SPELL_WHIRLWIND, 6s, 10s);
                    }
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

// ================= 2) KAM DEEPFURY (Fase 2: Lluvia de Sombras) =================
struct boss_kam_deepfury : public BossAI
{
    boss_kam_deepfury(Creature* c) : BossAI(c, DATA_KAM_DEEPFURY), _phase(1), _phase2Started(false) { }

    void Reset() override { _Reset(); _phase = 1; _phase2Started = false; }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        events.ScheduleEvent(SPELL_SHADOW_WORD_PAIN, 4s, 7s);
        events.ScheduleEvent(SPELL_MIND_BLAST, 8s, 11s);
        events.ScheduleEvent(SPELL_GREATER_HEAL, 12s, 18s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (_phase == 1 && me->HealthBelowPct(40))
        {
            _phase = 2;
            Talk(SAY_SPECIAL);
        }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        // Activar spameo de fase 2 una sola vez
        if (_phase == 2 && !_phase2Started)
        {
            _phase2Started = true;
            events.ScheduleEvent(SPELL_SHADOW_BOLT_VOLLEY, 1s);
        }

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case SPELL_SHADOW_WORD_PAIN:
                    DoCastVictim(SPELL_SHADOW_WORD_PAIN);
                    events.ScheduleEvent(SPELL_SHADOW_WORD_PAIN, 12s);
                    break;
                case SPELL_MIND_BLAST:
                    DoCastVictim(SPELL_MIND_BLAST);
                    events.ScheduleEvent(SPELL_MIND_BLAST, 8s);
                    break;
                case SPELL_GREATER_HEAL:
                    if (_phase == 1 && me->HealthBelowPct(60))
                        DoCastSelf(SPELL_GREATER_HEAL);
                    events.ScheduleEvent(SPELL_GREATER_HEAL, 15s);
                    break;
                case SPELL_SHADOW_BOLT_VOLLEY:
                    if (_phase == 2)
                    {
                        DoCastAOE(SPELL_SHADOW_BOLT_VOLLEY);
                        events.ScheduleEvent(SPELL_SHADOW_BOLT_VOLLEY, 3s, 5s);
                    }
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
    bool _phase2Started;
};

// ================= 3) HAMHOCK (Fase 2: Llamada de Refuerzos) =================
struct boss_hamhock : public BossAI
{
    boss_hamhock(Creature* c) : BossAI(c, DATA_HAMHOCK), _phase(1) { }

    void Reset() override { _Reset(); _phase = 1; }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        events.ScheduleEvent(SPELL_MORTAL_STRIKE, 6s, 10s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (_phase == 1 && me->HealthBelowPct(50))
        {
            _phase = 2;
            Talk(SAY_SPECIAL);
            
            me->SummonCreature(NPC_DEFIAS_SQUATTER, me->GetPositionX() + 3.0f, me->GetPositionY() + 3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            me->SummonCreature(NPC_DEFIAS_SQUATTER, me->GetPositionX() - 3.0f, me->GetPositionY() - 3.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            
            DoCastSelf(SPELL_ENRAGE);
        }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case SPELL_MORTAL_STRIKE:
                    DoCastVictim(SPELL_MORTAL_STRIKE);
                    events.ScheduleEvent(SPELL_MORTAL_STRIKE, _phase == 1 ? 9s : 5s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

// ================= 4) DEXTREN WARD (Fase 2: Emboscada Letal) =================
struct boss_dextren_ward : public BossAI
{
    boss_dextren_ward(Creature* c) : BossAI(c, DATA_DEXTREN_WARD), _phase(1), _phase2Started(false) { }

    void Reset() override { _Reset(); _phase = 1; _phase2Started = false; }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        DoCastSelf(SPELL_SLICE_AND_DICE);
        events.ScheduleEvent(SPELL_SINISTER_STRIKE, 4s, 6s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (_phase == 1 && me->HealthBelowPct(40))
        {
            _phase = 2;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_ENRAGE);
        }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        // Activar eventos de fase 2 una sola vez
        if (_phase == 2 && !_phase2Started)
        {
            _phase2Started = true;
            events.ScheduleEvent(SPELL_GOUGE, 5s);
            events.ScheduleEvent(SPELL_KIDNEY_SHOT, 10s);
        }

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case SPELL_SINISTER_STRIKE:
                    DoCastVictim(SPELL_SINISTER_STRIKE);
                    events.ScheduleEvent(SPELL_SINISTER_STRIKE, 3s);
                    break;
                case SPELL_GOUGE:
                    if (_phase == 2)
                    {
                        DoCastVictim(SPELL_GOUGE);
                        events.ScheduleEvent(SPELL_GOUGE, 8s, 12s);
                    }
                    break;
                case SPELL_KIDNEY_SHOT:
                    if (_phase == 2)
                    {
                        DoCastVictim(SPELL_KIDNEY_SHOT);
                        events.ScheduleEvent(SPELL_KIDNEY_SHOT, 15s, 20s);
                    }
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
    bool _phase2Started;
};

// ================= 5) BAZIL THREDD (Fase 2: Lluvia de Cuchillas y Esbirros) =================
struct boss_bazil_thredd : public BossAI
{
    boss_bazil_thredd(Creature* c) : BossAI(c, DATA_BAZIL_THREDD), _phase(1) { }

    void Reset() override { _Reset(); _phase = 1; }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        DoCastSelf(SPELL_BLADE_FLURRY);
        events.ScheduleEvent(SPELL_WOUND_POISON, 7s, 10s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (_phase == 1 && me->HealthBelowPct(50))
        {
            _phase = 2;
            Talk(SAY_SPECIAL);
            
            me->SummonCreature(NPC_DEFIAS_EVOKER, me->GetPositionX() + 2.0f, me->GetPositionY() + 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            me->SummonCreature(NPC_DEFIAS_EVOKER, me->GetPositionX() - 2.0f, me->GetPositionY() - 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0ms);
            
            DoCastSelf(SPELL_ENRAGE);
        }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case SPELL_WOUND_POISON:
                    DoCastVictim(SPELL_WOUND_POISON);
                    events.ScheduleEvent(SPELL_WOUND_POISON, _phase == 1 ? 9s : 4s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

// ================= 6) BRUEGAL IRONKNUCKLE (Jefe final con Refuerzos) =================
struct boss_bruegal_ironknuckle : public BossAI
{
    boss_bruegal_ironknuckle(Creature* c) : BossAI(c, DATA_BRUEGAL_IRONKNUCKLE), _adds(false), _enraged(false) { }

    void Reset() override { _Reset(); _adds = false; _enraged = false; }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        events.ScheduleEvent(SPELL_HAMMER_JUSTICE, 9s, 14s);
        events.ScheduleEvent(SPELL_SHIELD_BASH, 6s, 10s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_adds && me->HealthBelowPct(50))
        {
            _adds = true;
            Talk(SAY_SPECIAL);
            me->SummonCreature(NPC_DEFIAS_INSURGENT, me->GetPositionX() + 2.0f, me->GetPositionY() + 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000ms);
            me->SummonCreature(NPC_DEFIAS_INSURGENT, me->GetPositionX() - 2.0f, me->GetPositionY() - 2.0f, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000ms);
        }
        if (!_enraged && me->HealthBelowPct(20))
        {
            _enraged = true;
            DoCastSelf(SPELL_ENRAGE);
        }
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case SPELL_HAMMER_JUSTICE:
                    DoCastVictim(SPELL_HAMMER_JUSTICE);
                    events.ScheduleEvent(SPELL_HAMMER_JUSTICE, 14s, 20s);
                    break;
                case SPELL_SHIELD_BASH:
                    DoCastVictim(SPELL_SHIELD_BASH);
                    events.ScheduleEvent(SPELL_SHIELD_BASH, 8s, 12s);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

private:
    bool _adds;
    bool _enraged;
};

void AddSC_the_stockades_bosses()
{
    RegisterCreatureAI(boss_targorr);
    RegisterCreatureAI(boss_kam_deepfury);
    RegisterCreatureAI(boss_hamhock);
    RegisterCreatureAI(boss_dextren_ward);
    RegisterCreatureAI(boss_bazil_thredd);
    RegisterCreatureAI(boss_bruegal_ironknuckle);
}


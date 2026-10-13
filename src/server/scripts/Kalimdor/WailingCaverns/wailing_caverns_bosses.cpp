/*
 * TRIBOTSLK - Cuevas de los Lamentos (map 43)
 * API TrinityCore moderna. 7 jefes: 4 druidas corruptos, Skum, Verdan y Mutanus.
 */
#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "Player.h"
#include "SpellInfo.h"
#include "Map.h"
#include "TemporarySummon.h"

#include <chrono>
using namespace std::chrono_literals;

enum eNPCs
{
    NPC_TREANT = 5782
};

enum eSpells
{
    SPELL_ENRAGE       = 8599,
    SPELL_POISON_BOLT  = 13443,
    SPELL_VENOM        = 13443,
    SPELL_WRATH        = 9739,
    SPELL_MOONFIRE     = 9835,
    SPELL_REJUVENATION = 12742,
    SPELL_THRASH       = 3391,
    SPELL_CHARGE       = 6268,
    SPELL_CLEAVE       = 15496,
    SPELL_ROOTS        = 26989,
    SPELL_FEAR         = 29321,
    SPELL_HEAL         = 11642,
    SPELL_SUMMON_TREANT = 11968
};

enum eTexts { SAY_AGGRO = 0, SAY_DEATH = 1, SAY_SPECIAL = 2 };

// ================= 1) LORD COBRAHN (druida serpiente, veneno) =================
struct boss_lord_cobrahn : public CreatureAI
{
    boss_lord_cobrahn(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 4000ms);
        _events.ScheduleEvent(2, 10000ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: DoCastVictim(SPELL_POISON_BOLT);  _events.ScheduleEvent(1, 6000ms);  break;
                case 2: DoCastVictim(SPELL_VENOM);        _events.ScheduleEvent(2, 12000ms); break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
};

// ================= 2) LADY ANACONDRA (druida serpiente, naturaleza) =================
struct boss_lady_anacondra : public CreatureAI
{
    boss_lady_anacondra(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 5000ms);
        _events.ScheduleEvent(2, 12000ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: DoCastVictim(SPELL_WRATH);        _events.ScheduleEvent(1, 7000ms);  break;
                case 2:
                    if (me->HealthBelowPct(50))
                        DoCastSelf(SPELL_REJUVENATION);
                    _events.ScheduleEvent(2, 15000ms);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
};

// ================= 3) LORD PYTHAS (druida serpiente, fuego lunar) =================
struct boss_lord_pythas : public CreatureAI
{
    boss_lord_pythas(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 4000ms);
        _events.ScheduleEvent(2, 9000ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: DoCastVictim(SPELL_MOONFIRE);     _events.ScheduleEvent(1, 8000ms);  break;
                case 2: DoCastVictim(SPELL_POISON_BOLT);  _events.ScheduleEvent(2, 10000ms); break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
};

// ================= 4) LORD SERPENTIS (druida serpiente, enrage) =================
struct boss_lord_serpentis : public CreatureAI
{
    boss_lord_serpentis(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); _enraged = false; }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 5000ms);
        _events.ScheduleEvent(2, 11000ms);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_enraged && me->HealthBelowPct(30))
        {
            _enraged = true;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_ENRAGE);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: DoCastVictim(SPELL_VENOM);   _events.ScheduleEvent(1, 9000ms);  break;
                case 2: DoCastSelf(SPELL_THRASH);    _events.ScheduleEvent(2, 13000ms); break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
    bool _enraged = false;
};

// ================= 5) SKUM (bestia gigante, embestida) =================
struct boss_skum : public CreatureAI
{
    boss_skum(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 6000ms);
        _events.ScheduleEvent(2, 12000ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1:
                    if (me->GetDistance(me->GetVictim()) > 8.0f)
                        DoCastVictim(SPELL_CHARGE);
                    _events.ScheduleEvent(1, 10000ms);
                    break;
                case 2: DoCastVictim(SPELL_CLEAVE);    _events.ScheduleEvent(2, 14000ms); break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
};

// ================= 6) VERDAN THE EVERLIVING (elemental, raíces + adds) =================
struct boss_verdan : public CreatureAI
{
    boss_verdan(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); _adds = false; }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 5000ms);
        _events.ScheduleEvent(2, 15000ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: DoCastVictim(SPELL_ROOTS);       _events.ScheduleEvent(1, 12000ms); break;
                case 2:
                    if (!_adds)
                    {
                        _adds = true;
                        Talk(SAY_SPECIAL);
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            me->SummonCreature(NPC_TREANT,
                                me->GetPositionX() + irand(-3, 3),
                                me->GetPositionY() + irand(-3, 3),
                                me->GetPositionZ(), 0.0f,
                                TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000ms);
                        }
                    }
                    _events.ScheduleEvent(2, 20000ms);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
    bool _adds = false;
};

// ================= 7) MUTANUS THE DEVOURER (jefe final, miedo + veneno mortal) =================
struct boss_mutanus : public CreatureAI
{
    boss_mutanus(Creature* c) : CreatureAI(c) { }

    void Reset() override { _events.Reset(); _enraged = false; }

    void JustEngagedWith(Unit*) override
    {
        Talk(SAY_AGGRO);
        _events.ScheduleEvent(1, 7000ms);
        _events.ScheduleEvent(2, 10000ms);
        _events.ScheduleEvent(3, 18000ms);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override
    {
        if (!_enraged && me->HealthBelowPct(25))
        {
            _enraged = true;
            Talk(SAY_SPECIAL);
            DoCastSelf(SPELL_ENRAGE);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;
        _events.Update(diff);
        while (uint32 id = _events.ExecuteEvent())
        {
            switch (id)
            {
                case 1: DoCastVictim(SPELL_FEAR);        _events.ScheduleEvent(1, 15000ms); break;
                case 2: DoCastVictim(SPELL_VENOM);       _events.ScheduleEvent(2, 12000ms); break;
                case 3:
                    if (me->HealthBelowPct(40))
                        DoCastSelf(SPELL_HEAL);
                    _events.ScheduleEvent(3, 20000ms);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit*) override { Talk(SAY_DEATH); }

private:
    EventMap _events;
    bool _enraged = false;
};

void AddSC_wailing_caverns_bosses()
{
    RegisterCreatureAI(boss_lord_cobrahn);
    RegisterCreatureAI(boss_lady_anacondra);
    RegisterCreatureAI(boss_lord_pythas);
    RegisterCreatureAI(boss_lord_serpentis);
    RegisterCreatureAI(boss_skum);
    RegisterCreatureAI(boss_verdan);
    RegisterCreatureAI(boss_mutanus);
}


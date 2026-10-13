/*
 * Lord Vyletongue - Maraudon
 * Entry: 12225
 * TrinityCore 3.3.5a - TRIBOTSLK Epic Heroic Edition (Fixed Phase Gate)
 */

#include "ScriptPCH.h"
#include "maraudon.h"

enum Spells
{
    SPELL_SHADOW_STRIKE         = 16145, // Teletransporte e impacto de sombras
    SPELL_POISON_CLOUD          = 21853, // Nube de gas venenosos
    SPELL_SHADOW_BOLT           = 21025, // Descarga de sombras de los clones
    SPELL_VANISH_VISUAL         = 24222, // Humo de desvanecimiento
    SPELL_BOSS_FRENZY           = 21916  // ¡ID oficial de tu BD sin chocar con el core!
};


enum VyletongueTexts
{
    SAY_AGGRO       = 0,
    SAY_CLONES      = 1,
    SAY_FRENZY      = 2,
    SAY_KILL        = 3,
    SAY_DEATH       = 4
};

enum VyletongueEvents
{
    EVENT_SHADOW_STRIKE = 1,
    EVENT_POISON_CLOUD  = 2,
    EVENT_CHECK_CLONES  = 3
};

enum BossPhases
{
    PHASE_NORMAL        = 1,
    PHASE_MIRROR_SHADOW = 2,
    PHASE_FINAL_EXECUTE = 3
};

enum CustomAdds
{
    NPC_VYLETONGUE_SHADOW_CLONE = 13448 
};

struct boss_vyletongue : public BossAI
{
    boss_vyletongue(Creature* creature) : BossAI(creature, DATA_VYLETONGUE)
    {
        _phase = PHASE_NORMAL;
    }

    void Reset() override
    {
        BossAI::Reset();
        _phase = PHASE_NORMAL;
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetVisible(true);
        summons.DespawnAll();
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        
        events.ScheduleEvent(EVENT_SHADOW_STRIKE, Seconds(5));
        events.ScheduleEvent(EVENT_POISON_CLOUD,  Seconds(10));
    }

    void JustDied(Unit* killer) override
    {
        Talk(SAY_DEATH);
        summons.DespawnAll();
        BossAI::JustDied(killer);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(SAY_KILL);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
            summon->AI()->AttackStart(target);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    void EnterMirrorPhase()
    {
        _phase = PHASE_MIRROR_SHADOW;
        Talk(SAY_CLONES);
        
        me->AttackStop();
        me->SetImmuneToAll(true); // Bloqueo de daño para proteger la fase
        me->SetReactState(REACT_PASSIVE);
        me->SetVisible(false); 
        DoCastSelf(SPELL_VANISH_VISUAL);

        // Invocación segura de clones usando el sistema de dispersión nativa de tu Core
        for (uint8 i = 0; i < 2; ++i)
        {
            me->SummonCreature(NPC_VYLETONGUE_SHADOW_CLONE, me->GetPositionX() + urand(-6, 6), me->GetPositionY() + urand(-6, 6), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, Milliseconds(45000));
        }

        events.ScheduleEvent(EVENT_CHECK_CLONES, Seconds(1));
    }

    void ExitMirrorPhase()
    {
        _phase = PHASE_FINAL_EXECUTE;
        Talk(SAY_FRENZY);
        
        me->SetVisible(true);
        me->SetImmuneToAll(false); // Removemos la inmunidad
        me->SetReactState(REACT_AGGRESSIVE);
        
        if (me->GetVictim())
            me->GetMotionMaster()->MoveChase(me->GetVictim());

        DoCastSelf(SPELL_BOSS_FRENZY); // Poción de rabia final

        events.RescheduleEvent(EVENT_SHADOW_STRIKE, Seconds(2));
        events.RescheduleEvent(EVENT_POISON_CLOUD, Seconds(5));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        // GATILLO DE FASE SEGURO: Si la vida cae del 50% en cualquier momento, salta a los clones
        if (_phase == PHASE_NORMAL && me->GetHealthPct() <= 50.0f)
        {
            EnterMirrorPhase();
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

                while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHADOW_STRIKE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 35.0f, true))
                        DoCast(target, SPELL_SHADOW_STRIKE);
                    else if (me->GetVictim())
                        DoCastVictim(SPELL_SHADOW_STRIKE);

                    events.ScheduleEvent(EVENT_SHADOW_STRIKE, Seconds(_phase == PHASE_FINAL_EXECUTE ? 7 : 13));
                    break;
                    
                case EVENT_POISON_CLOUD:
                    DoCastSelf(SPELL_POISON_CLOUD);
                    events.ScheduleEvent(EVENT_POISON_CLOUD, Seconds(14)); // <-- ¡Corregido a tiempo fijo de 14 segundos! Eleva la estabilidad a 100%
                    break;

                case EVENT_CHECK_CLONES:
                    if (_phase == PHASE_MIRROR_SHADOW)
                    {
                        if (summons.empty())
                        {
                            ExitMirrorPhase();
                        }
                        else
                        {
                            events.ScheduleEvent(EVENT_CHECK_CLONES, Seconds(1));
                        }
                    }
                    break;
            }
        }
		
        if (_phase != PHASE_MIRROR_SHADOW)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
};

void AddSC_boss_vyletongue()
{
    RegisterCreatureAI(boss_vyletongue);
}


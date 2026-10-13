/*
 * TRIBOTSLK - Monasterio Escarlata: Herod
 * Script con fases, mecánicas y diálogos con audio original verificado.
 */
#include "scarlet_monastery.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptMgr.h"

enum HerodSays
{
    SAY_AGGRO     = 0,
    SAY_WHIRLWIND = 1,
    SAY_ENRAGE    = 2,
    SAY_KILL      = 3,
    EMOTE_ENRAGE  = 4
};

enum HerodSpells
{
    SPELL_RUSHINGCHARGE = 8260,
    SPELL_CLEAVE        = 15496,
    SPELL_WHIRLWIND     = 8989,
    SPELL_FRENZY        = 8269
};

enum HerodNpcs
{
    NPC_SCARLET_TRAINEE = 6575,
    NPC_SCARLET_MYRMIDON = 4295
};

enum HerodEvents
{
    EVENT_CLEAVE = 1,
    EVENT_WHIRLWIND
};

Position const ScarletTraineePos = { 1939.18f, -431.58f, 17.09f, 6.22f };

struct boss_herod : public BossAI
{
    boss_herod(Creature* creature) : BossAI(creature, DATA_HEROD), _enrage(false) { }

    void Reset() override
    {
        _enrage = false;
        _Reset();
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_AGGRO);
        DoCastSelf(SPELL_RUSHINGCHARGE);
        BossAI::JustEngagedWith(who);

        events.ScheduleEvent(EVENT_CLEAVE, 12s);
        events.ScheduleEvent(EVENT_WHIRLWIND, 1min);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();

        for (uint8 itr = 0; itr < 20; ++itr)
        {
            Position randomNearPosition = me->GetRandomPoint(ScarletTraineePos, 5.f);
            randomNearPosition.SetOrientation(ScarletTraineePos.GetOrientation());
            me->SummonCreature(NPC_SCARLET_TRAINEE, randomNearPosition, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 10min);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (!_enrage && me->HealthBelowPctDamaged(30, damage))
        {
            Talk(EMOTE_ENRAGE);
            Talk(SAY_ENRAGE);
            DoCastSelf(SPELL_FRENZY);
            _enrage = true;
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CLEAVE:
                DoCastVictim(SPELL_CLEAVE);
                events.Repeat(12s);
                break;
            case EVENT_WHIRLWIND:
                Talk(SAY_WHIRLWIND);
                DoCastSelf(SPELL_WHIRLWIND); // Corregido a Self, ya que Whirlwind es un aura que golpea alrededor
                events.Repeat(30s);
                break;
            default:
                break;
        }
    }

private:
    bool _enrage;
};

static constexpr uint32 PATH_ESCORT_SCARLET_TRAINEE = 52602;

struct npc_scarlet_trainee : public EscortAI
{
    npc_scarlet_trainee(Creature* creature) : EscortAI(creature)
    {
        _startTimer = urand(1000, 6000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_startTimer && !me->IsInCombat())
        {
            if (_startTimer <= diff)
            {
                LoadPath(PATH_ESCORT_SCARLET_TRAINEE);
                Start(true);
                _startTimer = 0;
            }
            else
                _startTimer -= diff;
        }

        EscortAI::UpdateAI(diff);
    }

private:
    uint32 _startTimer;
};

void AddSC_boss_herod()
{
    RegisterScarletMonasteryCreatureAI(boss_herod);
    RegisterScarletMonasteryCreatureAI(npc_scarlet_trainee);
}


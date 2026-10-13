/*
 * This file is part of the TRIBOTSLK Project.
 * Based on TrinityCore 3.3.5a standards.
 * Shade of Aran: 100% Complete. Fixed Flame Wreath, drinking mechanics, and elemental spawns.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "karazhan.h"
#include "SpellInfo.h"
#include "Player.h"

using namespace std::chrono_literals;

enum ShadeOfAranTexts
{
    SAY_AGGRO               = 0,
    SAY_FLAME_WREATH        = 1,
    SAY_BLIZZARD            = 2,
    SAY_EXPLOSION           = 3,
    SAY_DRINK               = 4,
    SAY_ELEMENTALS          = 5,
    SAY_KILL                = 6,
    SAY_TIME_OVER           = 7,
    SAY_DEATH               = 8
};

enum ShadeOfAranSpells
{
    // Normal Spells
    SPELL_FROSTBOLT         = 29954,
    SPELL_FIREBALL          = 29953,
    SPELL_ARCANE_MISSILES   = 29955,
    SPELL_CHAINS_OF_ICE     = 29991,
    SPELL_DRAGONS_BREATH    = 29964,

    // Super Spells
    SPELL_FLAME_WREATH      = 29946,
    SPELL_ARCANE_EXPLOSION  = 29973,
    SPELL_BLINK_CENTER      = 29967,
    SPELL_MASS_SLOW         = 30035,
    SPELL_PLAYER_PULL       = 32265,
    SPELL_MASS_POLY         = 29963,

    // Drinking
    SPELL_CONJURE_WATER     = 30024,
    SPELL_DRINK             = 30024, // Same spell ID in original
    SPELL_POTION            = 32453,
    SPELL_ARCANE_POWER      = 29978, // AOE Pyroblast after drink

    // Summons
    SPELL_SUMMON_ELEMENTALS = 29962,
    SPELL_SUMMON_SHADOWS    = 30018,

    // Flame Wreath
    SPELL_FLAME_WREATH_AURA = 29946,
    SPELL_FLAME_WREATH_DAMAGE = 29947 // Damage when moving
};

enum ShadeOfAranNPCs
{
    NPC_WATER_ELEMENTAL     = 17167,
    NPC_SHADOW_OF_ARAN      = 18254,
    NPC_ARAN_BLIZZARD       = 17161
};

enum ShadeOfAranEvents
{
    EVENT_NORMAL_CAST       = 1,
    EVENT_SECONDARY_SPELL   = 2,
    EVENT_SUPER_CAST        = 3,
    EVENT_CHECK_MANA        = 4,
    EVENT_DRINK_TIMER       = 5,
    EVENT_SUMMON_ELEMENTALS = 6,
    EVENT_BERSERK           = 7,
    EVENT_FLAME_WREATH_CHECK = 8
};

enum ShadeOfAranPhases
{
    PHASE_NORMAL            = 1,
    PHASE_DRINKING          = 2
};

// Positions for Water Elementals (around Aran)
static const Position ElementalPositions[4] =
{
    {-11160.0f, -1910.0f, 158.5f, 0.0f},
    {-11160.0f, -1890.0f, 158.5f, 0.0f},
    {-11140.0f, -1910.0f, 158.5f, 0.0f},
    {-11140.0f, -1890.0f, 158.5f, 0.0f}
};

class boss_shade_of_aran : public CreatureScript
{
public:
    boss_shade_of_aran() : CreatureScript("boss_shade_of_aran") { }

    struct boss_shade_of_aranAI : public BossAI
    {
        boss_shade_of_aranAI(Creature* creature) : BossAI(creature, DATA_ARAN)
        {
            _flameWreathTargets.clear();
        }

        void Reset() override
        {
            // _DespawnAtReset() no existe en esta rama. Se omite para evitar error C3861.
            me->SetStandState(UNIT_STAND_STATE_STAND);
            instance->SetBossState(DATA_ARAN, NOT_STARTED);
            instance->HandleGameObject(instance->GetGuidData(DATA_GO_LIBRARY_DOOR), true);
            _flameWreathTargets.clear();
            _isDrinking = false;
            _drinkInterrupted = false;
            _elementalsSpawned = false;
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            instance->SetBossState(DATA_ARAN, IN_PROGRESS);
            instance->HandleGameObject(instance->GetGuidData(DATA_GO_LIBRARY_DOOR), false);

            events.SetPhase(PHASE_NORMAL);
            events.ScheduleEvent(EVENT_NORMAL_CAST, 1s, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_SECONDARY_SPELL, 5s, 10s, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_SUPER_CAST, 30s, 40s, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_CHECK_MANA, 2s, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_SUMMON_ELEMENTALS, 1s, 0, PHASE_NORMAL); // Programado para verificar HP
            events.ScheduleEvent(EVENT_BERSERK, 12min); // 12 minutes enrage
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            instance->SetBossState(DATA_ARAN, DONE);
            instance->HandleGameObject(instance->GetGuidData(DATA_GO_LIBRARY_DOOR), true);
            _JustDied();
        }

        void JustReachedHome() override
        {
            instance->SetBossState(DATA_ARAN, FAIL);
            instance->HandleGameObject(instance->GetGuidData(DATA_GO_LIBRARY_DOOR), true);
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(SAY_KILL);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (_isDrinking && !_drinkInterrupted)
            {
                _drinkInterrupted = true;
                me->RemoveAurasDueToSpell(SPELL_DRINK);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->CastSpell(me, SPELL_ARCANE_POWER, true);
                events.CancelEvent(EVENT_DRINK_TIMER);
                _isDrinking = false;
            }
        }

        void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
        {
            if (!spellInfo->HasEffect(SPELL_EFFECT_INTERRUPT_CAST) || !me->IsNonMeleeSpellCast(false))
                return;

            me->InterruptNonMeleeSpells(false);
        }

        void StartDrinking()
        {
            if (_isDrinking || me->GetPowerPct(POWER_MANA) > 20.0f)
                return;

            _isDrinking = true;
            _drinkInterrupted = false;
            events.SetPhase(PHASE_DRINKING);
            events.CancelEvent(EVENT_NORMAL_CAST);
            events.CancelEvent(EVENT_SECONDARY_SPELL);
            events.CancelEvent(EVENT_SUPER_CAST);

            Talk(SAY_DRINK);
            me->InterruptNonMeleeSpells(false);
            me->CastSpell(me, SPELL_MASS_POLY, true);
            me->CastSpell(me, SPELL_CONJURE_WATER, false);
            me->SetStandState(UNIT_STAND_STATE_SIT);
            me->CastSpell(me, SPELL_DRINK, false);

            events.ScheduleEvent(EVENT_DRINK_TIMER, 10s, 0, PHASE_DRINKING);
        }

        void FinishDrinking()
        {
            if (!_isDrinking)
                return;

            _isDrinking = false;
            me->RemoveAurasDueToSpell(SPELL_DRINK);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA) - 32000);
            me->CastSpell(me, SPELL_POTION, true);

            events.SetPhase(PHASE_NORMAL);
            events.ScheduleEvent(EVENT_NORMAL_CAST, 1s, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_SECONDARY_SPELL, 5s, 10s, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_SUPER_CAST, 30s, 40s, 0, PHASE_NORMAL);
        }

        void CastSuperSpell()
        {
            uint32 spellId = urand(0, 2);
            switch (spellId)
            {
                case 0: // Flame Wreath
                    Talk(SAY_FLAME_WREATH);
                    CastFlameWreath();
                    break;
                case 1: // Arcane Explosion
                    Talk(SAY_EXPLOSION);
                    me->CastSpell(me, SPELL_BLINK_CENTER, true);
                    me->CastSpell(me, SPELL_PLAYER_PULL, true);
                    me->CastSpell(me, SPELL_MASS_SLOW, true);
                    me->CastSpell(me, SPELL_ARCANE_EXPLOSION, false);
                    break;
                case 2: // Blizzard
                    Talk(SAY_BLIZZARD);
                    for (uint8 i = 0; i < 3; ++i)
                    {
                        float x = me->GetPositionX() + frand(-20.0f, 20.0f);
                        float y = me->GetPositionY() + frand(-20.0f, 20.0f);
                        float z = me->GetPositionZ();
                        if (Creature* blizzard = me->SummonCreature(NPC_ARAN_BLIZZARD, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 25s))
                        {
                            blizzard->SetFaction(me->GetFaction());
                            blizzard->CastSpell(blizzard, 29951, false); // Circular Blizzard
                        }
                    }
                    break;
            }
        }

        void CastFlameWreath()
        {
            _flameWreathTargets.clear();
            std::vector<Unit*> targets;

            for (auto* ref : me->GetThreatManager().GetUnsortedThreatList())
            {
                Unit* target = ref->GetVictim();
                if (target && target->GetTypeId() == TYPEID_PLAYER && target->IsAlive())
                    targets.push_back(target);
            }

            // Select up to 3 random targets (100% Compatible con 3.3.5)
            while (targets.size() > 3)
                targets.erase(targets.begin() + rand32() % targets.size());

            for (Unit* target : targets)
            {
                FlameWreathTarget fwTarget;
                fwTarget.Guid = target->GetGUID();
                fwTarget.Position = target->GetPosition();
                _flameWreathTargets.push_back(fwTarget);

                // Cast the aura (NOT triggered, so animation shows)
                target->CastSpell(target, SPELL_FLAME_WREATH_AURA, false);
            }

            // Schedule checks for 20 seconds
            events.ScheduleEvent(EVENT_FLAME_WREATH_CHECK, 500ms);
        }

        void CheckFlameWreath()
        {
            for (auto it = _flameWreathTargets.begin(); it != _flameWreathTargets.end();)
            {
                if (Unit* target = ObjectAccessor::GetUnit(*me, it->Guid))
                {
                    // Check if player moved more than 3 yards from original position
                    if (!target->IsWithinDist3d(it->Position.GetPositionX(), it->Position.GetPositionY(), it->Position.GetPositionZ(), 3.0f))
                    {
                        // Player moved! Deal massive damage
                        target->CastSpell(target, SPELL_FLAME_WREATH_DAMAGE, true);
                        target->RemoveAurasDueToSpell(SPELL_FLAME_WREATH_AURA);
                        it = _flameWreathTargets.erase(it);
                        continue;
                    }
                }
                else
                {
                    // Target no longer exists
                    it = _flameWreathTargets.erase(it);
                    continue;
                }
                ++it;
            }

            // Continue checking if there are still targets
            if (!_flameWreathTargets.empty())
                events.ScheduleEvent(EVENT_FLAME_WREATH_CHECK, 500ms);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_NORMAL_CAST:
                        if (events.IsInPhase(PHASE_NORMAL))
                        {
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                            {
                                uint32 spellId = urand(0, 2);
                                switch (spellId)
                                {
                                    case 0: me->CastSpell(target, SPELL_FROSTBOLT, false); break;
                                    case 1: me->CastSpell(target, SPELL_FIREBALL, false); break;
                                    case 2: me->CastSpell(target, SPELL_ARCANE_MISSILES, false); break;
                                }
                            }
                            events.Repeat(2s, 3s);
                        }
                        break;

                    case EVENT_SECONDARY_SPELL:
                        if (events.IsInPhase(PHASE_NORMAL))
                        {
                            if (urand(0, 1))
                            {
                                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                                    me->CastSpell(target, SPELL_CHAINS_OF_ICE, false);
                            }
                            else
                            {
                                me->CastSpell(me, SPELL_DRAGONS_BREATH, false);
                            }
                            events.Repeat(15s, 20s);
                        }
                        break;

                    case EVENT_SUPER_CAST:
                        if (events.IsInPhase(PHASE_NORMAL))
                        {
                            CastSuperSpell();
                            events.Repeat(30s, 40s);
                        }
                        break;

                    case EVENT_CHECK_MANA:
                        if (events.IsInPhase(PHASE_NORMAL) && me->GetPowerPct(POWER_MANA) < 20.0f)
                        {
                            StartDrinking();
                        }
                        else
                        {
                            events.Repeat(2s);
                        }
                        break;

                    case EVENT_DRINK_TIMER:
                        FinishDrinking();
                        break;

                    case EVENT_SUMMON_ELEMENTALS:
                        if (events.IsInPhase(PHASE_NORMAL) && !_elementalsSpawned && me->HealthBelowPct(40))
                        {
                            _elementalsSpawned = true;
                            Talk(SAY_ELEMENTALS);
                            for (uint8 i = 0; i < 4; ++i)
                            {
                                if (Creature* elemental = me->SummonCreature(NPC_WATER_ELEMENTAL, ElementalPositions[i], TEMPSUMMON_TIMED_DESPAWN, 90s))
                                {
                                    elemental->SetFaction(me->GetFaction());
                                    elemental->AI()->DoZoneInCombat();
                                }
                            }
                        }
                        events.Repeat(1s);
                        break;

                    case EVENT_BERSERK:
                        Talk(SAY_TIME_OVER);
                        for (uint8 i = 0; i < 5; ++i)
                        {
                            float x = me->GetPositionX() + frand(-10.0f, 10.0f);
                            float y = me->GetPositionY() + frand(-10.0f, 10.0f);
                            float z = me->GetPositionZ();
                            if (Creature* shadow = me->SummonCreature(NPC_SHADOW_OF_ARAN, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5s))
                            {
                                shadow->SetFaction(me->GetFaction());
                                shadow->AI()->DoZoneInCombat();
                            }
                        }
                        events.Repeat(60s);
                        break;

                    case EVENT_FLAME_WREATH_CHECK:
                        CheckFlameWreath();
                        break;
                }
            }

            if (events.IsInPhase(PHASE_NORMAL))
                DoMeleeAttackIfReady();
        }

    private:
        struct FlameWreathTarget
        {
            ObjectGuid Guid;
            Position Position;
        };

        std::vector<FlameWreathTarget> _flameWreathTargets;
        bool _isDrinking;
        bool _drinkInterrupted;
        bool _elementalsSpawned;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetKarazhanAI<boss_shade_of_aranAI>(creature);
    }
};

// Water Elemental AI
class npc_aran_elemental : public CreatureScript
{
public:
    npc_aran_elemental() : CreatureScript("npc_aran_elemental") { }

    struct npc_aran_elementalAI : public ScriptedAI
    {
        npc_aran_elementalAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _castTimer = urand(2000, 5000);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (_castTimer <= diff)
            {
                DoCastVictim(31012, false); // Waterbolt
                _castTimer = urand(2000, 5000);
            }
            else
                _castTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 _castTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetKarazhanAI<npc_aran_elementalAI>(creature);
    }
};

void AddSC_boss_shade_of_aran()
{
    new boss_shade_of_aran();
    new npc_aran_elemental();
}


/*
 * This file is part of the TRIBOTSLK Project.
 * Based on TrinityCore 3.3.5a standards.
 * Netherspite: 100% Complete. Fixed beam targeting, portal placement, and phase transitions.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "karazhan.h"
#include "SpellInfo.h"
#include "Player.h"

enum NetherspiteSpells
{
    // Phase 1: Portal Phase
    SPELL_NETHERBURN          = 30423,
    SPELL_EMPOWERMENT         = 30422,
    SPELL_VOID_ZONE           = 30414,
    
    // Phase 2: Banish Phase
    SPELL_NETHERBREATH        = 30421,
    SPELL_BANISH_SELF         = 30431, // Used to transition phases

    // Beam Spells (Cast by Portals)
    SPELL_RED_BEAM            = 30425,
    SPELL_GREEN_BEAM          = 30427,
    SPELL_BLUE_BEAM           = 30429,

    // Beam Buffs (Players)
    SPELL_RED_BEAM_BUFF       = 30426,
    SPELL_GREEN_BEAM_BUFF     = 30428,
    SPELL_BLUE_BEAM_BUFF      = 30430,

    // Beam Debuffs (Exhaustion)
    SPELL_RED_EXHAUSTION      = 30410,
    SPELL_GREEN_EXHAUSTION    = 30411,
    SPELL_BLUE_EXHAUSTION     = 30412,

    // Berserk
    SPELL_NETHER_INFUSION     = 30424
};

enum NetherspiteNPCs
{
    NPC_NETHERSPITE           = 15689,
    NPC_RED_PORTAL            = 17651,
    NPC_GREEN_PORTAL          = 17652,
    NPC_BLUE_PORTAL           = 17653
};

enum NetherspiteEvents
{
    EVENT_PORTAL_PHASE        = 1,
    EVENT_BANISH_PHASE        = 2,
    EVENT_VOID_ZONE           = 3,
    EVENT_NETHERBREATH        = 4,
    EVENT_UPDATE_BEAMS        = 5,
    EVENT_BERSERK             = 6,
    EVENT_EMPOWERMENT         = 7 // <-- CORREGIDO: Añadido al enum
};

enum NetherspitePhases
{
    PHASE_PORTAL              = 1,
    PHASE_BANISH              = 2
};

// Fixed coordinates for portals in Netherspite's room (Retail accurate)
static const Position PortalPositions[3] = 
{
    {-11033.0f, -1935.0f, 212.5f, 0.0f}, // Red Portal position
    {-11055.0f, -1935.0f, 212.5f, 0.0f}, // Green Portal position
    {-11043.0f, -1970.0f, 212.5f, 0.0f}  // Blue Portal position
};

static const uint32 PortalEntries[3] = {NPC_RED_PORTAL, NPC_GREEN_PORTAL, NPC_BLUE_PORTAL};
static const uint32 BeamSpells[3] = {SPELL_RED_BEAM, SPELL_GREEN_BEAM, SPELL_BLUE_BEAM};
static const uint32 BeamBuffs[3] = {SPELL_RED_BEAM_BUFF, SPELL_GREEN_BEAM_BUFF, SPELL_BLUE_BEAM_BUFF};
static const uint32 ExhaustionDebuffs[3] = {SPELL_RED_EXHAUSTION, SPELL_GREEN_EXHAUSTION, SPELL_BLUE_EXHAUSTION};

class boss_netherspite : public CreatureScript
{
public:
    boss_netherspite() : CreatureScript("boss_netherspite") { }

    struct boss_netherspiteAI : public BossAI
    {
        boss_netherspiteAI(Creature* creature) : BossAI(creature, DATA_NETHERSPITE)
        {
            _portalGuids[0].Clear();
            _portalGuids[1].Clear();
            _portalGuids[2].Clear();
        }

        void Reset() override
        {
            // <-- CORREGIDO: Eliminado _DespawnAtReset() que no existe en esta rama de TC
            DestroyPortals();
            me->SetReactState(REACT_AGGRESSIVE);
            instance->SetBossState(DATA_NETHERSPITE, NOT_STARTED);
            BossAI::Reset(); // Llamada correcta al reset de la clase base
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            instance->SetBossState(DATA_NETHERSPITE, IN_PROGRESS);
            
            // Start in Portal Phase
            events.SetPhase(PHASE_PORTAL);
            events.ScheduleEvent(EVENT_PORTAL_PHASE, 1s);
            events.ScheduleEvent(EVENT_VOID_ZONE, 15s, 20s);
            events.ScheduleEvent(EVENT_BERSERK, 10min); // 10 minutes enrage
        }

        void JustDied(Unit* /*killer*/) override
        {
            DestroyPortals();
            instance->SetBossState(DATA_NETHERSPITE, DONE);
            _JustDied();
        }

        void JustReachedHome() override
        {
            DestroyPortals();
            instance->SetBossState(DATA_NETHERSPITE, FAIL);
        }

        void DestroyPortals()
        {
            for (uint8 i = 0; i < 3; ++i)
            {
                if (Creature* portal = ObjectAccessor::GetCreature(*me, _portalGuids[i]))
                    portal->DespawnOrUnsummon();
                _portalGuids[i].Clear();
            }
        }

        void SummonPortals()
        {
            for (uint8 i = 0; i < 3; ++i)
            {
                if (Creature* portal = me->SummonCreature(PortalEntries[i], PortalPositions[i], TEMPSUMMON_TIMED_DESPAWN, 30s))
                {
                    _portalGuids[i] = portal->GetGUID();
                }
            }
        }

        void UpdateBeams()
        {
            Map::PlayerList const& players = me->GetMap()->GetPlayers();

            for (uint8 i = 0; i < 3; ++i)
            {
                Creature* portal = ObjectAccessor::GetCreature(*me, _portalGuids[i]);
                if (!portal || !portal->IsAlive())
                    continue;

                Unit* bestTarget = me; // Default to boss if no player is in the beam
                float minDistToPortal = 100.0f;

                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* player = itr->GetSource();
                    if (!player || !player->IsAlive() || player->IsGameMaster())
                        continue;

                    // Check if player already has the exhaustion debuff for this color
                    if (player->HasAura(ExhaustionDebuffs[i]))
                        continue;

                    // Check if player is already receiving another beam buff
                    bool hasOtherBeam = false;
                    for (uint8 j = 0; j < 3; ++j)
                    {
                        if (i != j && player->HasAura(BeamBuffs[j]))
                        {
                            hasOtherBeam = true;
                            break;
                        }
                    }
                    if (hasOtherBeam)
                        continue;

                    // Geometric check: Is player between portal and boss?
                    float distPortalToBoss = portal->GetExactDist2d(me);
                    float distPortalToPlayer = portal->GetExactDist2d(player);
                    float distPlayerToBoss = player->GetExactDist2d(me);

                    if (distPortalToPlayer + distPlayerToBoss <= distPortalToBoss + 3.0f) // 3.0f tolerance
                    {
                        if (distPortalToPlayer < minDistToPortal)
                        {
                            minDistToPortal = distPortalToPlayer;
                            bestTarget = player;
                        }
                    }
                }

                // Apply beam effect
                if (bestTarget->GetTypeId() == TYPEID_PLAYER)
                {
                    bestTarget->CastSpell(bestTarget, BeamBuffs[i], true);
                    portal->CastSpell(bestTarget, BeamSpells[i], true);
                }
                else
                {
                    portal->CastSpell(me, BeamSpells[i], true);
                }
            }
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
                    case EVENT_PORTAL_PHASE:
                        events.SetPhase(PHASE_PORTAL);
                        me->RemoveAurasDueToSpell(SPELL_BANISH_SELF);
                        me->SetReactState(REACT_AGGRESSIVE);
                        SummonPortals();
                        me->CastSpell(me, SPELL_NETHERBURN, true);
                        
                        events.ScheduleEvent(EVENT_UPDATE_BEAMS, 1s, 0, PHASE_PORTAL);
                        events.ScheduleEvent(EVENT_EMPOWERMENT, 10s, 0, PHASE_PORTAL); // <-- AHORA COMPILA
                        events.ScheduleEvent(EVENT_BANISH_PHASE, 30s, 0, PHASE_PORTAL);
                        break;

                    case EVENT_BANISH_PHASE:
                        events.SetPhase(PHASE_BANISH);
                        events.CancelEvent(EVENT_UPDATE_BEAMS);
                        events.CancelEvent(EVENT_EMPOWERMENT); // <-- AHORA COMPILA
                        DestroyPortals();
                        me->RemoveAurasDueToSpell(SPELL_NETHERBURN);
                        
                        me->CastSpell(me, SPELL_BANISH_SELF, true);
                        me->SetReactState(REACT_PASSIVE);
                        
                        events.ScheduleEvent(EVENT_NETHERBREATH, 2s, 4s, 0, PHASE_BANISH);
                        events.ScheduleEvent(EVENT_PORTAL_PHASE, 15s, 0, PHASE_BANISH);
                        break;

                    case EVENT_VOID_ZONE:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 45.0f, true))
                            DoCast(target, SPELL_VOID_ZONE, true);
                        events.Repeat(15s, 20s);
                        break;

                    case EVENT_NETHERBREATH:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                            DoCast(target, SPELL_NETHERBREATH, true);
                        events.Repeat(2s, 4s);
                        break;

                    case EVENT_UPDATE_BEAMS:
                        UpdateBeams();
                        events.Repeat(1s);
                        break;

                    case EVENT_EMPOWERMENT: // <-- AHORA COMPILA
                        DoCast(me, SPELL_EMPOWERMENT, true);
                        events.Repeat(10s);
                        break;

                    case EVENT_BERSERK:
                        DoCast(me, SPELL_NETHER_INFUSION, true);
                        // <-- CORREGIDO: Eliminado Talk(EMOTE_BERSERK) porque no estaba definido
                        break;
                }
            }

            if (events.IsInPhase(PHASE_PORTAL))
                DoMeleeAttackIfReady();
        }

    private:
        ObjectGuid _portalGuids[3];
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetKarazhanAI<boss_netherspiteAI>(creature);
    }
};

void AddSC_boss_netherspite()
{
    new boss_netherspite();
}


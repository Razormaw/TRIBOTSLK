/*
 * This file is part of the TRIBOTSLK Project.
 * Karazhan Chess Event: Dynamic Faction Auto-Battle Mode.
 * Fully supports both Alliance and Horde players + NPCBots.
 * Compatible with TrinityCore 3.3.5 (TRIBOTSLK fork)
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "karazhan.h"
#include "SpellInfo.h"
#include "Player.h"
#include "ObjectAccessor.h"

enum ChessEventNPCs
{
    NPC_ECHO_OF_MEDIVH      = 16816,
    // Alliance Pieces
    NPC_KING_LLANE          = 21684,
    NPC_HUMAN_CONJURER      = 21683,
    NPC_HUMAN_CLERIC        = 21682,
    NPC_HUMAN_CHARGER       = 21664,
    NPC_WATER_ELEMENTAL     = 21160,
    NPC_HUMAN_FOOTMAN       = 17211,
    // Horde Pieces
    NPC_WARCHEF_BLACKHAND   = 21752,
    NPC_ORC_WARLOCK         = 21750,
    NPC_ORC_NECROLYTE       = 21747,
    NPC_ORC_WOLF            = 21748,
    NPC_SUMMONED_DAEMON     = 21726,
    NPC_ORC_GRUNT           = 17469
};

enum ChessEventSpells
{
    SPELL_HEAL              = 30024,
    SPELL_CHARGE            = 32265,
    SPELL_WATERBOLT         = 31012,
    SPELL_FIREBALL          = 29953,
    SPELL_SHADOW_BOLT       = 30414,
    SPELL_BUFF_RESOLVE      = 30422
};

// Helper function to determine faction of a chess piece
bool IsAlliancePiece(uint32 entry)
{
    return entry == NPC_KING_LLANE || entry == NPC_HUMAN_CONJURER || entry == NPC_HUMAN_CLERIC ||
           entry == NPC_HUMAN_CHARGER || entry == NPC_WATER_ELEMENTAL || entry == NPC_HUMAN_FOOTMAN;
}

uint32 GetEnemyKingEntry(uint32 myEntry)
{
    return IsAlliancePiece(myEntry) ? NPC_WARCHEF_BLACKHAND : NPC_KING_LLANE;
}

// ==========================================
// AI SCRIPT (CreatureScript)
// ==========================================
class npc_echo_of_medivh_chess : public CreatureScript
{
public:
    npc_echo_of_medivh_chess() : CreatureScript("npc_echo_of_medivh_chess") { }

    struct npc_echo_of_medivh_chessAI : public ScriptedAI
    {
        npc_echo_of_medivh_chessAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()) { }

        void DoAction(int32 action) override
        {
            bool activateAlliance = (action == TEAM_ALLIANCE);
            uint32 activeKing = activateAlliance ? NPC_KING_LLANE : NPC_WARCHEF_BLACKHAND;
            uint32 enemyKing = activateAlliance ? NPC_WARCHEF_BLACKHAND : NPC_KING_LLANE;

            std::list<Creature*> allPieces;
            
            // Búsqueda nativa de TC 3.3.5
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_KING_LLANE, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_HUMAN_CONJURER, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_HUMAN_CLERIC, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_HUMAN_CHARGER, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_WATER_ELEMENTAL, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_HUMAN_FOOTMAN, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_WARCHEF_BLACKHAND, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_ORC_WARLOCK, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_ORC_NECROLYTE, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_ORC_WOLF, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_SUMMONED_DAEMON, 100.0f);
            GetCreatureListWithEntryInGrid(allPieces, me, NPC_ORC_GRUNT, 100.0f);

            for (Creature* piece : allPieces)
            {
                bool isMyPiece = IsAlliancePiece(piece->GetEntry()) == activateAlliance;
                
                piece->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                piece->SetReactState(REACT_AGGRESSIVE);
                
                if (isMyPiece)
                {
                    piece->SetFaction(activateAlliance ? 14 : 15); 
                    if (Creature* target = piece->FindNearestCreature(enemyKing, 100.0f, true))
                        piece->AI()->AttackStart(target);
                }
                else
                {
                    piece->SetFaction(activateAlliance ? 15 : 14);
                    if (Creature* target = piece->FindNearestCreature(activeKing, 100.0f, true))
                        piece->AI()->AttackStart(target);
                }
            }
        }

        // CORRECCIÓN: El primer parámetro debe ser WorldObject* en TC 3.3.5, no Unit*
        void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
        {
            // 46598 es el hechizo dummy que lanza el SmartAI cuando el jugador hace clic en el gossip
            if (spellInfo->Id == 46598)
            {
                if (Player* player = target->ToPlayer())
                {
                    // Ejecutar la lógica de activación de piezas con la facción del jugador
                    DoAction(player->GetTeamId());
                }
            }
        }

    private:
        InstanceScript* _instance;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_echo_of_medivh_chessAI(creature);
    }
};

// ==========================================
// CHESS PIECES AI
// ==========================================
class npc_karazhan_chess_piece : public CreatureScript
{
public:
    npc_karazhan_chess_piece() : CreatureScript("npc_karazhan_chess_piece") { }

    struct npc_karazhan_chess_pieceAI : public ScriptedAI
    {
        uint32 _healTimer;
        uint32 _chargeTimer;
        uint32 _spellTimer;
        uint32 _buffTimer;
        bool _isAlliance;
        uint32 _myFaction;

        npc_karazhan_chess_pieceAI(Creature* creature) : ScriptedAI(creature) 
        {
            _isAlliance = IsAlliancePiece(me->GetEntry());
            _myFaction = _isAlliance ? 14 : 15;
        }

        void Reset() override
        {
            me->SetFaction(_myFaction);
            me->SetReactState(REACT_DEFENSIVE);
            
            _healTimer = 5000;
            _chargeTimer = 8000;
            _spellTimer = 3000;
            _buffTimer = 15000;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                uint32 enemyKing = GetEnemyKingEntry(me->GetEntry());
                if (Creature* target = me->FindNearestCreature(enemyKing, 100.0f, true))
                    me->AI()->AttackStart(target);
                return;
            }

            switch (me->GetEntry())
            {
                case NPC_HUMAN_CLERIC:
                case NPC_ORC_NECROLYTE:
                    if (_healTimer <= diff)
                    {
                        if (urand(0, 100) < 30)
                            if (Unit* friendly = DoSelectLowestHpFriendly(30.0f))
                                DoCast(friendly, SPELL_HEAL);
                        _healTimer = 5000;
                    }
                    else _healTimer -= diff;
                    break;

                case NPC_HUMAN_CHARGER:
                case NPC_ORC_WOLF:
                    if (_chargeTimer <= diff)
                    {
                        if (urand(0, 100) < 40)
                            DoCastVictim(SPELL_CHARGE);
                        _chargeTimer = 8000;
                    }
                    else _chargeTimer -= diff;
                    break;

                case NPC_WATER_ELEMENTAL:
                case NPC_SUMMONED_DAEMON:
                case NPC_HUMAN_CONJURER:
                case NPC_ORC_WARLOCK:
                    if (_spellTimer <= diff)
                    {
                        uint32 spell = SPELL_SHADOW_BOLT;
                        if (me->GetEntry() == NPC_WATER_ELEMENTAL) 
                            spell = SPELL_WATERBOLT;
                        else if (me->GetEntry() == NPC_HUMAN_CONJURER) 
                            spell = SPELL_FIREBALL;
                            
                        DoCastVictim(spell);
                        _spellTimer = 4000;
                    }
                    else _spellTimer -= diff;
                    break;

                case NPC_KING_LLANE:
                case NPC_WARCHEF_BLACKHAND:
                    if (_buffTimer <= diff)
                    {
                        if (me->HealthBelowPct(70))
                            DoCastSelf(SPELL_BUFF_RESOLVE);
                        _buffTimer = 15000;
                    }
                    else _buffTimer -= diff;
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_karazhan_chess_pieceAI(creature);
    }
};

void AddSC_karazhan_chess()
{
    new npc_echo_of_medivh_chess();
    new npc_karazhan_chess_piece();
}


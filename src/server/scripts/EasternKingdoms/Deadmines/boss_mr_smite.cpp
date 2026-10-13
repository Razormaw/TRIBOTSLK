/*
 * TRIBOTSLK - Minas de la Muerte: Mr. Smite
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"
#include "GameObject.h"
#include "ObjectAccessor.h"

using namespace std::chrono_literals;

enum SmiteEquips { EQUIP_SWORD = 5191, EQUIP_AXE = 5196, EQUIP_MACE = 7230 };
enum SmiteEvents { EVENT_PHASE_TRANSITION = 1, EVENT_EQUIP_CHANGE, EVENT_STAND_UP, EVENT_RESUME_COMBAT };
enum SmiteTexts { SAY_PHASE_1 = 0, SAY_PHASE_2 = 1, SAY_DEATH = 2 };

struct boss_mr_smite : public BossAI
{
    boss_mr_smite(Creature* c) : BossAI(c, BOSS_MR_SMITE), _phase(0), _isMoving(false) { }

    void Reset() override {
        _Reset(); _phase = 0; _isMoving = false;
        SetEquipmentSlots(false, EQUIP_SWORD, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit* who) override {
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(SPELL_SMITE_SLAM, 9s, 12s);
        events.ScheduleEvent(SPELL_TRASH, 6s, 10s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, const SpellInfo*) override {
        if (_phase == 0 && me->HealthBelowPct(66)) { _phase = 1; StartPhaseTransition(); }
        else if (_phase == 2 && me->HealthBelowPct(33)) { _phase = 3; StartPhaseTransition(); }
    }

    void StartPhaseTransition() {
        Talk(SAY_PHASE_1);
        DoCastAOE(SPELL_SMITE_STOMP, false);
        SetCombatMovement(false);
        me->AttackStop();
        me->InterruptNonMeleeSpells(false);
        me->SetReactState(REACT_PASSIVE);
        events.ScheduleEvent(EVENT_PHASE_TRANSITION, 2s);
    }

    void JustDied(Unit* /*killer*/) override { Talk(SAY_DEATH); _JustDied(); }

    void MovementInform(uint32 type, uint32 id) override {
        if (type != POINT_MOTION_TYPE || id != 1) return;
        me->SetFacingTo(5.47f);
        me->SetStandState(UNIT_STAND_STATE_KNEEL);
        events.ScheduleEvent(EVENT_EQUIP_CHANGE, 2s);
    }

    void UpdateAI(uint32 diff) override {
        if (!UpdateVictim() && _phase % 2 == 0) return;
        events.Update(diff);
        if (_isMoving) return;

        while (uint32 eventId = events.ExecuteEvent()) {
            switch (eventId) {
                case SPELL_SMITE_SLAM:
                    DoCastVictim(SPELL_SMITE_SLAM);
                    events.ScheduleEvent(SPELL_SMITE_SLAM, 11s, 15s);
                    break;
                case SPELL_TRASH:
                    DoCastSelf(SPELL_TRASH);
                    events.ScheduleEvent(SPELL_TRASH, 8s, 12s);
                    break;
                case EVENT_PHASE_TRANSITION:
                    _isMoving = true;
                    if (GameObject* chest = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(DATA_SMITE_CHEST)))
                        me->GetMotionMaster()->MovePoint(1, chest->GetPositionX() - 1.5f, chest->GetPositionY() + 1.4f, chest->GetPositionZ());
                    break;
                case EVENT_EQUIP_CHANGE:
                    if (_phase == 1) SetEquipmentSlots(false, EQUIP_AXE, EQUIP_AXE, EQUIP_NO_CHANGE);
                    else if (_phase == 3) SetEquipmentSlots(false, EQUIP_MACE, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                    events.ScheduleEvent(EVENT_STAND_UP, 500ms);
                    break;
                case EVENT_STAND_UP:
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    events.ScheduleEvent(EVENT_RESUME_COMBAT, 750ms);
                    break;
                case EVENT_RESUME_COMBAT:
                    me->SetReactState(REACT_AGGRESSIVE);
                    SetCombatMovement(true);
                    me->GetMotionMaster()->MoveChase(me->GetVictim(), me->m_CombatDistance);
                    _isMoving = false;
                    _phase++;
                    break;
            }
        }
        if (!_isMoving) DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
    bool _isMoving;
};

void AddSC_boss_mr_smite()
{
    RegisterDeadminesCreatureAI(boss_mr_smite);
}


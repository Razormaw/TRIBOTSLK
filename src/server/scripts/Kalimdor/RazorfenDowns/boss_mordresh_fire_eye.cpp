/*
 * TRIBOTSLK - RazorfenDowns: Mordresh Fire Eye
 * Script mejorado con diálogos inmersivos.
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "razorfen_downs.h"

enum MordreshYells
{
    SAY_OOC_1   = 0,
    SAY_OOC_2   = 1,
    SAY_OOC_3   = 2,
    SAY_AGGRO   = 3,
    SAY_KILL    = 4,
    SAY_DEATH   = 5
};

enum MordreshSpells
{
    SPELL_FIREBALL  = 12466,
    SPELL_FIRE_NOVA = 12470
};

enum MordreshEvents
{
    EVENT_OOC_1 = 1,
    EVENT_OOC_2,
    EVENT_OOC_3,
    EVENT_OOC_4,
    EVENT_FIREBALL,
    EVENT_FIRE_NOVA
};

struct boss_mordresh_fire_eye : public BossAI
{
    boss_mordresh_fire_eye(Creature* creature) : BossAI(creature, DATA_MORDRESH_FIRE_EYE) { }

    void Reset() override
    {
        _Reset();
        events.ScheduleEvent(EVENT_OOC_1, 10s);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        events.Reset();
        Talk(SAY_AGGRO);
        events.ScheduleEvent(EVENT_FIREBALL, 0s);
        events.ScheduleEvent(EVENT_FIRE_NOVA, 8s, 12s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_DEATH);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);

        if (!UpdateVictim())
        {
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_OOC_1:
                        Talk(SAY_OOC_1);
                        events.ScheduleEvent(EVENT_OOC_2, 8s);
                        break;
                    case EVENT_OOC_2:
                        Talk(SAY_OOC_2);
                        events.ScheduleEvent(EVENT_OOC_3, 3s);
                        break;
                    case EVENT_OOC_3:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                        events.ScheduleEvent(EVENT_OOC_4, 6s);
                        break;
                    case EVENT_OOC_4:
                        Talk(SAY_OOC_3);
                        events.ScheduleEvent(EVENT_OOC_1, 14s);
                        break;
                }
            }
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_FIREBALL:
                    DoCastVictim(SPELL_FIREBALL);
                    events.Repeat(2400ms, 3800ms);
                    break;
                case EVENT_FIRE_NOVA:
                    DoCastSelf(SPELL_FIRE_NOVA);
                    events.Repeat(11s, 16s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }
};

void AddSC_boss_mordresh_fire_eye()
{
    RegisterRazorfenDownsCreatureAI(boss_mordresh_fire_eye);
}


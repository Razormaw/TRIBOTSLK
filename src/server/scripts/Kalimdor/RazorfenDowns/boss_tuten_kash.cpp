/*
 * TRIBOTSLK - RazorfenDowns: Tuten'kash
 * Script mejorado con diálogos inmersivos.
 */
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "razorfen_downs.h"

enum TutenkashYells
{
    SAY_AGGRO     = 0,
    SAY_WEB_SPRAY = 1,
    SAY_DEATH     = 2,
    SAY_KILL      = 3
};

enum TutenkashSpells
{
    SPELL_VIRULENT_POISON    = 12254,
    SPELL_THRASH             = 8876,
    SPELL_WEB_SPRAY          = 12252,
    SPELL_CURSE_OF_TUTENKASH = 12255
};

enum TutenkashEvents
{
    EVENT_WEB_SPRAY = 1,
    EVENT_CURSE_OF_TUTENKASH
};

struct boss_tuten_kash : public BossAI
{
    boss_tuten_kash(Creature* creature) : BossAI(creature, DATA_TUTEN_KASH) { }

    void Reset() override
    {
        _Reset();
        DoCastSelf(SPELL_VIRULENT_POISON);
        DoCastSelf(SPELL_THRASH);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        events.ScheduleEvent(EVENT_WEB_SPRAY, 10s, 20s);
        events.ScheduleEvent(EVENT_CURSE_OF_TUTENKASH, 10s, 15s);
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
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_WEB_SPRAY:
                    Talk(SAY_WEB_SPRAY);
                    DoCastSelf(SPELL_WEB_SPRAY);
                    events.Repeat(15s, 25s);
                    break;
                case EVENT_CURSE_OF_TUTENKASH:
                    DoCastSelf(SPELL_CURSE_OF_TUTENKASH);
                    events.Repeat(15s, 25s);
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

void AddSC_boss_tuten_kash()
{
    RegisterRazorfenDownsCreatureAI(boss_tuten_kash);
}


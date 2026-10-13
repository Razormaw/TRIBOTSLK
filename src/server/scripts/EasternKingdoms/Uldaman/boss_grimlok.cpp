#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum GrimlokSpells { SPELL_CLEAVE = 15496, SPELL_WAR_STOMP = 11876 };
enum GrimlokEvents { EVENT_CLEAVE = 1, EVENT_WAR_STOMP };

struct boss_grimlok : public BossAI
{
    boss_grimlok(Creature* creature) : BossAI(creature, DATA_GRIMLOK) { }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); events.ScheduleEvent(EVENT_CLEAVE, 8s, 12s); events.ScheduleEvent(EVENT_WAR_STOMP, 15s, 20s); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId) { case EVENT_CLEAVE: DoCastVictim(SPELL_CLEAVE); events.Repeat(8s, 12s); break; case EVENT_WAR_STOMP: DoCastSelf(SPELL_WAR_STOMP); events.Repeat(15s, 20s); break; }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
};
void AddSC_boss_grimlok() { RegisterUldamanCreatureAI(boss_grimlok); }


#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum ReveloshSpells { SPELL_ENRAGE = 8599, SPELL_CLEAVE = 15496 };
enum ReveloshEvents { EVENT_ENRAGE = 1, EVENT_CLEAVE };

struct boss_revelosh : public BossAI
{
    boss_revelosh(Creature* creature) : BossAI(creature, DATA_REVELOSH) { }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); events.ScheduleEvent(EVENT_ENRAGE, 30s); events.ScheduleEvent(EVENT_CLEAVE, 8s, 12s); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId) { case EVENT_ENRAGE: DoCastSelf(SPELL_ENRAGE); break; case EVENT_CLEAVE: DoCastVictim(SPELL_CLEAVE); events.Repeat(8s, 12s); break; }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
};
void AddSC_boss_revelosh() { RegisterUldamanCreatureAI(boss_revelosh); }


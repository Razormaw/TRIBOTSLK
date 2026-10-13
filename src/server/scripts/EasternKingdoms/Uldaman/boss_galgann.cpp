#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum GalgannSpells { SPELL_FIREBLAST = 11981, SPELL_FLAME_SHOCK = 10458, SPELL_HAMMER = 10459, SPELL_ENRAGE = 8599 };
enum GalgannEvents { EVENT_FIREBLAST = 1, EVENT_FLAME_SHOCK, EVENT_HAMMER, EVENT_ENRAGE };

struct boss_galgann : public BossAI
{
    boss_galgann(Creature* creature) : BossAI(creature, DATA_GALGANN) { }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); events.ScheduleEvent(EVENT_FIREBLAST, 5s, 8s); events.ScheduleEvent(EVENT_FLAME_SHOCK, 10s, 15s); events.ScheduleEvent(EVENT_HAMMER, 20s, 30s); events.ScheduleEvent(EVENT_ENRAGE, 60s); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId) { case EVENT_FIREBLAST: DoCastVictim(SPELL_FIREBLAST); events.Repeat(5s, 8s); break; case EVENT_FLAME_SHOCK: DoCastVictim(SPELL_FLAME_SHOCK); events.Repeat(10s, 15s); break; case EVENT_HAMMER: DoCastVictim(SPELL_HAMMER); events.Repeat(20s, 30s); break; case EVENT_ENRAGE: DoCastSelf(SPELL_ENRAGE); break; }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
};
void AddSC_boss_galgann() { RegisterUldamanCreatureAI(boss_galgann); }


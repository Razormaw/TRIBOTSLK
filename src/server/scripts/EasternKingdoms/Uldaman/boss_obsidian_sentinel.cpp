#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum ObsidianSpells { SPELL_SHIELD = 13022, SPELL_STONE_FIST = 11428, SPELL_GROUND_SLAM = 8538 };
enum ObsidianEvents { EVENT_SHIELD = 1, EVENT_STONE_FIST, EVENT_GROUND_SLAM };

struct boss_obsidian_sentinel : public BossAI
{
    boss_obsidian_sentinel(Creature* creature) : BossAI(creature, DATA_OBSIDIAN_SENTINEL) { }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); events.ScheduleEvent(EVENT_SHIELD, 20s, 30s); events.ScheduleEvent(EVENT_STONE_FIST, 10s, 15s); events.ScheduleEvent(EVENT_GROUND_SLAM, 15s, 25s); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId) { case EVENT_SHIELD: DoCastSelf(SPELL_SHIELD); events.Repeat(20s, 30s); break; case EVENT_STONE_FIST: DoCastVictim(SPELL_STONE_FIST); events.Repeat(10s, 15s); break; case EVENT_GROUND_SLAM: DoCastSelf(SPELL_GROUND_SLAM); events.Repeat(15s, 25s); break; }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
};
void AddSC_boss_obsidian_sentinel() { RegisterUldamanCreatureAI(boss_obsidian_sentinel); }


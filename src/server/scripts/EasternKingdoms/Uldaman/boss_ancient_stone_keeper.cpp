#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum AncientSpells { SPELL_STONE_FORM = 10256, SPELL_CRUSHING_BLOW = 10257, SPELL_STONE_SHATTER = 10258 };
enum AncientEvents { EVENT_STONE_FORM = 1, EVENT_CRUSHING_BLOW, EVENT_STONE_SHATTER };

struct boss_ancient_stone_keeper : public BossAI
{
    boss_ancient_stone_keeper(Creature* creature) : BossAI(creature, DATA_ANCIENT_STONE_KEEPER) { }
    void JustEngagedWith(Unit* who) override { BossAI::JustEngagedWith(who); events.ScheduleEvent(EVENT_STONE_FORM, 25s, 35s); events.ScheduleEvent(EVENT_CRUSHING_BLOW, 8s, 12s); events.ScheduleEvent(EVENT_STONE_SHATTER, 15s, 20s); }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim()) return;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId) { case EVENT_STONE_FORM: DoCastSelf(SPELL_STONE_FORM); events.Repeat(25s, 35s); break; case EVENT_CRUSHING_BLOW: DoCastVictim(SPELL_CRUSHING_BLOW); events.Repeat(8s, 12s); break; case EVENT_STONE_SHATTER: DoCastSelf(SPELL_STONE_SHATTER); events.Repeat(15s, 20s); break; }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
};
void AddSC_boss_ancient_stone_keeper() { RegisterUldamanCreatureAI(boss_ancient_stone_keeper); }


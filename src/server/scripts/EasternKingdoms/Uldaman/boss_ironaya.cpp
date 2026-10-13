#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ScriptedCreature.h"
#include "uldaman.h"
using namespace std::chrono_literals;

enum IronayaTexts { SAY_INTRO = 0 };
enum IronayaSpells { SPELL_ARCING_SMASH = 8374, SPELL_KNOCK_AWAY = 10101, SPELL_WAR_STOMP = 11876 };
enum IronayaEvents { EVENT_INTRO = 1, EVENT_ARCING_SMASH, EVENT_KNOCK_AWAY, EVENT_WAR_STOMP };
enum IronayaMisc { PATH_INTRO = 6 };

struct boss_ironaya : public BossAI
{
    boss_ironaya(Creature* creature) : BossAI(creature, DATA_IRONAYA) { }
    void JustAppeared() override { events.ScheduleEvent(EVENT_INTRO, 25s); }
    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(EVENT_ARCING_SMASH, 0s, 5s);
        events.ScheduleEvent(EVENT_KNOCK_AWAY, 20s, 30s);
        events.ScheduleEvent(EVENT_WAR_STOMP, 20s, 30s);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_INTRO) { Talk(SAY_INTRO); me->SetFaction(FACTION_TITAN_HOSTILE); me->GetMotionMaster()->MovePath(PATH_INTRO, false); instance->SetData(DATA_IRONAYA_INTRO, DONE); }
            }
            return;
        }
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_ARCING_SMASH: DoCastSelf(SPELL_ARCING_SMASH); events.Repeat(0s, 5s); break;
                case EVENT_KNOCK_AWAY: DoCastVictim(SPELL_KNOCK_AWAY); events.Repeat(20s, 30s); break;
                case EVENT_WAR_STOMP: DoCastSelf(SPELL_WAR_STOMP); events.Repeat(20s, 30s); break;
            }
            if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        }
        DoMeleeAttackIfReady();
    }
};
void AddSC_boss_ironaya() { RegisterUldamanCreatureAI(boss_ironaya); }


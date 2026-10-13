/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackfathom_deeps.h"

using namespace std::chrono_literals;

// NOTA: NPC_AKU_MAI_SERVANT ya está definido en blackfathom_deeps.h, no lo repetimos aquí.
enum AkuMaiSpells
{
    SPELL_POISON_CLOUD     = 3815,
    SPELL_FRENZIED_RAGE    = 3490,
    SPELL_CLEAVE           = 15496,
    SPELL_POISON_SPIT      = 51646
};

enum AkuMaiEvents
{
    EVENT_POISON_CLOUD = 1,
    EVENT_CLEAVE,
    EVENT_POISON_SPIT,
    EVENT_SUMMON_ADDS
};

struct boss_aku_mai : public BossAI
{
    boss_aku_mai(Creature* creature) : BossAI(creature, DATA_AKU_MAI), _isEnraged(false) { }

    void Reset() override
    {
        _Reset();
        _isEnraged = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        events.ScheduleEvent(EVENT_CLEAVE, 5s, 8s);
        events.ScheduleEvent(EVENT_POISON_SPIT, 3s, 6s);
        events.ScheduleEvent(EVENT_POISON_CLOUD, 15s, 20s);
    }

    void DamageTaken(Unit*, uint32& damage, DamageEffectType, SpellInfo const*) override
    {
        if (!_isEnraged && me->HealthBelowPctDamaged(30, damage))
        {
            _isEnraged = true;
            DoCastSelf(SPELL_FRENZIED_RAGE);
            
            // Mecánica dinámica: invoca adds al enfurecerse
            events.ScheduleEvent(EVENT_SUMMON_ADDS, 2s);
            events.ScheduleEvent(EVENT_SUMMON_ADDS, 15s); // Segunda oleada si no los matan rápido
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
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
                case EVENT_CLEAVE:
                    DoCastVictim(SPELL_CLEAVE);
                    events.ScheduleEvent(EVENT_CLEAVE, 8s, 12s);
                    break;
                case EVENT_POISON_SPIT:
                    DoCastVictim(SPELL_POISON_SPIT);
                    events.ScheduleEvent(EVENT_POISON_SPIT, 6s, 9s);
                    break;
                case EVENT_POISON_CLOUD:
                    DoCastAOE(SPELL_POISON_CLOUD);
                    events.ScheduleEvent(EVENT_POISON_CLOUD, 20s, 30s);
                    break;
                case EVENT_SUMMON_ADDS:
                    if (_isEnraged)
                    {
                        // Calculamos las coordenadas manualmente para evitar GetRelativePosition
                        float x = me->GetPositionX();
                        float y = me->GetPositionY();
                        float z = me->GetPositionZ();
                        
                        // Invoca 2 sirvientes a los lados del jefe
                        me->SummonCreature(NPC_AKU_MAI_SERVANT, x + 5.0f, y + 2.0f, z, 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 45000ms);
                        me->SummonCreature(NPC_AKU_MAI_SERVANT, x + 5.0f, y - 2.0f, z, 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 45000ms);
                    }
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    bool _isEnraged;
};

void AddSC_boss_aku_mai()
{
    RegisterBlackfathomDeepsCreatureAI(boss_aku_mai);
}


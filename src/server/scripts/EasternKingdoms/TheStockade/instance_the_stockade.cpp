/*
 * This file is part of the TrinityCore Project.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "the_stockades.h"

class instance_the_stockade : public InstanceMapScript
{
public:
    instance_the_stockade() : InstanceMapScript("instance_the_stockade", 34) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_the_stockade_InstanceMapScript(map);
    }

    struct instance_the_stockade_InstanceMapScript : public InstanceScript
    {
        instance_the_stockade_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_TARGORR:             _targorrGUID = creature->GetGUID(); break;
                case NPC_KAM_DEEPFURY:        _kamGUID = creature->GetGUID(); break;
                case NPC_HAMHOCK:             _hamhockGUID = creature->GetGUID(); break;
                case NPC_DEXTREN_WARD:        _dextrenGUID = creature->GetGUID(); break;
                case NPC_BAZIL_THREDD:        _bazilGUID = creature->GetGUID(); break;
                case NPC_BRUEGAL_IRONKNUCKLE: _bruegalGUID = creature->GetGUID(); break;
            }
        }

        ObjectGuid GetGuidData(uint32 data) const override
        {
            switch (data)
            {
                case DATA_TARGORR:             return _targorrGUID;
                case DATA_KAM_DEEPFURY:        return _kamGUID;
                case DATA_HAMHOCK:             return _hamhockGUID;
                case DATA_DEXTREN_WARD:        return _dextrenGUID;
                case DATA_BAZIL_THREDD:        return _bazilGUID;
                case DATA_BRUEGAL_IRONKNUCKLE: return _bruegalGUID;
            }
            return ObjectGuid::Empty;
        }

    private:
        ObjectGuid _targorrGUID;
        ObjectGuid _kamGUID;
        ObjectGuid _hamhockGUID;
        ObjectGuid _dextrenGUID;
        ObjectGuid _bazilGUID;
        ObjectGuid _bruegalGUID;
    };
};

void AddSC_instance_the_stockade()
{
    new instance_the_stockade();
}


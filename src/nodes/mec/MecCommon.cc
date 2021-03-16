#include "MecCommon.h"

Trigger getTrigger(std::string& trigger)
{
    if(trigger.compare("L2_MEAS_PERIODICAL") == 0)
        return L2_MEAS_PERIODICAL;
    if(trigger.compare("L2_MEAS_UTI_80") == 0)
        return L2_MEAS_UTI_80;
    if(trigger.compare("L2_MEAS_DL_TPU_1") == 0)
        return L2_MEAS_DL_TPU_1;
    if(trigger.compare("UE_CQI") == 0)
        return UE_CQI;

    throw omnetpp::cRuntimeError("getTrigger - trigger %s not exist", trigger.c_str());
}

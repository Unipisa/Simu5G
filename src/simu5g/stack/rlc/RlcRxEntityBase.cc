#include "simu5g/stack/rlc/RlcRxEntityBase.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

void RlcRxEntityBase::handleMessage(cMessage *msg)
{
    throw cRuntimeError("RlcRxEntityBase::handleMessage - must be overridden");
}

void RlcRxEntityBase::setFlowControlInfo(FlowControlInfo *info)
{
    flowControlInfo_ = info->dup();
}

} // namespace simu5g

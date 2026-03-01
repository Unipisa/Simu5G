#include "simu5g/stack/rlc/RlcTxEntityBase.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

void RlcTxEntityBase::handleMessage(cMessage *msg)
{
    throw cRuntimeError("RlcTxEntityBase::handleMessage - must be overridden");
}

void RlcTxEntityBase::setFlowControlInfo(FlowControlInfo *info)
{
    flowControlInfo_ = info->dup();
}

} // namespace simu5g

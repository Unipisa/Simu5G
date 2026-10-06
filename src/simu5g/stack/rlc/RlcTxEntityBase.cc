//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rlc/RlcTxEntityBase.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

void RlcTxEntityBase::handleMessage(cMessage *msg)
{
    throw cRuntimeError("RlcTxEntityBase::handleMessage - must be overridden");
}

void RlcTxEntityBase::setFlowControlInfo(FlowControlInfo *info)
{
    delete flowControlInfo_;
    flowControlInfo_ = info->dup();
}

} // namespace simu5g

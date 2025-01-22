//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "PacketFlowManagerBase.h"

#include <inet/common/ModuleAccess.h>

#include "stack/mac/LteMacBase.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"
#include "stack/rlc/LteRlcDefs.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"

#include "common/LteControlInfo.h"

#include <sstream>

namespace simu5g {

//Define_Module(PacketFlowManagerBase);



void PacketFlowManagerBase::initialize(int stage)
{
    if (stage == 1) {
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
        nodeType_ = mac->getNodeType();
        harqProcesses_ = (nodeType_ == UE) ? UE_TX_HARQ_PROCESSES : ENB_TX_HARQ_PROCESSES;
        pfmType = par("pfmType").stringValue();
    }
}

void PacketFlowManagerBase::resetDiscardCounter()
{
    pktDiscardCounterTotal_ = { 0, 0 };
}

void PacketFlowManagerBase::finish()
{
}

} //namespace


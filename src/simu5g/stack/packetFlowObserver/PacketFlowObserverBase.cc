//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <sstream>
#include <inet/common/ModuleAccess.h>
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/pdcp/LtePdcp.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/common/LteControlInfo.h"
#include "PacketFlowObserverBase.h"

namespace simu5g {

void PacketFlowObserverBase::initialize(int stage)
{
    if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
        nodeType_ = mac->getNodeType();
        harqProcesses_ = (nodeType_ == UE) ? UE_TX_HARQ_PROCESSES : ENB_TX_HARQ_PROCESSES;
        pfmType = par("pfmType").stringValue();
    }
}

void PacketFlowObserverBase::resetDiscardCounter()
{
    pktDiscardCounterTotal_ = { 0, 0 };
}

} //namespace

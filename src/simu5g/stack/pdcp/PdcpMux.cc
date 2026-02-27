//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/pdcp/PdcpMux.h"
#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/LowerMux.h"

namespace simu5g {

Define_Module(PdcpMux);

using namespace omnetpp;

PdcpMux::~PdcpMux()
{
}

void PdcpMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperMux_ = check_and_cast<UpperMux *>(getParentModule()->getSubmodule("upperMux"));
        lowerMux_ = check_and_cast<LowerMux *>(getParentModule()->getSubmodule("lowerMux"));
    }
}

PdcpTxEntityBase *PdcpMux::lookupTxEntity(DrbKey id) { return upperMux_->lookupTxEntity(id); }
PdcpTxEntityBase *PdcpMux::createTxEntity(DrbKey id) { return upperMux_->createTxEntity(id); }
PdcpRxEntityBase *PdcpMux::lookupRxEntity(DrbKey id) { return lowerMux_->lookupRxEntity(id); }
PdcpRxEntityBase *PdcpMux::createRxEntity(DrbKey id) { return lowerMux_->createRxEntity(id); }
PdcpTxEntityBase *PdcpMux::createBypassTxEntity(DrbKey id) { return lowerMux_->createBypassTxEntity(id); }
PdcpRxEntityBase *PdcpMux::createBypassRxEntity(DrbKey id) { return lowerMux_->createBypassRxEntity(id); }

void PdcpMux::deleteEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();
    upperMux_->deleteTxEntities(nodeId);
    lowerMux_->deleteRxAndBypassEntities(nodeId);
}

void PdcpMux::activeUeUL(std::set<MacNodeId> *ueSet)
{
    lowerMux_->activeUeUL(ueSet);
}

} //namespace

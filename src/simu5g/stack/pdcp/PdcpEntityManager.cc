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

#include "simu5g/stack/pdcp/PdcpEntityManager.h"
#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/LowerMux.h"

namespace simu5g {

Define_Module(PdcpEntityManager);

using namespace omnetpp;

PdcpEntityManager::~PdcpEntityManager()
{
}

void PdcpEntityManager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        upperMux_ = check_and_cast<UpperMux *>(getParentModule()->getSubmodule("upperMux"));
        lowerMux_ = check_and_cast<LowerMux *>(getParentModule()->getSubmodule("lowerMux"));
    }
}

PdcpTxEntityBase *PdcpEntityManager::lookupTxEntity(DrbKey id) { return upperMux_->lookupTxEntity(id); }
PdcpTxEntityBase *PdcpEntityManager::createTxEntity(DrbKey id) { return upperMux_->createTxEntity(id); }
PdcpRxEntityBase *PdcpEntityManager::lookupRxEntity(DrbKey id) { return lowerMux_->lookupRxEntity(id); }
PdcpRxEntityBase *PdcpEntityManager::createRxEntity(DrbKey id) { return lowerMux_->createRxEntity(id); }
PdcpTxEntityBase *PdcpEntityManager::createBypassTxEntity(DrbKey id) { return lowerMux_->createBypassTxEntity(id); }
PdcpRxEntityBase *PdcpEntityManager::createBypassRxEntity(DrbKey id) { return lowerMux_->createBypassRxEntity(id); }

void PdcpEntityManager::deleteEntities(MacNodeId nodeId)
{
    Enter_Method_Silent();
    upperMux_->deleteTxEntities(nodeId);
    lowerMux_->deleteRxAndBypassEntities(nodeId);
}

void PdcpEntityManager::activeUeUL(std::set<MacNodeId> *ueSet)
{
    lowerMux_->activeUeUL(ueSet);
}

} //namespace

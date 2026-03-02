//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/rrc/Registration.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/RlcEntityManager.h"
#include "simu5g/stack/pdcp/PdcpEntityManager.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

using namespace inet;

Define_Module(BearerManagement);

void BearerManagement::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        registration_ = check_and_cast<Registration *>(getParentModule()->getSubmodule("registration"));

        pdcpModule.reference(this, "pdcpModule", true);
        rlcUmModule.reference(this, "rlcUmModule", true);
        nrRlcUmModule.reference(this, "nrRlcUmModule", false);
        macModule.reference(this, "macModule", true);
        nrMacModule.reference(this, "nrMacModule", false);
    }
}

void BearerManagement::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

void BearerManagement::createIncomingConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    Enter_Method_Silent("createIncomingConnection()");

    EV << "BearerManagement::createIncomingConnection - " << " srcId=" << lteInfo->getSourceId() << " destId=" << lteInfo->getDestId()
        << " groupId=" << lteInfo->getMulticastGroupId() << " drbId=" << lteInfo->getDrbId()
        << " direction=" << dirToA(lteInfo->getDirection())
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(lteInfo->getDestId() == registration_->getLteNodeId() || lteInfo->getDestId() == registration_->getNrNodeId() || lteInfo->getMulticastGroupId() != NODEID_NONE);

    // Create MAC incoming connection
    FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
    MacNodeId senderId = desc.getSourceId();
    auto mac = (registration_->getNodeType()==UE && isNrUe(lteInfo->getDestId())) ? nrMacModule.get() : macModule.get(); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    LogicalCid lcid = mac->drbIdToLcid(desc.getDrbId());
    MacCid cid = MacCid(senderId, lcid);
    mac->createIncomingConnection(cid, desc);

    // RLC: only UM works
    DrbKey rlcId = ctrlInfoToRxDrbKey(lteInfo);
    auto rlcUm = (registration_->getNodeType()==UE && isNrUe(lteInfo->getDestId())) ? nrRlcUmModule.get() : rlcUmModule.get(); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    rlcUm->createRxBuffer(rlcId, lteInfo);

    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        pdcpModule->createRxEntity(id);
    }
    else {
        // DC secondary node: create bypass RX entity (forwards UL to master via X2)
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        pdcpModule->createBypassRxEntity(id);
    }
}

void BearerManagement::createOutgoingConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    Enter_Method_Silent("createOutgoingConnection()");

    EV << "BearerManagement::createOutgoingConnection - " << " srcId=" << lteInfo->getSourceId() << " destId=" << lteInfo->getDestId()
        << " groupId=" << lteInfo->getMulticastGroupId() << " drbId=" << lteInfo->getDrbId()
        << " direction=" << dirToA(lteInfo->getDirection())
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(lteInfo->getSourceId() == registration_->getLteNodeId() || lteInfo->getSourceId() == registration_->getNrNodeId());

    // Create MAC outgoing connection
    FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
    MacNodeId destId = desc.getDestId();
    auto mac = (registration_->getNodeType()==UE && isNrUe(lteInfo->getSourceId())) ? nrMacModule.get() : macModule.get();
    LogicalCid lcid = mac->drbIdToLcid(desc.getDrbId());
    MacCid cid = MacCid(destId, lcid);
    mac->createOutgoingConnection(cid, desc);

    // RLC: only UM works
    DrbKey rlcId = ctrlInfoToTxDrbKey(lteInfo);
    auto rlcUm = (registration_->getNodeType()==UE && isNrUe(lteInfo->getSourceId())) ? nrRlcUmModule.get() : rlcUmModule.get();
    rlcUm->createTxBuffer(rlcId, lteInfo);

    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        pdcpModule->createTxEntity(id);
    }
    else {
        // DC secondary node: create bypass TX entity (forwards DL from master to RLC)
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        pdcpModule->createBypassTxEntity(id);
    }
}

} // namespace simu5g

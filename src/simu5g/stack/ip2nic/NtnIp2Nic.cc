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

#include "simu5g/stack/ip2nic/NtnIp2Nic.h"

#include <inet/common/ModuleAccess.h>

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnIp2Nic);

void NtnIp2Nic::initialize(int stage)
{
    Ip2Nic::initialize(stage);

    if (stage == INITSTAGE_SIMU5G_NODE_RELATIONSHIPS && nodeType_ == NODEB) {
        if (!registerNtnAssociation(false))
            // Dynamic satellites may register after this init stage, so retry at the first event.
            scheduleAt(SIMTIME_ZERO, new cMessage("registerNtnAssociation"));
    }
}

void NtnIp2Nic::handleMessage(cMessage *msg)
{
    if (msg->isName("registerNtnAssociation")) {
        delete msg;
        registerNtnAssociation(true);
        return;
    }

    Ip2Nic::handleMessage(msg);
}

bool NtnIp2Nic::registerNtnAssociation(bool throwOnMissing)
{
    cModule *bs = inet::getContainingNode(this);
    MacNodeId ntnGatewayId = MacNodeId(bs->par("ntnGatewayId").intValue());
    MacNodeId satelliteId = MacNodeId(bs->par("satelliteId").intValue());

    if (!binder_->nodeExists(ntnGatewayId) || !binder_->nodeExists(satelliteId)) {
        if (throwOnMissing)
            throw cRuntimeError("NtnIp2Nic::registerNtnAssociation - NTN gateway %hu or satellite %hu is not registered",
                    num(ntnGatewayId), num(satelliteId));
        return false;
    }

    binder_->setGnbNtnAssociation(nodeId_, ntnGatewayId, satelliteId, true);
    return true;
}

} // namespace simu5g

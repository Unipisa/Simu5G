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

#include "simu5g/stack/pdcp/DcMux.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/dcX2Forwarder/X2DcTunnelInd_m.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/NetworkInterface.h>

namespace simu5g {

Define_Module(DcMux);

void DcMux::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macNodeId").intValue());

        bearerManagement_ = inet::getModuleFromPar<BearerManagement>(par("bearerManagementModule"), this);

        dcManagerInGate_ = gate("dcManagerIn");
    }
}

void DcMux::handleMessage(cMessage *msg)
{
    cGate *incoming = msg->getArrivalGate();
    if (incoming == dcManagerInGate_) {
        // Incoming from DC manager via X2
        auto pkt = check_and_cast<inet::Packet *>(msg);
        auto tag = pkt->removeTag<X2SourceNodeInd>();
        MacNodeId sourceNode = tag->getSourceNode();

        // The bearer's X2-U tunnel names the bearer, by the UE's id on the stack this node
        // serves; the PDU's flow identity at this node follows from it
        auto tunnel = pkt->removeTag<X2DcTunnelInd>();
        DrbKey id = DrbKey(tunnel->getUeNodeId(), tunnel->getDrbId());
        auto lteInfo = pkt->addTag<FlowControlInfo>();
        lteInfo->setDirection(tunnel->getDirection());
        lteInfo->setDrbId(tunnel->getDrbId());
        if (tunnel->getDirection() == DL) {
            // DL: master sent data for a UE — dispatch to this bearer's X2 relay, which
            // hands it to the RLC of this node's leg
            lteInfo->setSourceId(nodeId_);
            lteInfo->setDestId(id.getNodeId());
            cModule *relay = bearerManagement_->lookupPdcpRelayEntityModule(id);
            ASSERT(relay != nullptr);

            EV << NOW << " DcMux::handleMessage - Received DL PDCP PDU from master node " << sourceNode
               << " for UE " << id.getNodeId() << " - dispatching to the X2 relay entity" << endl;
            // path start = our toRelayEntity gate (crosses the compound boundary into its DL relay)
            send(pkt, relay->gate("x2In")->getPathStartGate());
        }
        else {
            // UL: secondary forwarded UL data — dispatch directly to RX entity. Above the
            // compound's joiner the PDU belongs to the bearer, whose identity at this node
            // is the served stack's pair, so a secondary-leg PDU is indistinguishable from
            // an anchor-leg one there (SDAP keys its RX lookup by the source id)
            lteInfo->setSourceId(id.getNodeId());
            lteInfo->setDestId(nodeId_);
            cModule *pdcpEnt = bearerManagement_->lookupPdcpEntityModule(id);
            ASSERT(pdcpEnt != nullptr);

            EV << NOW << " DcMux::handleMessage - Received UL PDCP PDU from secondary node " << sourceNode
               << " for " << id << " - dispatching to the PDCP entity's remote leg" << endl;
            // the remote (X2) leg of the master's PDCP entity compound is the one whose
            // legIn this mux feeds (leg 1 of a split bearer, leg 0 of an SCG bearer);
            // path start = our toRxEntity gate (crosses the compound boundary)
            cGate *x2LegIn = nullptr;
            for (int i = 0; i < pdcpEnt->gateSize("legIn"); i++)
                if (pdcpEnt->gate("legIn", i)->getPathStartGate()->getOwnerModule() == this) {
                    x2LegIn = pdcpEnt->gate("legIn", i);
                    break;
                }
            ASSERT(x2LegIn != nullptr);
            send(pkt, x2LegIn->getPathStartGate());
        }
    }
    else if (incoming->isName("fromEntity")) {
        // Outgoing from PDCP entity: forward to DC manager
        send(msg, "dcManagerOut");
    }
    else {
        throw cRuntimeError("DcMux: unexpected message from gate %s", incoming->getFullName());
    }
}

} // namespace simu5g

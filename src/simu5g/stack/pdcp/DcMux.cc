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
#include "simu5g/common/binder/Binder.h"
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
        binder_.reference(this, "binderModule", true);
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

        // the bearer's X2-U tunnel names the bearer; the tag the PDU carries across X2
        // must name the same one, with no D2D fields set
        auto tunnel = pkt->removeTag<X2DcTunnelInd>();
        auto ctrlInfo = pkt->getTag<FlowControlInfo>();
        ASSERT(tunnel->getDirection() == ctrlInfo->getDirection());
        ASSERT(ctrlInfo->getD2dTxPeerId() == NODEID_NONE && ctrlInfo->getD2dRxPeerId() == NODEID_NONE && ctrlInfo->getD2dGroupId() == NODEID_NONE);
        if (ctrlInfo->getDirection() == DL) {
            // DL: master sent data for a UE — dispatch to this bearer's X2 relay
            MacNodeId destId = ctrlInfo->getDestId();
            DrbKey id = DrbKey(destId, ctrlInfo->getDrbId());
            ASSERT(DrbKey(tunnel->getUeNodeId(), tunnel->getDrbId()) == id && ctrlInfo->getSourceId() == nodeId_);
            cModule *relay = bearerManagement_->lookupPdcpRelayEntityModule(id);
            ASSERT(relay != nullptr);

            EV << NOW << " DcMux::handleMessage - Received DL PDCP PDU from master node " << sourceNode
               << " for UE " << destId << " - dispatching to the X2 relay entity" << endl;
            // path start = our toRelayEntity gate (crosses the compound boundary into its DL relay)
            send(pkt, relay->gate("x2In")->getPathStartGate());
        }
        else {
            // UL: secondary forwarded UL data — dispatch directly to RX entity
            auto lteInfo = pkt->getTag<FlowControlInfo>();
            // The PDU crossed X2 from the secondary leg, so it carries the UE's
            // secondary-facing id (the leg splitter's rewrite), while this node's
            // PDCP compounds are keyed by the id of the stack it serves itself.
            // Translate to that id whenever the carried one is not this node's to
            // serve: at an EN-DC master the NR id becomes the LTE one, at an NE-DC
            // master the LTE id becomes the NR one.
            MacNodeId sourceId = lteInfo->getSourceId();
            if (binder_->getServingNode(sourceId) != nodeId_)
                sourceId = binder_->getUeNodeId(sourceId, !isNrUe(sourceId));
            DrbKey id = DrbKey(sourceId, lteInfo->getDrbId());
            ASSERT(DrbKey(tunnel->getUeNodeId(), tunnel->getDrbId()) == id);
            cModule *pdcpEnt = bearerManagement_->lookupPdcpEntityModule(id);
            ASSERT(pdcpEnt != nullptr);

            // Above the compound's joiner the PDU belongs to the bearer, whose
            // identity at this node is the served stack's pair: rewrite the ids so
            // a secondary-leg PDU is indistinguishable from an anchor-leg one there
            // (SDAP keys its RX lookup by the source id)
            auto ctrlForUpdate = pkt->getTagForUpdate<FlowControlInfo>();
            ctrlForUpdate->setSourceId(sourceId);
            ctrlForUpdate->setDestId(nodeId_);

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

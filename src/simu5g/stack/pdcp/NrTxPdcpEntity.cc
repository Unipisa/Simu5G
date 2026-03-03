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

#include "simu5g/stack/pdcp/NrTxPdcpEntity.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"
#include <inet/networklayer/common/NetworkInterface.h>

namespace simu5g {

Define_Module(NrTxPdcpEntity);

simsignal_t NrTxPdcpEntity::pdcpSduSentNrSignal_ = registerSignal("pdcpSduSentNr");


void NrTxPdcpEntity::initialize(int stage)
{
    LteTxPdcpEntity::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        inet::NetworkInterface *nic = inet::getContainingNicModule(this);
        dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();

        if (getNodeTypeById(nodeId_) == UE) {
            nrNodeId_ = MacNodeId(getContainingNode(this)->par("nrMacNodeId").intValue());
        }
    }
}

void NrTxPdcpEntity::deliverPdcpPdu(Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    if (getNodeTypeById(nodeId_) == UE) {
        // NR UE: route to NR or LTE RLC depending on DC and useNR flag
        bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();
        if (!dualConnectivityEnabled_ || useNR) {
            EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - DRB ID[" << lteInfo->getDrbId() << "] - sending packet to NR RLC" << endl;
            // Translate to NR-leg IDs for NR RLC buffer lookup
            lteInfo->setSourceId(nrNodeId_);
            if (dualConnectivityEnabled_)
                lteInfo->setDestId(binder_->getNextHop(nrNodeId_));
            if (hasListeners(pdcpSduSentNrSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
                emit(pdcpSduSentNrSignal_, pkt);
            }
            emit(sentPacketToLowerLayerSignal_, pkt);
            // In DC, the master PDCP entity's 'out' is wired to LTE RLC;
            // use 'nrOut' gate (wired to NR RLC) for NR-leg traffic
            send(pkt, dualConnectivityEnabled_ ? "nrOut" : "out");
        }
        else {
            EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - DRB ID[" << lteInfo->getDrbId() << "] - sending packet to LTE RLC" << endl;
            if (hasListeners(pdcpSduSentSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
                emit(pdcpSduSentSignal_, pkt);
            }
            emit(sentPacketToLowerLayerSignal_, pkt);
            send(pkt, "out");
        }
    }
    else { // ENODEB
        MacNodeId destId = lteInfo->getDestId();
        if (getNodeTypeById(destId) != UE)
            throw cRuntimeError("NrTxPdcpEntity::deliverPdcpPdu - destination must be a UE");

        if (!dualConnectivityEnabled_) {
            EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - DRB ID[" << lteInfo->getDrbId() << "] - the destination is a UE. Sending packet to lower layer" << endl;
            if (hasListeners(pdcpSduSentSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
                emit(pdcpSduSentSignal_, pkt);
            }
            emit(sentPacketToLowerLayerSignal_, pkt);
            send(pkt, "out");
        }
        else {
            bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();

            if (!useNR) {
                EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - DRB ID[" << lteInfo->getDrbId() << "] useNR[" << useNR << "] - the destination is a UE. Sending packet to lower layer." << endl;
                if (hasListeners(pdcpSduSentSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
                    emit(pdcpSduSentSignal_, pkt);
                }
                emit(sentPacketToLowerLayerSignal_, pkt);
                send(pkt, "out");
            }
            else { // useNR
                EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - DRB ID[" << lteInfo->getDrbId() << "] - the destination is under the control of a secondary node" << endl;
                MacNodeId secondaryNodeId = binder_->getSecondaryNode(nodeId_);
                ASSERT(secondaryNodeId != NODEID_NONE);
                ASSERT(secondaryNodeId != nodeId_);

                // Translate to NR-leg IDs for the secondary gNB's bypass entity
                MacNodeId nrDestId = binder_->getUeNodeId(lteInfo->getDestId(), true);
                ASSERT(nrDestId != NODEID_NONE);
                lteInfo->setSourceId(secondaryNodeId);
                lteInfo->setDestId(nrDestId);

                auto tag = pkt->addTagIfAbsent<X2TargetReq>();
                tag->setTargetNode(secondaryNodeId);
                send(pkt, "dcOut");
            }
        }
    }
}

} //namespace


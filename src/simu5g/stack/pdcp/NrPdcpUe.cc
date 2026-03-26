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

#include <inet/networklayer/common/NetworkInterface.h>

#include "simu5g/stack/pdcp/NrPdcpUe.h"
#include "simu5g/stack/pdcp/NrTxPdcpEntity.h"
#include "simu5g/stack/pdcp/NrRxPdcpEntity.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowObserverBase.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(NrPdcpUe);

void NrPdcpUe::initialize(int stage)
{
    LtePdcpUeD2D::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        nrNodeId_ = MacNodeId(getContainingNode(this)->par("nrMacNodeId").intValue());

        inet::NetworkInterface *nic = inet::getContainingNicModule(this);
        dualConnectivityEnabled_ = nic->par("dualConnectivityEnabled").boolValue();

        // initialize gates
        // nrTmSapInGate_ = gate("TM_Sap$i", 1);
        nrTmSapOutGate_ = gate("TM_Sap$o", 1);
        //nrUmSapInGate_ = gate("UM_Sap$i", 1);
        nrUmSapOutGate_ = gate("UM_Sap$o", 1);
        //nrAmSapInGate_ = gate("AM_Sap$i", 1);
        nrAmSapOutGate_ = gate("AM_Sap$o", 1);
    }
}

MacNodeId NrPdcpUe::getNextHopNodeId(const Ipv4Address& destAddr, bool useNR, MacNodeId sourceId)
{
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    MacNodeId srcId = useNR ? nrNodeId_ : nodeId_;

    // check whether the destination is inside or outside the LTE network
    if (destId == NODEID_NONE || getDirection(srcId, destId) == UL) {
        // if not, the packet is destined to the eNB

        // UE is subject to handovers: master may change
        return binder_->getNextHop(sourceId);
    }

    return destId;
}

/*
 * Upper Layer handlers
 */

void NrPdcpUe::analyzePacket(inet::Packet *pkt)
{
    auto lteInfo = pkt->addTagIfAbsent<FlowControlInfo>();

    // Traffic category, RLC type
    LteTrafficClass trafficCategory = getTrafficCategory(pkt);
    LteRlcType rlcType = getRlcType(trafficCategory);
    lteInfo->setTraffic(trafficCategory);
    lteInfo->setRlcType(rlcType);

    // direction of transmitted packets depends on node type
    Direction dir = getNodeTypeById(nodeId_) == UE ? UL : DL;
    lteInfo->setDirection(dir);

    bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();

    // select the correct nodeId for the source
    MacNodeId nodeId = useNR ? nrNodeId_ : nodeId_;
    lteInfo->setSourceId(nodeId);

    // get IP flow information
    auto ipFlowInd = pkt->getTag<IpFlowInd>();
    Ipv4Address srcAddr = ipFlowInd->getSrcAddr();
    Ipv4Address destAddr = ipFlowInd->getDstAddr();
    uint16_t typeOfService = ipFlowInd->getTypeOfService();

    // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
    // else the direction will be selected according to the current status of the UE, i.e., D2D or UL
    if (destAddr.isMulticast()) {
        binder_->addD2DMulticastTransmitter(nodeId);

        lteInfo->setDirection(D2D_MULTI);

        // assign a multicast group id
        MacNodeId groupId = binder_->getOrAssignDestIdForMulticastAddress(destAddr);
        lteInfo->setMulticastGroupId(groupId);
    }
    else {
        MacNodeId destId = binder_->getMacNodeId(destAddr);
        if (destId != NODEID_NONE) { // the destination is a UE within the LTE network
            if (binder_->checkD2DCapability(nodeId, destId)) {
                // this way, we record the ID of the endpoints even if the connection is currently in IM
                // this is useful for mode switching
                lteInfo->setD2dTxPeerId(nodeId);
                lteInfo->setD2dRxPeerId(destId);
            }
            else {
                lteInfo->setD2dTxPeerId(NODEID_NONE);
                lteInfo->setD2dRxPeerId(NODEID_NONE);
            }

            // set actual flow direction based (D2D/UL) based on the current mode (DM/IM) of this pairing
            lteInfo->setDirection(getDirection(nodeId, destId));
        }
        else { // the destination is outside the LTE network
            lteInfo->setDirection(UL);
            lteInfo->setD2dTxPeerId(NODEID_NONE);
            lteInfo->setD2dRxPeerId(NODEID_NONE);
        }
    }

    lteInfo->setSourceId(useNR ? getNrNodeId() : getNodeId());

    if (lteInfo->getMulticastGroupId() != NODEID_NONE)   // destId is meaningless for multicast D2D (we use the id of the source for statistical purposes at lower levels)
        lteInfo->setDestId(getNodeId());
    else
        lteInfo->setDestId(getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId()));

    // assign LCID
    ConnectionKey key{srcAddr, destAddr, typeOfService, lteInfo->getDirection()};
    LogicalCid lcid = lookupOrAssignLcid(key);
    lteInfo->setLcid(lcid);

    EV << "NrPdcpUe : Assigned Lcid: " << lcid << "\n";
    EV << "NrPdcpUe : Assigned Node ID: " << nodeId << "\n";

    // get effective next hop dest ID
    // TODO this was in the original code, but has no effect:
    // MacNodeId destId = getNextHopNodeId(destAddr, useNR, lteInfo->getSourceId());
}

void NrPdcpUe::deleteEntities(MacNodeId nodeId)
{
    // delete connections related to the given master nodeB only
    // (the UE might have dual connectivity enabled)
    for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
        auto& [cid, txEntity] = *tit;
        if (cid.getNodeId() == nodeId) {
            txEntity->deleteModule();  // Delete Entity
            tit = txEntities_.erase(tit);       // Delete Element
        }
        else {
            ++tit;
        }
    }
    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
        auto& [cid, rxEntity] = *rit;
        if (cid.getNodeId() == nodeId) {
            rxEntity->deleteModule();  // Delete Entity
            rit = rxEntities_.erase(rit);       // Delete Element
        }
        else
            ++rit;
    }
}

void NrPdcpUe::sendToLowerLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();

    if (!dualConnectivityEnabled_ || useNR) {
        EV << "NrPdcpUe : Sending packet " << pkt->getName() << " on port "
           << (lteInfo->getRlcType() == UM ? "NR_UM_Sap$o\n" : "NR_AM_Sap$o\n");

        // use NR id as source
        lteInfo->setSourceId(nrNodeId_);

        // notify the packetFlowObserver only with UL packet
        if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
            if (NRpacketFlowObserver_ != nullptr) {
                EV << "LteTxPdcpEntity::handlePacketFromUpperLayer - notify NRpacketFlowObserver_" << endl;
                NRpacketFlowObserver_->insertPdcpSdu(pkt);
            }
        }

        // Send message
        send(pkt, (lteInfo->getRlcType() == UM ? nrUmSapOutGate_ : nrAmSapOutGate_));

        emit(sentPacketToLowerLayerSignal_, pkt);
    }
    else
        LtePdcpBase::sendToLowerLayer(pkt);
}

} //namespace

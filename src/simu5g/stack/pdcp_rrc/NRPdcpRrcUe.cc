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

#include <inet/networklayer/common/NetworkInterface.h>

#include "stack/pdcp_rrc/NRPdcpRrcUe.h"
#include "stack/pdcp_rrc/NRTxPdcpEntity.h"
#include "stack/pdcp_rrc/NRRxPdcpEntity.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

namespace simu5g {

Define_Module(NRPdcpRrcUe);

void NRPdcpRrcUe::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
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

    if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION)
        nrNodeId_ = MacNodeId(getContainingNode(this)->par("nrMacNodeId").intValue());

    LtePdcpRrcUeD2D::initialize(stage);
}

MacNodeId NRPdcpRrcUe::getDestId(inet::Ptr<FlowControlInfo> lteInfo)
{
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    MacNodeId srcId = (lteInfo->getUseNR()) ? nrNodeId_ : nodeId_;

    // check whether the destination is inside or outside the LTE network
    if (destId == NODEID_NONE || getDirection(srcId, destId) == UL) {
        // if not, the packet is destined to the eNB

        // UE is subject to handovers: master may change
        return binder_->getNextHop(lteInfo->getSourceId());
    }

    return destId;
}

/*
 * Upper Layer handlers
 */
void NRPdcpRrcUe::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayerSignal_, pktAux);

    // Control Information
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    setTrafficInformation(pkt, lteInfo);

    // select the correct nodeId for the source
    MacNodeId nodeId = (lteInfo->getUseNR()) ? nrNodeId_ : nodeId_;
    lteInfo->setSourceId(nodeId);

    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId;

    // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
    // else the direction will be selected according to the current status of the UE, i.e., D2D or UL
    if (destAddr.isMulticast()) {
        binder_->addD2DMulticastTransmitter(nodeId);

        lteInfo->setDirection(D2D_MULTI);

        // assign a multicast group id
        // multicast IP addresses are 224.0.0.0/4.
        // We consider the host part of the IP address (the remaining 28 bits) as identifier of the group,
        // so it is univocally determined for the whole network
        uint32_t address = Ipv4Address(lteInfo->getDstAddr()).getInt();
        uint32_t mask = ~((uint32_t)255 << 28);      // 0000 1111 1111 1111
        uint32_t groupId = address & mask;
        lteInfo->setMulticastGroupId((int32_t)groupId);
    }
    else {
        destId = binder_->getMacNodeId(destAddr);
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

    // Cid Request
    EV << "NRPdcpRrcUe : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
       << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
       << " , ToS: " << lteInfo->getTypeOfService()
       << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different LCIDs for different directions of the same flow are assigned.
     * The RLC layer will create different RLC entities for different LCIDs
     */

    LogicalCid mylcid;
    if ((mylcid = ht_.find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection())) == 0xFFFF) {
        // LCID not found

        // assign a new LCID to the connection
        mylcid = lcid_++;

        EV << "NRPdcpRrcUe : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_.create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection(), mylcid);
    }

    // assign LCID
    lteInfo->setLcid(mylcid);

    EV << "NRPdcpRrcUe : Assigned Lcid: " << mylcid << "\n";
    EV << "NRPdcpRrcUe : Assigned Node ID: " << nodeId << "\n";

    // get effective next hop dest ID
    destId = getDestId(lteInfo);

    // obtain CID
    MacCid cid = idToMacCid(destId, mylcid);

    // get the PDCP entity for this CID and process the packet
    LteTxPdcpEntity *entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}

LteTxPdcpEntity *NRPdcpRrcUe::getTxEntity(MacCid cid)
{
    // Find entity for this LCID
    PdcpTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end()) {
        // Not found: create
        std::stringstream buf;
        buf << "NRTxPdcpEntity Cid: " << cid;
        cModuleType *moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.NRTxPdcpEntity");
        NRTxPdcpEntity *txEnt = check_and_cast<NRTxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));

        txEntities_[cid] = txEnt;    // Add to entities map

        EV << "NRPdcpRrcUe::getEntity - Added new PdcpEntity for Cid: " << cid << "\n";

        return txEnt;
    }
    else {
        // Found
        EV << "NRPdcpRrcUe::getEntity - Using old PdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

LteRxPdcpEntity *NRPdcpRrcUe::getRxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end()) {
        // Not found: create

        std::stringstream buf;
        buf << "NRRxPdcpEntity cid: " << cid;
        cModuleType *moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.NRRxPdcpEntity");
        NRRxPdcpEntity *rxEnt = check_and_cast<NRRxPdcpEntity *>(moduleType->createScheduleInit(buf.str().c_str(), this));
        rxEntities_[cid] = rxEnt;    // Add to entities map

        EV << "NRPdcpRrcUe::getRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

        return rxEnt;
    }
    else {
        // Found
        EV << "NRPdcpRrcUe::getRxEntity - Using old RxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

void NRPdcpRrcUe::deleteEntities(MacNodeId nodeId)
{
    // delete connections related to the given master nodeB only
    // (the UE might have dual connectivity enabled)
    for (auto tit = txEntities_.begin(); tit != txEntities_.end(); ) {
        if (MacCidToNodeId(tit->first) == nodeId) {
            tit->second->deleteModule();  // Delete Entity
            tit = txEntities_.erase(tit);       // Delete Element
        }
        else {
            ++tit;
        }
    }
    for (auto rit = rxEntities_.begin(); rit != rxEntities_.end(); ) {
        if (MacCidToNodeId(rit->first) == nodeId) {
            rit->second->deleteModule();  // Delete Entity
            rit = rxEntities_.erase(rit);       // Delete Element
        }
        else
            ++rit;
    }
}

void NRPdcpRrcUe::sendToLowerLayer(Packet *pkt)
{
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    if (!dualConnectivityEnabled_ || lteInfo->getUseNR()) {
        EV << "NRPdcpRrcUe : Sending packet " << pkt->getName() << " on port "
           << (lteInfo->getRlcType() == UM ? "NR_UM_Sap$o\n" : "NR_AM_Sap$o\n");

        // use NR id as source
        lteInfo->setSourceId(nrNodeId_);

        // notify the packetFlowManager only with UL packet
        if (lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D) {
            if (NRpacketFlowManager_ != nullptr) {
                EV << "LteTxPdcpEntity::handlePacketFromUpperLayer - notify NRpacketFlowManager_" << endl;
                NRpacketFlowManager_->insertPdcpSdu(pkt);
            }
        }

        // Send message
        send(pkt, (lteInfo->getRlcType() == UM ? nrUmSapOutGate_ : nrAmSapOutGate_));

        emit(sentPacketToLowerLayerSignal_, pkt);
    }
    else
        LtePdcpRrcBase::sendToLowerLayer(pkt);
}

} //namespace


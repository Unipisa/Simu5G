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

#include "stack/pdcp_rrc/layer/NRPdcpRrcUe.h"
#include "stack/pdcp_rrc/layer/entity/NRTxPdcpEntity.h"
#include "stack/pdcp_rrc/layer/entity/NRRxPdcpEntity.h"

Define_Module(NRPdcpRrcUe);

void NRPdcpRrcUe::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        dualConnectivityEnabled_ = getAncestorPar("dualConnectivityEnabled").boolValue();

        // initialize gates
        nrTmSap_[IN_GATE] = gate("TM_Sap$i",1);
        nrTmSap_[OUT_GATE] = gate("TM_Sap$o",1);
        nrUmSap_[IN_GATE] = gate("UM_Sap$i",1);
        nrUmSap_[OUT_GATE] = gate("UM_Sap$o",1);
        nrAmSap_[IN_GATE] = gate("AM_Sap$i",1);
        nrAmSap_[OUT_GATE] = gate("AM_Sap$o",1);
    }

    if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION)
        nrNodeId_ = getAncestorPar("nrMacNodeId");

    LtePdcpRrcUeD2D::initialize(stage);
}

MacNodeId NRPdcpRrcUe::getDestId(FlowControlInfo* lteInfo)
{
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    MacNodeId srcId = (lteInfo->getUseNR())? nrNodeId_ : nodeId_;

    // check whether the destination is inside or outside the LTE network
    if (destId == 0 || getDirection(srcId,destId) == UL)
    {
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
    emit(receivedPacketFromUpperLayer, pktAux);

    // Control Information
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    setTrafficInformation(pkt, lteInfo);

    // select the correct nodeId
    MacNodeId nodeId = (lteInfo->getUseNR()) ? nrNodeId_ : nodeId_;

    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId;

    // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
    // else the direction will be selected according to the current status of the UE, i.e. D2D or UL
    if (destAddr.isMulticast())
    {
        binder_->addD2DMulticastTransmitter(nodeId);

        lteInfo->setDirection(D2D_MULTI);

        // assign a multicast group id
        // multicast IP addresses are 224.0.0.0/4.
        // We consider the host part of the IP address (the remaining 28 bits) as identifier of the group,
        // so as it is univocally determined for the whole network
        uint32 address = Ipv4Address(lteInfo->getDstAddr()).getInt();
        uint32 mask = ~((uint32)255 << 28);      // 0000 1111 1111 1111
        uint32 groupId = address & mask;
        lteInfo->setMulticastGroupId((int32)groupId);
    }
    else
    {
        destId = binder_->getMacNodeId(destAddr);
        if (destId != 0)  // the destination is a UE within the LTE network
        {
            if (binder_->checkD2DCapability(nodeId, destId))
            {
                // this way, we record the ID of the endpoints even if the connection is currently in IM
                // this is useful for mode switching
                lteInfo->setD2dTxPeerId(nodeId);
                lteInfo->setD2dRxPeerId(destId);
            }
            else
            {
                lteInfo->setD2dTxPeerId(0);
                lteInfo->setD2dRxPeerId(0);
            }

            // set actual flow direction based (D2D/UL) based on the current mode (DM/IM) of this peering
            lteInfo->setDirection(getDirection(nodeId,destId));
        }
        else  // the destination is outside the LTE network
        {
            lteInfo->setDirection(UL);
            lteInfo->setD2dTxPeerId(0);
            lteInfo->setD2dRxPeerId(0);
        }
    }

    // Cid Request
    EV << "NRPdcpRrcUe : Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
            << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
            << " , ToS: " << lteInfo->getTypeOfService()
            << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different lcid for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection())) == 0xFFFF)
    {
        // LCID not found

        // assign a new LCID to the connection
        mylcid = lcid_++;

        EV << "NRPdcpRrcUe : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection(), mylcid);
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
    LteTxPdcpEntity* entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}


LteTxPdcpEntity* NRPdcpRrcUe::getTxEntity(MacCid lcid)
{
    // Find entity for this LCID
    PdcpTxEntities::iterator it = txEntities_.find(lcid);
    if (it == txEntities_.end())
    {
        // Not found: create
        std::stringstream buf;
        buf << "NRTxPdcpEntity Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.NRTxPdcpEntity");
        NRTxPdcpEntity* txEnt = check_and_cast<NRTxPdcpEntity*>(moduleType->createScheduleInit(buf.str().c_str(), this));

        txEntities_[lcid] = txEnt;    // Add to entities map

        EV << "NRPdcpRrcUe::getEntity - Added new PdcpEntity for Lcid: " << lcid << "\n";

        return txEnt;
    }
    else
    {
        // Found
        EV << "NRPdcpRrcUe::getEntity - Using old PdcpEntity for Lcid: " << lcid << "\n";

        return it->second;
    }
}

LteRxPdcpEntity* NRPdcpRrcUe::getRxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end())
    {
        // Not found: create

        std::stringstream buf;
        buf << "NRRxPdcpEntity cid: " << cid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.NRRxPdcpEntity");
        NRRxPdcpEntity* rxEnt = check_and_cast<NRRxPdcpEntity*>(moduleType->createScheduleInit(buf.str().c_str(), this));
        rxEntities_[cid] = rxEnt;    // Add to entities map

        EV << "NRPdcpRrcUe::getRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

        return rxEnt;
    }
    else
    {
        // Found
        EV << "NRPdcpRrcUe::getRxEntity - Using old RxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

void NRPdcpRrcUe::deleteEntities(MacNodeId nodeId)
{
    PdcpTxEntities::iterator tit;
    PdcpRxEntities::iterator rit;

    // delete connections related to the given master nodeB only
    // (the UE might have dual connectivity enabled)
    for (tit = txEntities_.begin(); tit != txEntities_.end(); )
    {
        if (MacCidToNodeId(tit->first) == nodeId)
        {
            (tit->second)->deleteModule();  // Delete Entity
            txEntities_.erase(tit++);       // Delete Elem
        }
        else
        {
            ++tit;
        }
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); )
    {
        if (MacCidToNodeId(rit->first) == nodeId)
        {
            (rit->second)->deleteModule();  // Delete Entity
            rxEntities_.erase(rit++);       // Delete Elem
        }
        else
        {
            ++rit;
        }
    }
}

void NRPdcpRrcUe::sendToLowerLayer(Packet *pkt)
{

    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (!dualConnectivityEnabled_ || lteInfo->getUseNR())
    {
        EV << "NRPdcpRrcUe : Sending packet " << pkt->getName() << " on port "
           << (lteInfo->getRlcType() == UM ? "NR_UM_Sap$o\n" : "NR_AM_Sap$o\n");

        // use NR id as source
        lteInfo->setSourceId(nrNodeId_);

        // Send message
        send(pkt, (lteInfo->getRlcType() == UM ? nrUmSap_[OUT_GATE] : nrAmSap_[OUT_GATE]));

        emit(sentPacketToLowerLayer, pkt);
    }
    else
        LtePdcpRrcBase::sendToLowerLayer(pkt);
}

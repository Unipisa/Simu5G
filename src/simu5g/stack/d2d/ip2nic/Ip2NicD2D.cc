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
#include "simu5g/stack/d2d/ip2nic/Ip2NicD2D.h"
#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

Define_Module(Ip2NicD2D);

void Ip2NicD2D::initialize(int stage)
{
    Ip2Nic::initialize(stage);
    if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS)
        d2dBinder_ = D2dBinder::getInstance(this);
}

MacNodeId Ip2NicD2D::getNextHopNodeId(const L3Address& destAddr, MacNodeId sourceId)
{
    // D2D-capable UE (a base station's downlink next hop is D2D-agnostic, see
    // getDownlinkNextHopNodeId()): check if direct D2D communication is possible
    MacNodeId destId = binder_->getMacNodeId(destAddr);

    // check whether the destination is inside the LTE network and D2D is active
    if (destId == NODEID_NONE ||
        !(d2dBinder_->getD2DCapability(sourceId, destId) && d2dBinder_->getD2DMode(sourceId, destId) == DM)) {
        // packet is destined to the eNB; UE is subject to handovers: master may change
        return binder_->getServingNodeOrSelf(sourceId);
    }

    return destId;
}

void Ip2NicD2D::classifyConnection(inet::Packet *pkt, FlowControlInfo *lteInfo, const L3Address& destAddr, MacNodeId localNodeId, bool isEnb)
{
    if (isEnb) {
        // ENB: set D2D peer IDs to none
        lteInfo->setD2dTxPeerId(NODEID_NONE);
        lteInfo->setD2dRxPeerId(NODEID_NONE);
        return;
    }

    // UE: D2D multicast/unicast handling (unified for NrPdcpUe and LtePdcpUeD2D)
    if (isNr_)
        lteInfo->setSourceId(localNodeId);

    // D2D groupcast is IPv4-only: IPv6 multicast (the node's own Neighbor Discovery
    // traffic, too) goes up the PDU session like any other packet
    if (destAddr.getType() == L3Address::IPv4 && destAddr.isMulticast()) {
        d2dBinder_->addD2DMulticastTransmitter(localNodeId);
        lteInfo->setDirection(D2D_MULTI);
        MacNodeId groupId = binder_->getOrAssignDestIdForMulticastAddress(destAddr.toIpv4());
        lteInfo->setD2dGroupId(groupId);
    }
    else {
        MacNodeId destId = binder_->getMacNodeId(destAddr);
        if (destId != NODEID_NONE) { // the destination is a UE within the LTE network
            if (d2dBinder_->checkD2DCapability(localNodeId, destId)) {
                // this way, we record the ID of the endpoints even if the connection is currently in IM
                // this is useful for mode switching
                lteInfo->setD2dTxPeerId(localNodeId);
                lteInfo->setD2dRxPeerId(destId);
            }
            else {
                lteInfo->setD2dTxPeerId(NODEID_NONE);
                lteInfo->setD2dRxPeerId(NODEID_NONE);
            }

            // set actual flow direction (D2D/UL) based on the current mode (DM/IM) of this peering
            if (d2dBinder_->getD2DCapability(localNodeId, destId) && d2dBinder_->getD2DMode(localNodeId, destId) == DM)
                lteInfo->setDirection(D2D);
            else
                lteInfo->setDirection(UL);
        }
        else { // the destination is outside the LTE network
            lteInfo->setDirection(UL);
            lteInfo->setD2dTxPeerId(NODEID_NONE);
            lteInfo->setD2dRxPeerId(NODEID_NONE);
        }
    }
}

} //namespace

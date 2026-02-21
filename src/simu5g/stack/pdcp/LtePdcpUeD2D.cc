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

#include "simu5g/stack/pdcp/LtePdcpUeD2D.h"
#include <inet/networklayer/common/L3AddressResolver.h>
#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowObserverBase.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(LtePdcpUeD2D);

using namespace inet;
using namespace omnetpp;

MacNodeId LtePdcpUeD2D::getNextHopNodeId(const Ipv4Address& destAddr, bool useNR, MacNodeId sourceId)
{
    MacNodeId destId = binder_->getMacNodeId(destAddr);

    // check if the destination is inside the LTE network
    if (destId == NODEID_NONE || getDirection(destId) == UL) { // if not, the packet is destined to the eNB
        // UE is subject to handovers: master may change
        return binder_->getServingNodeOrSelf(sourceId);
    }

    return destId;
}

void LtePdcpUeD2D::initialize(int stage)
{
    LtePdcpUe::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        hasD2DSupport_ = true;
}

void LtePdcpUeD2D::handleMessage(cMessage *msg)
{
    cPacket *pktAux = check_and_cast<cPacket *>(msg);

    // check whether the message is a notification for mode switch
    if (strcmp(pktAux->getName(), "D2DModeSwitchNotification") == 0) {
        EV << "LtePdcpUeD2D::handleMessage - Received packet " << pktAux->getName() << " from port " << pktAux->getArrivalGate()->getName() << endl;

        auto pkt = check_and_cast<inet::Packet *>(pktAux);
        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());

        delete pktAux;
    }
    else {
        LtePdcpBase::handleMessage(msg);
    }
}

void LtePdcpUeD2D::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpUeD2D::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;

    // add here specific behavior for handling mode switch at the PDCP layer
}

} //namespace

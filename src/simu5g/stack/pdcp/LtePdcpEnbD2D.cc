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

#include "simu5g/stack/pdcp/LtePdcpEnbD2D.h"

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/d2dModeSelection/D2DModeSwitchNotification_m.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowObserverBase.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(LtePdcpEnbD2D);

using namespace omnetpp;
using namespace inet;

void LtePdcpEnbD2D::initialize(int stage)
{
    LtePdcpEnb::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        hasD2DSupport_ = true;
}

void LtePdcpEnbD2D::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto chunk = pkt->peekAtFront<Chunk>();

    // check whether the message is a notification for mode switch
    if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr) {
        EV << "LtePdcpEnbD2D::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();
        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());
        delete pkt;
    }
    else {
        LtePdcpEnb::handleMessage(msg);
    }
}

void LtePdcpEnbD2D::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpEnbD2D::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;

    // add here specific behavior for handling mode switch at the PDCP layer
}

} //namespace

//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/radio/CellularRadio.h"

#include "simu5g/stack/phy/packet/AirFrame_m.h"
#include "simu5g/stack/phy/radio/RadioTransmissionRequest.h"

namespace simu5g {

Define_Module(CellularRadio);

void CellularRadio::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        radioInGateId_ = findGate("radioIn");
        // frames arrive at the start of their transmission
        gate(radioInGateId_)->setDeliverImmediately(true);
        upperLayerInGateId_ = findGate("upperLayerIn");
        upperLayerOutGateId_ = findGate("upperLayerOut");
        radioMedium_.reference(this, "radioMediumModule", true);
    }
}

void CellularRadio::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        // the end of a held frame's transmission
        send(msg, upperLayerOutGateId_);
    else if (msg->getArrivalGateId() == radioInGateId_) {
        auto frame = check_and_cast<cPacket *>(msg);
        if (frame->getDuration() == 0)
            send(frame, upperLayerOutGateId_);
        else
            // held until the transmission ends; the frame keeps its scheduling priority
            scheduleAfter(frame->getDuration(), frame);
    }
    else if (msg->getArrivalGateId() == upperLayerInGateId_)
        transmit(check_and_cast<AirFrame *>(msg));
    else
        throw cRuntimeError("CellularRadio: unexpected message on gate %s", msg->getArrivalGate()->getFullName());
}

void CellularRadio::transmit(AirFrame *frame)
{
    auto request = check_and_cast<RadioTransmissionRequest *>(frame->removeControlInfo());
    if (request->broadcast)
        radioMedium_->sendToNeighbors(request->sender, this, frame, request->duration);
    else if (request->copyPerTarget) {
        for (cGate *target : request->targets)
            sendDirect(frame->dup(), 0, request->duration, target);
        delete frame;
    }
    else {
        ASSERT(request->targets.size() == 1);
        sendDirect(frame, 0, request->duration, request->targets.front());
    }
    delete request;
}

} // namespace simu5g

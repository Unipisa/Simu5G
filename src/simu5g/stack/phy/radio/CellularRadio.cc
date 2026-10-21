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

#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"
#include "simu5g/stack/phy/radio/CellularTransmitter.h"
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
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "receptionTimer") == 0) {
            // the end of a held frame's transmission
            auto frame = check_and_cast<AirFrame *>(msg->removeControlInfo());
            delete msg;
            send(frame, upperLayerOutGateId_);
        }
        else if (strcmp(msg->getName(), "transmissionTimer") == 0)
            // the end of this radio's own transmission
            delete msg;
        else
            throw cRuntimeError("CellularRadio: unknown self-message %s", msg->getName());
    }
    else if (msg->getArrivalGateId() == radioInGateId_) {
        auto frame = check_and_cast<AirFrame *>(msg);
        // the reception is evaluated where the radio is as the frame starts to arrive
        frame->setArrivalPosition(endpoint_->getCoord());
        frame->setReceptionStartTime(simTime());
        // held until the transmission ends, on a reception timer of the radio's
        // own, as INET's -- for a zero-duration beacon, an event at the same
        // instant; the timer has the frame's scheduling priority
        auto timer = new cMessage("receptionTimer");
        timer->setControlInfo(frame);
        timer->setSchedulingPriority(frame->getSchedulingPriority());
        scheduleAfter(frame->getDuration(), timer);
    }
    else if (msg->getArrivalGateId() == upperLayerInGateId_)
        transmit(check_and_cast<AirFrame *>(msg));
    else
        throw cRuntimeError("CellularRadio: unexpected message on gate %s", msg->getArrivalGate()->getFullName());
}

void CellularRadio::transmit(AirFrame *frame)
{
    auto request = check_and_cast<RadioTransmissionRequest *>(frame->removeControlInfo());
    // every frame that leaves the radio is a transmission on the medium, in the order they are created
    auto transmitter = check_and_cast<CellularTransmitter *>(getSubmodule("transmitter"));
    CellularTransmission *transmission = transmitter->createTransmission(*request, frame->getAdditionalInfo());
    // where the transmission starts travels with the frame, to every receiver
    frame->setStartPosition(transmission->startPosition);
    radioMedium_->addTransmission(transmission);
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
    // the transmission ends with an event of its own at the transmitter, as INET's radio schedules one
    scheduleAfter(request->duration, new cMessage("transmissionTimer"));
    delete request;
}

} // namespace simu5g

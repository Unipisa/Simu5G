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

#include "simu5g/stack/NtnServiceLinkNic.h"

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/phy/packet/LteAirFrame_m.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NtnServiceLinkNic);

void NtnServiceLinkNic::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
        binder_.reference(this, "binderModule", true);
}

void NtnServiceLinkNic::handleMessage(cMessage *msg)
{
    cGate *arrivalGate = msg->getArrivalGate();
    if (arrivalGate->isName("upperLayerIn")) {
        auto *pkt = check_and_cast<Packet *>(msg);
        auto *frame = check_and_cast<LteAirFrame *>(pkt->decapsulate());
        delete pkt;

        MacNodeId destId = frame->getAdditionalInfo().getDestId();
        cModule *receiver = binder_->getNodeModule(destId);
        if (receiver == nullptr) {
            delete frame;
            return;
        }

        sendDirect(frame, 0, frame->getDuration(), receiver, getReceiverGateIndex(receiver, isNrUe(destId)));
        return;
    }

    if (arrivalGate->isName("radioIn")) {
        auto *frame = check_and_cast<LteAirFrame *>(msg);
        auto *pkt = new Packet(frame->getName());
        pkt->encapsulate(frame);
        send(pkt, "upperLayerOut");
        return;
    }

    throw cRuntimeError("NtnServiceLinkNic::handleMessage - unexpected gate %s", arrivalGate->getName());
}

int NtnServiceLinkNic::getReceiverGateIndex(const cModule *receiver, bool isNr) const
{
    int gate = isNr ? receiver->findGate("nrRadioIn") : receiver->findGate("radioIn");
    if (gate < 0) {
        gate = receiver->findGate("lteRadioIn");
        if (gate < 0)
            throw cRuntimeError("receiver \"%s\" has no suitable radio input gate", receiver->getFullPath().c_str());
    }
    return gate;
}

} // namespace simu5g

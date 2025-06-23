//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "MacDrbMultiplexer.h"
#include "simu5g/common/RadioBearerTag_m.h"

namespace simu5g {

Define_Module(MacDrbMultiplexer);

void MacDrbMultiplexer::initialize()
{
}

void MacDrbMultiplexer::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    cGate *incomingGate = pkt->getArrivalGate();

    if (incomingGate == gate("macIn")) {
        auto lteInfo = pkt->getTag<FlowControlInfo>();
        int drbIndex = lteInfo->getLcid();  // Assuming LCID == DRB index
        int numDrbs = gateSize("rlcOut");
        if (drbIndex < 0 || drbIndex >= numDrbs)
            throw cRuntimeError("Invalid DRB index %d", drbIndex);
        send(pkt, "rlcOut", drbIndex);
    }
    else {
        send(pkt, "macOut");
    }
}

} //namespace

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

#include "SdapDrbMultiplexer.h"
#include "simu5g/common/RadioBearerTag_m.h"

namespace simu5g {

Define_Module(SdapDrbMultiplexer);

void SdapDrbMultiplexer::initialize()
{
}

void SdapDrbMultiplexer::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    cGate *incomingGate = pkt->getArrivalGate();

    if (incomingGate == gate("sdapIn")) {
        auto drbReqTag = pkt->getTag<RadioBearerReq>();
        int drbIndex = drbReqTag->getDrbIndex();
        int numDrbs = gateSize("pdcpOut");
        if (drbIndex < 0 || drbIndex >= numDrbs)
            throw cRuntimeError("Invalid DRB index %d", drbIndex);
        send(pkt, "pdcpOut", drbIndex);
    }
    else {
        // Packet coming from PDCP --> Forward to SDAP
        int drbIndex = incomingGate->getIndex();
        auto drbIndTag = pkt->addTagIfAbsent<RadioBearerInd>();
        drbIndTag->setDrbIndex(drbIndex);
        send(pkt, "sdapOut");
    }
}

} //namespace

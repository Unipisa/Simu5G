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

#include "PdcpToSdapMultiplexer.h"
#include "simu5g/common/RadioBearerTag_m.h"

namespace simu5g {

Define_Module(PdcpToSdapMultiplexer);

void PdcpToSdapMultiplexer::initialize()
{
    numDrbs = par("numDrbs");
}

void PdcpToSdapMultiplexer::handleMessage(cMessage *msg)
{
    cPacket *pkt = check_and_cast<cPacket *>(msg);
    cGate *incoming = pkt->getArrivalGate();

    if (incoming == gate("sdapIn")) {
        // Packet coming from SDAP --> Demux to correct PDCP
        auto pktInet = check_and_cast<inet::Packet *>(pkt);

        auto drbReqTag = pktInet->getTag<RadioBearerReq>();
        int drbIndex = drbReqTag->getDrbIndex();
        if (drbIndex >= numDrbs)
            throw cRuntimeError("PdcpToSdapMultiplexer: Invalid DRB index %d", drbIndex);

        send(pkt, "pdcpOut", drbIndex);
    }
    else {
        // Packet coming from PDCP --> Forward to SDAP
        send(pkt, "sdapOut");
    }
}

} //namespace

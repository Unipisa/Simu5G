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

#ifndef __RESPONSEAPP_H_
#define __RESPONSEAPP_H_

#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "apps/mecRequestResponseApp/packets/MecRequestResponsePacket_m.h"

namespace simu5g {

using namespace omnetpp;

class MecResponseApp : public cSimpleModule
{
    inet::UdpSocket socket;
    double coreNetworkDelay_;

    static simsignal_t recvRequestSnoSignal_;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    void handleRequest(cMessage *msg);
    void sendResponse(cMessage *msg);

};

} //namespace

#endif


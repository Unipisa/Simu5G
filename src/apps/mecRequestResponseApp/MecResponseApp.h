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

#include "apps/mecRequestResponseApp/packets/MecRequestResponsePacket_m.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace omnetpp;

class MecResponseApp : public cSimpleModule
{
    inet::UdpSocket socket;
    double coreNetworkDelay_;

    static simsignal_t recvRequestSno_;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        void handleRequest(cMessage* msg);
        void sendResponse(cMessage* msg);

};

#endif

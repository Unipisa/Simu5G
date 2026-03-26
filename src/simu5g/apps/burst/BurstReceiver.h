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

#ifndef _BURSTRECEIVER_H_
#define _BURSTRECEIVER_H_

#include <string.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/common/LteDefs.h"
#include "BurstPacket_m.h"

namespace simu5g {

using namespace inet;

class BurstReceiver : public cSimpleModule
{
    UdpSocket socket;

    int numReceived_ = 0;
    int recvBytes_ = 0;

    static simsignal_t burstRcvdPktSignal_;
    static simsignal_t burstPktDelaySignal_;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
};

} //namespace

#endif


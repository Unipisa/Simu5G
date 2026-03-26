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

#ifndef _LTE_AlertReceiver_H_
#define _LTE_AlertReceiver_H_

#include <string.h>

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/apps/alert/AlertPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class AlertReceiver : public cSimpleModule
{
    inet::UdpSocket socket;

    static simsignal_t alertDelaySignal_;
    static simsignal_t alertRcvdMsgSignal_;

    simtime_t delaySum = 0;
    long nrReceived = 0;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // utility: show current statistics above the icon
    void refreshDisplay() const override;
};

} //namespace

#endif

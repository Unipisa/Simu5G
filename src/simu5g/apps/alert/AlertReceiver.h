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

#ifndef _LTE_AlertReceiver_H_
#define _LTE_AlertReceiver_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include "apps/alert/AlertPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class AlertReceiver : public cSimpleModule
{
    inet::UdpSocket socket;

    static simsignal_t alertDelaySignal_;
    static simsignal_t alertRcvdMsgSignal_;

    simtime_t delaySum;
    long nrReceived;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // utility: show current statistics above the icon
    void refreshDisplay() const override;
};

} //namespace

#endif


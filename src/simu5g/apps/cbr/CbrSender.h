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

#ifndef _CBRSENDER_H_
#define _CBRSENDER_H_

#include <string.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "simu5g/common/LteDefs.h"
#include "CbrPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class CbrSender : public cSimpleModule
{
    inet::UdpSocket socket;

    int numFrames_ = 0;
    int frameId_ = 0;
    int packetSize_ = 0;
    simtime_t samplingTime;
    simtime_t startTime_;
    simtime_t finishTime_;

    static simsignal_t cbrGeneratedThroughputSignal_;
    static simsignal_t cbrGeneratedBytesSignal_;
    static simsignal_t cbrSentPktSignal_;

    int txBytes_ = 0;

    cMessage *sendTimer_ = nullptr;
    cMessage *initTrafficTimer_ = nullptr;

    simtime_t timestamp_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void initTraffic();
    void sendCbrPacket();

  public:
    ~CbrSender() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(cMessage *msg) override;
};

} //namespace

#endif


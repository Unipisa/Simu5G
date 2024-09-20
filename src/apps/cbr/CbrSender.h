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

#ifndef _CBRSENDER_H_
#define _CBRSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "CbrPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class CbrSender : public cSimpleModule
{
    inet::UdpSocket socket;
    //has the sender been initialized?
    bool initialized_ = false;

    cMessage *selfSource_ = nullptr;
    //sender
    int nframes_;
    int iDframe_;
    int nframesTmp_;
    int size_;
    simtime_t sampling_time;
    simtime_t startTime_;
    simtime_t finishTime_;

    static simsignal_t cbrGeneratedThroughputSignal_;
    static simsignal_t cbrGeneratedBytesSignal_;
    static simsignal_t cbrSentPktSignal_;

    int txBytes_;
    // ----------------------------

    cMessage *selfSender_ = nullptr;
    cMessage *initTraffic_ = nullptr;

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


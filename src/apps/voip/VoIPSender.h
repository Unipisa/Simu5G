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

#ifndef _VOIPSENDER_H_
#define _VOIPSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include "apps/voip/VoipPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class VoIPSender : public cSimpleModule
{
    inet::UdpSocket socket;

    //source
    simtime_t durTalk_;
    simtime_t durSil_;
    double scaleTalk_;
    double shapeTalk_;
    double scaleSil_;
    double shapeSil_;
    bool isTalk_;
    cMessage *selfSource_ = nullptr;
    //sender
    int iDtalk_;
    int nframes_;
    int iDframe_;
    int nframesTmp_;
    int size_;
    simtime_t sampling_time;

    bool silences_;

    unsigned int totalSentBytes_;
    simtime_t warmUpPer_;

    static simsignal_t voIPGeneratedThroughputSignal_;
    // ----------------------------

    cMessage *selfSender_ = nullptr;

    cMessage *initTraffic_ = nullptr;

    simtime_t timestamp_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void initTraffic();
    void talkspurt(simtime_t dur);
    void selectPeriodTime();
    void sendVoIPPacket();

  public:
    ~VoIPSender() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

};

} //namespace

#endif


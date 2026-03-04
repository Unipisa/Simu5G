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

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/apps/voip/VoipPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class VoipSender : public cSimpleModule
{
    inet::UdpSocket socket;

    //source
    simtime_t durTalk_ = 0;
    simtime_t durSil_ = 0;
    double scaleTalk_;
    double shapeTalk_;
    double scaleSil_;
    double shapeSil_;
    bool isTalk_;
    cMessage *selfSource_ = nullptr;
    //sender
    int iDtalk_ = 0;
    int nframes_ = 0;
    int iDframe_ = 0;
    int nframesTmp_ = 0;
    int size_;
    simtime_t sampling_time;

    bool silences_;

    unsigned int totalSentBytes_ = 0;
    simtime_t warmUpPer_;

    static simsignal_t voipGeneratedThroughputSignal_;
    // ----------------------------

    cMessage *selfSender_ = nullptr;

    cMessage *initTraffic_ = nullptr;

    simtime_t timestamp_ = 0;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void initTraffic();
    void talkspurt(simtime_t dur);
    void selectPeriodTime();
    void sendVoIPPacket();

  public:
    ~VoipSender() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void refreshDisplay() const override;

};

} //namespace

#endif

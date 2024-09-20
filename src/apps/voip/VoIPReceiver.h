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

#ifndef _LTE_VOIPRECEIVER_H_
#define _LTE_VOIPRECEIVER_H_

#include <list>
#include <string.h>

#include <omnetpp.h>

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "apps/voip/VoipPacket_m.h"

namespace simu5g {

using namespace omnetpp;

class VoIPReceiver : public cSimpleModule
{
    inet::UdpSocket socket;

    ~VoIPReceiver() override;

    int emodel_Ie_;
    int emodel_Bpl_;
    int emodel_A_;
    double emodel_Ro_;

    typedef std::list<VoipPacket *> PacketsList;
    PacketsList mPacketsList_;
    PacketsList mPlayoutQueue_;
    unsigned int mCurrentTalkspurt_;
    unsigned int mBufferSpace_;
    simtime_t mSamplingDelta_;
    simtime_t mPlayoutDelay_;

    bool mInit_;

    unsigned int totalRcvdBytes_;
    simtime_t warmUpPer_;

    static simsignal_t voIPFrameLossSignal_;
    static simsignal_t voIPFrameDelaySignal_;
    static simsignal_t voIPPlayoutDelaySignal_;
    static simsignal_t voIPMosSignal_;
    static simsignal_t voIPTaildropLossSignal_;
    static simsignal_t voIPPlayoutLossSignal_;
    static simsignal_t voIPJitterSignal_;
    static simsignal_t voIPReceivedThroughputSignal_;

    void finish() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    double eModel(simtime_t delay, double loss);
    void playout(bool finish);
};

} //namespace

#endif


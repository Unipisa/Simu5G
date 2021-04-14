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
#ifndef _BURSTSENDER_H_
#define _BURSTSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "BurstPacket_m.h"

using namespace inet;

class BurstSender : public cSimpleModule
{
    UdpSocket socket;
    //has the sender been initialized?
    bool initialized_;

    // timers
    cMessage* selfBurst_;
    cMessage* selfPacket_;

    //sender
    int idBurst_;
    int idFrame_;

    int burstSize_;
    int size_;
    simtime_t startTime_;
    simtime_t interBurstTime_;
    simtime_t intraBurstTime_;

    simsignal_t burstSentPkt_;
    // ----------------------------

    cMessage *selfSender_;
    cMessage *initTraffic_;

    simtime_t timestamp_;
    int localPort_;
    int destPort_;
    L3Address destAddress_;

    void initTraffic();
    void sendBurst();
    void sendPacket();

  public:
    ~BurstSender();
    BurstSender();

  protected:

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
};

#endif


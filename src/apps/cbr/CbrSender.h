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

class CbrSender : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;
    //has the sender been initialized?
    bool initialized_;

    omnetpp::cMessage* selfSource_;
    //sender
    int nframes_;
    int iDframe_;
    int nframesTmp_;
    int size_;
    omnetpp::simtime_t sampling_time;
    omnetpp::simtime_t startTime_;
    omnetpp::simtime_t finishTime_;

    static omnetpp::simsignal_t cbrGeneratedThroughtputSignal_;
    static omnetpp::simsignal_t cbrGeneratedBytesSignal_;
    static omnetpp::simsignal_t cbrSentPktSignal_;

    int txBytes_;
    // ----------------------------

    omnetpp::cMessage *selfSender_;
    omnetpp::cMessage *initTraffic_;

    omnetpp::simtime_t timestamp_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void initTraffic();
    void sendCbrPacket();

  public:
    ~CbrSender();
    CbrSender();

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(omnetpp::cMessage *msg) override;
};

#endif


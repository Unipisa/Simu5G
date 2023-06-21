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

#ifndef _TASKOFFLOADINGREQUESTER_H_
#define _TASKOFFLOADINGREQUESTER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "apps/taskOffloading/TaskOffloadingRequest_m.h"
#include "apps/taskOffloading/TaskOffloadingResponse_m.h"



class TaskOffloadingRequester : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;
    bool initialized_;

    omnetpp::cMessage* selfSource_;

    //sender
    int iDframe_;
    int size_;
    omnetpp::simtime_t sampling_time;
    omnetpp::simtime_t startTime_;
    omnetpp::simtime_t finishTime_;

    static omnetpp::simsignal_t taskOffloadingSentReq;
    static omnetpp::simsignal_t taskOffloadingRtt;

    // ----------------------------

    omnetpp::cMessage *selfSender_;
    omnetpp::cMessage *initTraffic_;

    omnetpp::simtime_t timestamp_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    int tempCounter_;

    void initTraffic();
    void sendRequest();

  public:
    ~TaskOffloadingRequester();
    TaskOffloadingRequester();

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(omnetpp::cMessage *msg) override;
    void handleResponse(omnetpp::cMessage *msg);
};

#endif


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

#ifndef _TASKOFFLOADINGRESPONDER_H_
#define _TASKOFFLOADINGRESPONDER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "apps/taskOffloading/TaskOffloadingRequest_m.h"
#include "apps/taskOffloading/TaskOffloadingResponse_m.h"

#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

class TaskOffloadingResponder : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;

    int numReceived_;
    int recvBytes_;
    int respSize_;
    bool mInit_;
    int destPort_;
    inet::L3Address destAddress_;

    inet::Packet* respPacket_;
    cMessage* processingTimer_;

    static omnetpp::simsignal_t taskOffloadingRcvdReq;

    // MEC support
    bool enableVimComputing_;
    VirtualisationInfrastructureManager* vim_;
    unsigned int appId_;

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;
};

#endif


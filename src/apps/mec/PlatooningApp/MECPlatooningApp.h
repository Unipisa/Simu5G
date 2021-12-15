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

#ifndef __MECPLATOONINGAPP_H_
#define __MECPLATOONINGAPP_H_

#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_m.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"



using namespace std;
using namespace omnetpp;


class MECPlatooningApp : public MecAppBase
{
    // temp
    double  newAcceleration_;

    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    std::string subId;


  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    virtual void finish() override;

    virtual void handleServiceMessage() override {}
    virtual void handleMp1Message() override {}

    virtual void handleUeMessage(omnetpp::cMessage *msg) override;

//    virtual void modifySubscription();
//    virtual void sendSubscription();
//    virtual void sendDeleteSubscription();

    virtual void handleSelfMessage(cMessage *msg) override;

    // handler for request to join a platoon from the UE
    void handleJoinPlatoonRequest(cMessage* msg);
    // handler for request to leave a platoon from the UE
    void handleLeavePlatoonRequest(cMessage* msg);

    void control();

//        /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override {}

  public:
    MECPlatooningApp();
    virtual ~MECPlatooningApp();

};

#endif

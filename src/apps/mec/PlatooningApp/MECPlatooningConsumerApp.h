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

#ifndef __MECPLATOONINGCONSUMERAPP_H_
#define __MECPLATOONINGCONSUMERAPP_H_

#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_m.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"


using namespace std;
using namespace omnetpp;

class MECPlatooningConsumerApp : public MecAppBase
{
    // UDP socket to communicate with the UeApp
    inet::UdpSocket ueAppSocket;
    int localUePort;

    // socket to handle communication with the provider app
    inet::UdpSocket platooningProviderAppSocket;
    int localPlatooningProviderPort;

    // address+port of the UeApp
    inet::L3Address ueAppAddress;
    int ueAppPort;

    // endpoint for contacting the Location Service
    // this is obtained by sending a GET request to the Service Registry as soon as
    // the connection with the latter has been established
    inet::L3Address locationServiceAddress_;
    int locationServicePort_;

    // endpoint for contacting the MEC platooning provider app
    inet::L3Address platooningProviderAddress_;
    int platooningProviderPort_;

    // coordinates of the last position retrieved
    double lastXposition;
    double lastYposition;
    double lastZposition;

    // when joined a platoon, stores the index of the controller
    // -1 means that this MEC app does not belong to any platoon
    int controllerIndex_;

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *msg) override;

    // @brief handler for data received from the service registry
    virtual void handleMp1Message() override;

    // @brief handler for data received from a MEC service
    virtual void handleServiceMessage() override;

    // @brief multiplexer for data received from a Client app
    virtual void handleUeMessage(omnetpp::cMessage *msg) override;

//    virtual void modifySubscription();
//    virtual void sendSubscription();
//    virtual void sendDeleteSubscription();

    // @brief sends a GET request to the Location Service to retrieve the location of the UE
    void requestLocation();

    // @brief notify the PlatooningProviderApp about the presence
    //        of this new MecApp
    void registerToPlatooningProviderApp();
    // @brief handle the response from the PlatooningProviderApp
    //        about the registration to the service
    void handleProviderRegistrationResponse(cMessage* msg);

    // @brief handler for request to join a platoon from the UE
    void handleJoinPlatoonRequest(cMessage* msg);
    // @brief handler for request to leave a platoon from the UE
    void handleLeavePlatoonRequest(cMessage* msg);
    // @brief handler for response to join a platoon from the UE
    void handleJoinPlatoonResponse(cMessage* msg);
    // @brief handler for response to leave a platoon from the UE
    void handleLeavePlatoonResponse(cMessage* msg);
    // @brief handler for message containing the new command from the controller
    void handlePlatoonCommand(cMessage* msg);

    /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;

  public:
    MECPlatooningConsumerApp();
    virtual ~MECPlatooningConsumerApp();

};

#endif

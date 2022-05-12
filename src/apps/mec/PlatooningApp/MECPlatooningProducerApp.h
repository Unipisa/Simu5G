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

#ifndef __MECPLATOONINGPRODUCERAPP_H_
#define __MECPLATOONINGPRODUCERAPP_H_

#include "omnetpp.h"

#include <queue>

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/PlatooningApp/PlatooningUtils.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_m.h"
#include "apps/mec/PlatooningApp/packets/PlatooningTimers_m.h"
#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionBase.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

class PlatoonControllerBase;
class PlatoonSelectionBase;
class CurrentPlatoonRequestTimer;
class PlatoonVehicleInfo;

typedef std::map<int, PlatoonControllerStatus *> ControllerMap;
typedef std::map<int, AppEndpoint> ProducerAppMap;
typedef std::map<int, PlatooningTimer*> PlatooningTimerMap;
typedef std::map<int, CommandInfo> CommandList;
typedef std::map<int, const ControllerMap*> GlobalAvailablePlatoons;

class MECPlatooningProducerApp : public MecAppBase
{
    friend class PlatoonControllerBase;

    int producerAppId_;

    std::map<int, std::vector<cMessage*>> instantiatingControllersQueue_;
    double mecAppInstantiationTime_;

    inet::TcpSocket *mp1Socket_;
    inet::TcpSocket *serviceSocket_;

    inet::UdpSocket platooningProducerAppsSocket_; // socket used to communicate with other MecProduderApps
    int platooningProducerAppsPort_;

    // UDP socket to communicate with the MecConsumerApps of the UEs
    inet::UdpSocket platooningConsumerAppsSocket_;
    int platooningConsumerAppsPort_;

    // UDP socket to communicate with the MecControllerApps
    inet::UdpSocket platooningControllerAppsSocket_;
    int platooningControllerAppsPort_;

    // address+port of the UeApp
    inet::L3Address ueAppAddress;
    int ueAppPort;

    // endpoint for contacting the Location Service
    // this is obtained by sending a GET request to the Service Registry as soon as
    // the connection with the latter has been established
    inet::L3Address locationServiceAddress_;
    int locationServicePort_;

    HttpBaseMessage* serviceHttpMessage;
    HttpBaseMessage* mp1HttpMessage;

    // flag to let the controller to adjust the position if timestamp does not match
    bool adjustPosition_;

    // flag to let the controller to send GET requests in a separated way
    bool sendBulk_;

    // for each registered MEC app, stores its connection endpoint info and the producerApp id where is its platoon
    std::map<int, AppEndpoint> consumerAppEndpoint_;


    // for each PlatoonProducerApp in the federation, stores its connection endpoint info
    std::map<int, ProducerAppInfo> producerAppEndpoint_;
    // maps of location of the MecProducerApps in other Mes systems
    ProducerAppMap platooningProducerApps_;

    // reference to the class running the platoon selection algorithm
    PlatoonSelectionBase* platoonSelection_;

    // index to be assigned to the next controller
    // (used for controllers created automatically, which have index >= 1000)
    int nextControllerIndex_;

    // used to instantiate new MecPlatoonControllerApps
    int mecPlatoonControllerAppIdCounter_;

    // maps of active platoon managers
    ControllerMap platoonControllers_;

    // store the scheduled control timers
    PlatooningTimerMap activeControlTimer_;

    // store the scheduled update position timers
    PlatooningTimerMap activeUpdatePositionTimer_;

    // timer upon join request to retrieve inter-producerApp platoons
    CurrentPlatoonRequestTimer *currentPlatoonRequestTimer_;
    GlobalAvailablePlatoons currentAvailablePlatoons_;
    cQueue consumerAppRequests_; // cMessage requests;
    int requestCounter_;
    double retrievePlatoonsInfoDuration_;

    int received_;
    simtime_t lastFirst;
    simtime_t lastSecond;
    simsignal_t updatesDifferences_;

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleProcessedMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleHttpMessage(int connId) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override {}

    // @brief handler for data received from the service registry
    virtual void handleMp1Message(int connId) override;

    // @brief handler for data received from a MEC service
    virtual void handleServiceMessage(int connId) override;

    // From MECPlatooningControllerApps

    // @brief handler for heartbeats from the MECPlatooningControllerApps
    void handleHeartbeat(cMessage* msg);
    //@brief handler for a "new member notification" coming from a controller
    void handleNewMemberNotification(cMessage* msg);
    //@brief handler for a "leave member notification" coming from a controller
    void handleLeaveMemberNotification(cMessage* msg);

    // @brief handler for configuration response from MECPlatooningControllerApp
    void handleControllerConfigurationResponse(cMessage* msg);

    void handleControllerNotification(cMessage* msg);

    // From MECPlatooningConsumerApps

    // @brief handler for request of the list of the available platoons
    void handleDiscoverPlatoonRequest(cMessage* msg);
    // @brief handler for request to discover and associate to the most suitable platoon
    // it creates a new platoon if needed
    void handleDiscoverAndAssociatePlatoonRequest(cMessage* msg);
    // @brief handler to request for associate a specific platoon
    void handleAssociatePlatoonRequest(cMessage* msg);

    // MECPlatooningProducerApps

    // @brief handler for the notification of a new platoon from a PlatooningProducerApp
    void handleAvailablePlatoonsRequest(cMessage* msg);

    // @brief handler for the notification of a new platoon from a PlatooningProducerApp
    void handleAvailablePlatoonsResponse(cMessage* msg);

    void handleInstantiationNotificationRequest(cMessage *msg);

    void handleInstantiationNotificationResponse(cMessage *msg);


    //@brief handler for JOIN request queue to wait for the available platoons running on the federated producerApps
    void handlePendingJoinRequest(cMessage* msg);

    //@brief upon choosing the platoon, this method sends the response to the Consumer App
    void finalizeJoinRequest(cMessage* msg, bool result, L3Address& address, int port, int controllerId);


    //@brief this function instantiate a new local MECPlatoonControllerApp
    MecAppInstanceInfo* instantiateLocalPlatoon(inet::Coord& direction, int index);


    // @brief controls all the MECPlatooningControllerApps to check the last hb received.
    // If it is too old, remove the controller from the structure
    // TODO implement
    void controlHeartbeats();

    /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;

  public:
    MECPlatooningProducerApp();
    virtual ~MECPlatooningProducerApp();
};

#endif

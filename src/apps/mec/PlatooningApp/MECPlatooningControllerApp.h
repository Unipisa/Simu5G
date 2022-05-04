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

#ifndef __MECPLATOONINGCONTROLLERAPP_H_
#define __MECPLATOONINGCONTROLLERAPP_H_

#include "omnetpp.h"

#include <queue>

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/PlatooningApp/PlatooningUtils.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_m.h"
#include "apps/mec/PlatooningApp/packets/PlatooningTimer_m.h"
#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionBase.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonVehicleInfo.h"


using namespace std;
using namespace omnetpp;

class PlatoonControllerBase;
class PlatoonSelectionBase;
class CurrentPlatoonRequestTimer;
class PlatoonVehicleInfo;

typedef std::map<int, PlatoonControllerStatus*> ControllerMap;
typedef std::map<int, AppEndpoint> ProducerAppMap;
typedef std::map<int, PlatooningTimer*> PlatooningTimerMap;
typedef std::map<int, CommandInfo> CommandList;
typedef std::map<int, PlatoonVehicleInfo> PlatoonMembersInfo;
typedef std::map<int, std::map<int, PlatoonControllerStatus *> > GlobalAvailablePlatoons;

class MECPlatooningControllerApp : public MecAppBase
{
//    friend class PlatoonLongitudinalControllerBase;
    friend class PlatoonControllerBase;
//    friend class PlatoonTransitionalControllerBase;
protected:
    int controllerId_;
    int producerAppId_;

    inet::Coord direction_;

    ControllerState state_;

    double heartbeatPeriod_;
    cMessage* heartbeatMsg_;

    // info about platoon members. The key is the ID of their MEC apps
    PlatoonMembersInfo membersInfo_;

    PlatoonVehicleInfo* maneuvringVehicle_; // one at a time
    cMessage* maneuvringVehicleJoinMsg_;

    // store the id of the vehicles sorted according to their position in the platoon
    std::list<int> platoonPositions_;

    cQueue joinRequests_; // cMessage requests;

    double minimumDistanceForManoeuvre_;

    inet::UdpSocket platooningProducerAppSocket_; // socket used to communicate with the producer App
    inet::UdpSocket platooningConsumerAppsSocket_; // socket used to communicate with the consumer Apps
    int platooningProducerAppPort_;
    int platooningConsumerAppsPort_;

    AppEndpoint platooningProducerAppEndpoint_;

    inet::TcpSocket *mp1Socket_;
    inet::TcpSocket *serviceSocket_;

    HttpBaseMessage* mp1HttpMessage;

    // address+port of the UeApp
    inet::L3Address ueAppAddress;
    int ueAppPort;

    // endpoint for contacting the Location Service
    // this is obtained by sending a GET request to the Service Registry as soon as
    // the connection with the latter has been established
    inet::L3Address locationServiceAddress_;
    int locationServicePort_;


    // flag to let the controller to adjust the position if timestamp does not match
    bool adjustPosition_;

    // flag to let the controller to send GET requests in a separated way
    bool sendBulk_;

    bool isConfigured_;

    // for each registered MEC app, stores its connection endpoint info and the producerApp id where is its platoon
    std::map<int, ConsumerAppInfo> consumerAppEndpoint_;
    // this map stores the producerApp id associated with a consumer App managed by a local platoon (useful for the leave hase)
    std::map<int, int> remoteConsumerAppToProducerApp_;

    // for each PlatoonProducerApp in the federation, stores its connection endpoint info
    std::map<int, ProducerAppInfo> producerAppEndpoint_;


    // reference to the class running the platoon selection algorithm
    PlatoonSelectionBase* platoonSelection_;
    PlatoonControllerBase* longitudinalController_;
    PlatoonControllerBase* lateralController_;

    PlatooningTimer* controlTimer_;
    PlatooningTimer* updatePositionTimer_;


    double controlPerdiodLongitudinal_;
    double controlPerdiodLateral_;

    double updatePositionTimerLongitudinal_;
    double updatePositionTimerLateral_;


    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleHttpMessage(int connId) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override {}

    // @brief handler for data received from the service registry
    virtual void handleMp1Message(int connId) override;

    // @brief handler for data received from a MEC service
    virtual void handleServiceMessage(int connId) override;


    // From MECPlatooningProducerApps

    // @brief handler for configure notifications from MECPlatooningProducerApp.
    // it configures all the controller parameter and allows the controller to handle
    // requests from the MECPlatooningConsumeApps
    void handleControllerConfiguration(cMessage* msg);

    // From MECPlatooningConsumerApps
    // @brief handler for request to join a platoon from the UE
    void handleJoinPlatoonRequest(cMessage* msg, bool fromQueue = false);

    //@brief it sends a response about a JOIN request
    void sendJoinPlatoonResponse(bool success, int platoonIndex, cMessage* msg);

    // @brief handler for request to leave a platoon from the UE
    void handleLeavePlatoonRequest(cMessage* msg);
    // @brief handler for the notification of a new platoon from a PlatooningProducerApp

    //------ handlers for inter-producerApps communication ------//
    //@brief handler for request to add a new member in a platoon
    bool addMember(int consumerAppId, L3Address& carIpAddress ,Coord& position, int producerAppId);

    // @brief handler for add a PlatooningConsumerApp to a local platoon
    void handleAddMember(cMessage* msg);
    // @brief handler for remove a PlatooningConsumerApp to a local platoon
    bool removePlatoonMember(int mecAppId);

    // @brief method to finalize a join request. It add the member to the platoon structure
    // and pops a pending join request, if present.
    // Note: only one join request at a time can be managed
    void finalizeJoinPlatoonRequest(cMessage* msg, bool success);

    // @brief add a new member to the platoon
    bool addPlatoonMember(int mecAppId, int producerAppId, inet::Coord position, inet::L3Address);

    //@brief this method sends the manoeuvre notification to the MECPlatoonConsumerApp
    void sendManoeuvreNotification(cMessage* msg);
    //@brief this method sends the queued join request notification to the MECPlatoonConsumerApp
    void sendQueuedJoinNotification(cMessage* msg);

    // @brief invoked by the mecPlatooningProviderApp to update the location of the UEs of a platoon
    void updatePlatoonPositions(std::vector<UEInfo>*);

    std::map<int, std::set<inet::L3Address> > getUeAddressList();

    // @brief used by a controller to set a timer
    void startTimer(cMessage* msg, double timeOffset);
    // @brief used by a controller to stop a  timer
    void stopTimer(PlatooningTimerType timerType);

    // @brief handler called when the longitudinal ControlTimer expires
    void handleLongitudinalControllerTimer(ControlTimer* ctrlTimer);
    // @brief handler called when the lateral ControlTimer expires
    void handleLateralControllerTimer(ControlTimer* ctrlTimer);

    // @brief this method periodically sends a heartbeat to the producer app
    void sendHeartbeatNotification();

    // @brief before manoeuvring a car related to a pending request, a new check of its position is needed.
    // This method checks if the position of the pending car became too far to star a manoeuvre action.
    void checkWaitingCarPosition();

    // @brief handler called when an UpdatePositionTimer expires
    void handleUpdatePositionTimer(UpdatePositionTimer* posTimer);

    // @brief used to require the position of the cars of a platoon on behalf of the platoon controller
    void requirePlatoonLocations(int producerAppId, const set<inet::L3Address>& ues);
    // @brief used to require the position of the cars of a platoon to the Location Service
    void sendGetRequest(int producerAppId, const std::string& ues);

    /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;

  public:
    MECPlatooningControllerApp();
    virtual ~MECPlatooningControllerApp();


    // info about platoon members. The key is the ID of their MEC apps
    PlatoonMembersInfo* getMemberInfo(){ return &membersInfo_;}
    PlatoonVehicleInfo* getManoeuvringVehicle(){ return maneuvringVehicle_;} // one at a time
    std::vector<PlatoonVehicleInfo> getPlatoonMembers();
    ControllerState getState() const { return state_;}

    PlatoonVehicleInfo* getPlatoonLeader();
    PlatoonVehicleInfo* getLastPlatoonCar();


    void switchState(ControllerState state);

    // store the id of the vehicles sorted according to their position in the platoon
    std::list<int>* getPlatoonPositions(){return &platoonPositions_;}


    bool getAdjustPositionFlag() { return adjustPosition_; }

    void adjustPositions();

};

#endif

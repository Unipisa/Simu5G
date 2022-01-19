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

#ifndef __MECPLATOONINGPROVIDERAPP_H_
#define __MECPLATOONINGPROVIDERAPP_H_

#include "omnetpp.h"

#include <queue>

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_m.h"
#include "apps/mec/PlatooningApp/packets/ControllerTimer_m.h"
#include "apps/mec/PlatooningApp/platoonSelection/PlatoonSelectionBase.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/PlatooningApp/PlatooningUtils.h"

using namespace std;
using namespace omnetpp;

class PlatoonControllerBase;
class PlatoonSelectionBase;

typedef struct
{
    inet::L3Address address;
    int port;
} MECAppEndpoint;

typedef std::map<int, PlatoonControllerBase*> ControllerMap;
typedef std::map<int, ControllerTimer*> ControllerTimerMap;
typedef std::map<int, double> CommandList;

class MECPlatooningProviderApp : public MecAppBase
{
    friend class PlatoonControllerBase;

    // UDP socket to communicate with the MecConsumerApps of the UEs
    inet::UdpSocket platooningConsumerAppsSocket;
    int platooningConsumerAppsPort;

    // address+port of the UeApp
    inet::L3Address ueAppAddress;
    int ueAppPort;

    // endpoint for contacting the Location Service
    // this is obtained by sending a GET request to the Service Registry as soon as
    // the connection with the latter has been established
    inet::L3Address locationServiceAddress_;
    int locationServicePort_;

    // for each registered MEC app, stores its connection endpoint info
    std::map<int, MECAppEndpoint> mecAppEndpoint_;

    // reference to the class running the platoon selection algorithm
    PlatoonSelectionBase* platoonSelection_;

    // index to be assigned to the next controller
    int nextControllerIndex_;

    // FIFO queue with the platoon index id of the pending requests
    std::queue<int> controllerPendingRequests;

    // maps of active platoon managers
    ControllerMap platoonControllers_;
    ControllerTimerMap activeControllerTimer_;

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleUeMessage(omnetpp::cMessage *msg) override {}

    // @brief handler for data received from the service registry
    virtual void handleMp1Message() override;

    // @brief handler for data received from a MEC service
    virtual void handleServiceMessage() override;

    // @brief handler for registration request from a MEC platooning app
    void handleRegistrationRequest(cMessage* msg);
    // @brief handler for request to join a platoon from the UE
    void handleJoinPlatoonRequest(cMessage* msg);
    // @brief handler for request to leave a platoon from the UE
    void handleLeavePlatoonRequest(cMessage* msg);

    // @brief used by a controller to set a periodic timer
    void startControllerTimer(int controllerIndex, double controlPeriod);
    // @brief used by a controller to stop a periodic timer
    void stopControllerTimer(int controllerIndex);
    // @brief handler called when a controller timer expires
    void handleControllerTimer(cMessage* msg);

    // @brief used to require the position of the cars of a platoon on behalf of the platoon controller
    void requirePlatoonLocations(int controllerIndex, const set<inet::L3Address>* ues);
    // @brief used to require the position of the cars of a platoon to the Location Service
    void sendGetRequest(const std::string& ues);

    /* TCPSocket::CallbackInterface callback methods */
    virtual void established(int connId) override;

  public:
    MECPlatooningProviderApp();
    virtual ~MECPlatooningProviderApp();

};

#endif

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

#ifndef __INET_GENERICSERVICE_H
#define __INET_GENERICSERVICE_H

#include <map>
#include <queue>
#include <vector>

#include <inet/applications/base/ApplicationBase.h>
#include <inet/common/INETDefs.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/common/lifecycle/ILifecycle.h>
#include <inet/common/lifecycle/LifecycleOperation.h>
#include <inet/common/socket/SocketMap.h>

#include "common/binder/Binder.h"
#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

using namespace omnetpp;

/**
 *
 * This class implements the general structure of an MEC Service. It holds all the TCP connections
 * with the e.g. MEC Applications and manages its lifecycle. It manages Request-Reply and Subscribe-Notify schemes.
 * Every request is inserted in the queue and executed in FIFO order. Also, subscription events are queued in a separate queue and
 * have priority. The execution times are calculated with the calculateRequestServiceTime method.
 * During initialization, it saves all the eNodeB/gNodeBs connected to the MEC Host on which the service is running.
 *
 * Each connection is managed by the SocketManager object that implements the TcpSocket::CallbackInterface
 * It must be subclassed and only the methods related to request management (e.g., handleGETRequest)
 * have to be implemented.
 *
 * Note:
 * currently, subscription mode is implemented using the same socket that the client used to create the subscription (i.e., the POST request).
 * A more flexible solution, i.e., usage of the endpoint specified in the POST request, is planned to be implemented soon.
 *
 */

#define REQUEST_RNG         0
#define SUBSCRIPTION_RNG    1

class SocketManager;
class SubscriptionBase;
class HttpRequestMessage;
class ServiceRegistry;
class MecPlatformManager;
class EventNotification;

class MecServiceBase : public inet::ApplicationBase, public inet::TcpSocket::ICallback
{
  protected:
    std::string serviceName_;
    inet::TcpSocket serverSocket; // Used to listen to incoming connections
    inet::SocketMap socketMap; // Stores the connections
    typedef std::set<SocketManager *, simu5g::utils::cModule_LessId> ThreadSet;
    ThreadSet threadSet;
    std::string host_;
    inet::ModuleRefByPar<Binder> binder_;
    opp_component_ptr<cModule> meHost_;

    MecPlatformManager *mecPlatformManager_ = nullptr;
    ServiceRegistry *servRegistry_ = nullptr;

    std::string baseUriQueries_;
    std::string baseUriSubscriptions_;
    std::string baseSubscriptionLocation_;

    /*
     * Load generator variables
     * the current implementation assumes a M/M/1 system
     */
    bool loadGenerator_ = false;
    double lambda_; // arrival rate of a BG request from a BG app
    double beta_; // arrival rate of a BG request from a BG app

    int numBGApps_; // number of BG apps
    double rho_ = 0;
    simtime_t lastFGRequestArrived_ = 0;

    unsigned int subscriptionId_ = 0; // identifier for new subscriptions

    // currently not used
    std::set<std::string> supportedQueryParams_;
    std::set<std::string> supportedSubscriptionParams_;

    typedef std::map<unsigned int, SubscriptionBase *> Subscriptions;
    Subscriptions subscriptions_; // list of all active subscriptions

    std::set<cModule *, simu5g::utils::cModule_LessId> eNodeB_;     // eNodeBs connected to the MEC Host

    int requestQueueSize_;

    HttpRequestMessage *currentRequestMessageServed_ = nullptr;

    cMessage *requestService_ = nullptr;
    double requestServiceTime_;
    cQueue requests_;               // queue that holds incoming requests

    cMessage *subscriptionService_ = nullptr;
    double subscriptionServiceTime_;
    int subscriptionQueueSize_;
    std::queue<EventNotification *> subscriptionEvents_;          // queue that holds events related to subscriptions
    EventNotification *currentSubscriptionServed_ = nullptr;

    // signals for statistics
    static simsignal_t requestQueueSizeSignal_;
    static simsignal_t responseTimeSignal_;

    /*
     * This method is called for every request in the requests_ queue.
     * It checks if the receiver is still connected and in case manages the request
     */
    virtual bool manageRequest();

    /*
     * This method is called for every element in the subscriptions_ queue.
     */
    virtual bool manageSubscription();

    /*
     * This method checks the queue lengths and in case it simulates a request/subscription
     * execution time.
     * Subscriptions have precedence over requests
     *
     * if the parameter is true -> scheduleAt(NOW)
     *
     * @param now when to send the next event
     */
    virtual void scheduleNextEvent(bool now = false);

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessageWhenUp(cMessage *msg) override;
    void finish() override;
    void refreshDisplay() const override;

    /*
     * This method finds all the BSs connected to the MEC Host hosting the service
     */
    virtual void getConnectedBaseStations();

    /*
     * This method calls the handle method according to the HTTP verb
     * e.g. GET --> handleGetRequest()
     * These methods have to be implemented by the MEC services
     */
    virtual void handleCurrentRequest(inet::TcpSocket *socket);

    /*
     * This method manages the situation of an incoming request when
     * the request queue is full. It responds with an HTTP 503
     */
    virtual void handleRequestQueueFull(HttpRequestMessage *msg);

    /*
     * This method calculates the service time of the request based on:
     *  - the method (e.g., GET, POST)
     *  - the number of parameters
     *  - .ini parameter
     *  - combination of the above
     *
     * In the base class, it returns a Poisson service time with mean
     * given by an .ini parameter.
     * The MEC service can implement the calculation as preferred
     *
     */
    virtual double calculateRequestServiceTime();
    virtual double calculateSubscriptionServiceTime();

    /*
     * Abstract methods
     *
     * handleGETRequest
     * handlePOSTRequest
     * handleDELETERequest
     * handlePUTRequest
     *
     * The above methods handle the corresponding HTTP requests
     * @param uri uri of the resource
     * @param socket to send back the response
     *
     * @param body body of the request
     *
     */

    virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) = 0;
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) = 0;
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) = 0;
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) = 0;

    void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override { throw cRuntimeError("Unexpected data"); }
    void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
    void socketEstablished(inet::TcpSocket *socket) override {}
    void socketPeerClosed(inet::TcpSocket *socket) override {}
    void socketClosed(inet::TcpSocket *socket) override;
    void socketFailure(inet::TcpSocket *socket, int code) override {}
    void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    void socketDeleted(inet::TcpSocket *socket) override {}

    void handleStartOperation(inet::LifecycleOperation *operation) override;
    void handleStopOperation(inet::LifecycleOperation *operation) override;
    void handleCrashOperation(inet::LifecycleOperation *operation) override;

    ~MecServiceBase() override;

  public:
    /*
     * This method can be used by a module that wants to inform that
     * something happened, like a value greater than a threshold
     * in order to send a subscription
     *
     * @param event structure to be added to the queue
     */
    virtual void triggeredEvent(EventNotification *event);

    /*
     * This method adds the request to the requests_ queue
     *
     * @param msg request
     */
    virtual void newRequest(HttpRequestMessage *msg);

    // This method adds the subscription event to the subscriptions_ queue

    virtual void newSubscriptionEvent(EventNotification *event);

    /*
     * This method handles a request. It parses the payload and in case
     * calls the correct method (e.g., GET, POST)
     * @param socket used to send back the response
     */
    virtual void handleRequest(inet::TcpSocket *socket);

    /* This method is used by the SocketManager object in order to remove itself from the
     * map
     *
     * @param connection connection object to be deleted
     */
    virtual void removeConnection(SocketManager *connection);

    virtual void closeConnection(SocketManager *connection);

    /* This method can be used by the SocketManager class to emit
     * the length of the request queue upon a request arrival
     */
    virtual void emitRequestQueueLength();

    /* This method removes the subscriptions associated
     * with a closed connection
     */
    virtual void removeSubscriptions(int connId);
};

} //namespace

#endif // __INET_GENERICSERVICE_H


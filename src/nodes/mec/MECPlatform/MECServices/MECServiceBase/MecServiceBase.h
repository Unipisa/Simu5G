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

#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/socket/SocketMap.h"
#include <vector>
#include <map>
//#include "inet/applications/tcpapp/TCPSrvHostApp.h"
#include <queue>
#include "inet/applications/base/ApplicationBase.h"

#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/MecCommon.h"

#include "common/binder/Binder.h"

/**
 *
 * This class implements the general structure of a Mec Service. It holds all the TCP connections
 * with the e.g Mec Applications and manages its lifecycle. It manages Request-Replay and Subscribe-Notify schemes.
 * Every request is inserted in the queue and executed in FIFO order. Also subscription event are queued in a separate queue and 
 * have the priority. The execution times are calculated with the calculateRequestServiceTime method.
 * During initialization it saves all the eNodeB/gNodeBs connected to the Mec Host on which the service is running.
 *
 * Each connection is managed by the SocketManager object that implements the TcpSocket::CallbackInterface
 * It must be subclassed and only the methods relative to the requests management (e.g handleGETrequest)
 * have to be implemented.
 *
 * Note:
 * currently subscription mode is implemented using the same socket the client used to create the subscription (i.e. the POST request).
 * A more elastic solution, i.e. usage of the endpoint specified in the POST request,  is planned to be implemented soon.
 *
 */


#define REQUEST_RNG 0
#define SUBSCRIPTION_RNG 1

typedef std::map<std::string, std::string> reqMap;

class SocketManager;
class SubscriptionBase;
class HttpRequestMessage;
class ServiceRegistry;
class MecPlatformManager;
class EventNotification;

class MecServiceBase: public inet::ApplicationBase, public inet::TcpSocket::ICallback
{
    public:
        MecServiceBase();

    protected:
        std::string serviceName_;
        inet::TcpSocket serverSocket; // Used to listen incoming connections
        inet::SocketMap socketMap; // Stores the connections
        typedef std::set<SocketManager *> ThreadSet;
        ThreadSet threadSet;
        std::string host_;
        Binder* binder_;
        omnetpp::cModule* meHost_;

        MecPlatformManager* mecPlatformManager_;
        ServiceRegistry* servRegistry_;

        std::string baseUriQueries_;
        std::string baseUriSubscriptions_;
        std::string baseSubscriptionLocation_;


        /* 
        * Load generator variables
        * the current implementation assumes a M/M/1 system
        */
        bool loadGenerator_;
        double lambda_; // arrival rate of a BG request form a BG app
        double beta_; // arrival rate of a BG request form a BG app

        int numBGApps_; // number of BG apps
        double rho_;
        omnetpp::simtime_t lastFGRequestArrived_;

        unsigned int subscriptionId_; // identifier for new subscriptions

        // currently not uses
        std::set<std::string>supportedQueryParams_;
        std::set<std::string>supportedSubscriptionParams_;


        typedef std::map<unsigned int, SubscriptionBase*> Subscriptions;
        Subscriptions subscriptions_; //list of all active subscriptions

        std::set<omnetpp::cModule*> eNodeB_;     //eNodeBs connected to the ME Host

        int requestQueueSize_;


        HttpRequestMessage *currentRequestMessageServed_;


        omnetpp::cMessage *requestService_;
        double requestServiceTime_;
        omnetpp::cQueue requests_;               // queue that holds incoming requests

        omnetpp::cMessage *subscriptionService_;
        double subscriptionServiceTime_;
        int subscriptionQueueSize_;
        std::queue<EventNotification*> subscriptionEvents_;          // queue that holds events relative to subscriptions
        EventNotification *currentSubscriptionServed_;

        // signals for statistics
        omnetpp::simsignal_t requestQueueSizeSignal_;
        omnetpp::simsignal_t responseTimeSignal_;


        /*
         * This method is called for every request in the requests_ queue.
         * It check if the receiver is still connected and in case manages the request
         */
        virtual bool manageRequest();

        /*
         * This method is called for every element in the subscriptions_ queue.
         */
        virtual bool manageSubscription();

        /*
         * This method checks the queues length and in case it simulates a request/subscription
         * execution time.
         * Subscriptions have precedence wrt requests
         *
         * if parameter is true -> scheduleAt(NOW)
         * This is
         *
         *
         * @param now when to send the next event
         */
        virtual void scheduleNextEvent(bool now = false);

        virtual void initialize(int stage) override;
        virtual int  numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
        virtual void finish() override;
        virtual void refreshDisplay() const override;

        /*
         * This method finds all the BSs connected to the Mec Host hosting the service
         */
        virtual void getConnectedBaseStations();

        /*
         * This method call the handle method according to the HTTP verb
         * e.g GET --> handleGetRequest()
         * These methods have to be implemented by the MEC services
         */
        virtual void handleCurrentRequest(inet::TcpSocket *socket);

        /*
         * This method manages the situation of an incoming request when
         * the request queue is full. It responds with a HTTP 503
         */
        virtual void handleRequestQueueFull(HttpRequestMessage* msg);


        /*
        * This method calculate the service time of the request based on:
        *  - the method (e.g. GET POST)
        *  - the number of parameters
        *  - .ini parameter
        *  - combination of the above
        *
        * In the base class it returns a Poisson service time with mean
        * given by an .ini parameter.
        * The Mec service can implement the calculation as preferred
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

        virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) = 0;
        virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)   = 0;
        virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    = 0;
        virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) = 0;


        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override { throw omnetpp::cRuntimeError("Unexpected data"); }
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
        virtual void socketEstablished(inet::TcpSocket *socket) override {}
        virtual void socketPeerClosed(inet::TcpSocket *socket) override {}
        virtual void socketClosed(inet::TcpSocket *socket) override;
        virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
        virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
        virtual void socketDeleted(inet::TcpSocket *socket) override {}

        virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
        virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
        virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;





//        virtual void removeSubscription(inet::TcpSocket* socket) = 0;
        virtual ~MecServiceBase();

    public:
        /*
         * This method can be used by a module that want to inform that
         * something happened, like a value greater than a  threshold
         * in order to send a subscription
         *
         * @param event structure to be added in the queue
         */
        virtual void triggeredEvent(EventNotification *event);

        /*
         * This method adds the request in the requests_ queue
         *
         * @param msg request
         */
        virtual void newRequest(HttpRequestMessage *msg);



        // This method adds the subscription event in the subscriptions_ queue

        virtual void newSubscriptionEvent(EventNotification *event);

        /*
         * This method handles a request. It parses the payload and in case
         * calls the correct method (e.g GET, POST)
         * @param socket used to send back the response
         */
        virtual void handleRequest( inet::TcpSocket *socket);

        /* This method is used by the SocketManager object in order to remove itself from the
         * map
         *
         * @param connection connection object to be deleted
         */
        virtual void removeConnection(SocketManager *connection);

        virtual void closeConnection(SocketManager *connection);


//        virtual Http::DataType getDataType(std::string& packet_);

        /* This method can be used by the socketManager class to emit
         * the length of the request queue upon a request arrival
         */
        virtual void emitRequestQueueLength();

        /* This method removes the subscriptions associated
         * with a closed connection
         */
        virtual void removeSubscritions(int connId);
};


#endif // __INET_GENERICSERVICE_H


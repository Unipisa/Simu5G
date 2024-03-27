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

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include "common/utils/utils.h"

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"

#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/SocketManager.h"

#include "nodes/mec/MECPlatform/EventNotification/EventNotification.h"


using namespace omnetpp;

MecServiceBase::MecServiceBase()
{
    binder_ = nullptr;
    meHost_ = nullptr;
    servRegistry_ = nullptr;
    mecPlatformManager_ = nullptr;
    currentRequestMessageServed_ = nullptr;
    currentSubscriptionServed_ = nullptr;
    lastFGRequestArrived_ = 0;
    loadGenerator_ = false;
    rho_ = 0;
}

void MecServiceBase::initialize(int stage)
{

    EV << "MecServiceBase::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "MecServiceBase::initialize" << endl;

        serviceName_ = par("serviceName").stringValue();
        requestServiceTime_ = par("requestServiceTime");
        requestService_ = new cMessage("serveRequest");
        requestQueueSize_ = par("requestQueueSize");

        subscriptionServiceTime_ = par("subscriptionServiceTime");
        subscriptionService_ = new cMessage("serveSubscription");
        subscriptionQueueSize_ = par("subscriptionQueueSize");

        EV << "MecServiceBase::initialize - mean request service time " << requestServiceTime_ << endl;
        EV << "MecServiceBase::initialize - mean subscription service time " << subscriptionServiceTime_<< endl;

        EV << "MecServiceBase::initialize" << endl;


        subscriptionId_ = 0;
        subscriptions_.clear();

        requestQueueSizeSignal_ = registerSignal("requestQueueSize");
        responseTimeSignal_ = registerSignal("responseTime");

        binder_ = getBinder();
        meHost_ = getParentModule() // MECPlatform
                ->getParentModule(); // MeHost

        loadGenerator_ = par("loadGenerator").boolValue();
        if(loadGenerator_)
        {
            beta_ = par("betaa").doubleValue();
            lambda_ = 1/beta_;
            numBGApps_ = par("numBGApps").intValue();
            /*
             * check if the system is stable.
             * For M/M/1 --> rho (lambda_/mu) < 1
             * If arrivals come from multiple independent exponential sources:
             * (numBGApps*lambda_)/mu) < 1
             */
            double lambdaT = lambda_*(numBGApps_+1);
            rho_ = lambdaT*requestServiceTime_;
            if(rho_ >= 1)
                throw cRuntimeError ("M/M/1 system is unstable: rho is %f", rho_);
            EV <<"MecServiceBase::initialize - rho: "<< rho_ <<  endl;
        }

        int found = getParentModule()->findSubmodule("serviceRegistry");
        if(found == -1)
            throw cRuntimeError("MecServiceBase::initialize - ServiceRegistry not present!");
        servRegistry_ = check_and_cast<ServiceRegistry*>(getParentModule()->getSubmodule("serviceRegistry"));

        cModule* module = meHost_->getSubmodule("mecPlatformManager");
        if(module != nullptr)
        {
            EV << "MecServiceBase::initialize - MecPlatformManager found" << endl;
            mecPlatformManager_ = check_and_cast<MecPlatformManager*>(module);
        }

        // get the BSs connected to the mec host
        getConnectedBaseStations();
    }
    inet::ApplicationBase::initialize(stage);
}

void MecServiceBase::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "MecServiceBase::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "MecServiceBase::handleStartOperation - local Address: " << localAddress << " port: " << localPort << endl;
    inet::L3Address localAdd(inet::L3AddressResolver().resolve(localAddress));
    EV << "MecServiceBase::handleStartOperation - local Address resolved: "<< localAdd << endl;

    EV << "MecServiceBase::handleStartOperation - registering MEC service..." << endl;

    ServiceDescriptor servDescriptor;
    servDescriptor.mecHostname = meHost_->getName();
    servDescriptor.name = par("serviceName").stringValue();
    servDescriptor.version = par("serviceVersion").stringValue();
    servDescriptor.serialize = par("serviceSerialize").stringValue();
    servDescriptor.transportId = par("transportId").stringValue();
    servDescriptor.transportName = par("transportName").stringValue();
    servDescriptor.transportType = par("transportType").stringValue();
    servDescriptor.transportProtocol = par("transportProtocol").stringValue();

    servDescriptor.catId = par("catId").stringValue();
    servDescriptor.catName = par("catName").stringValue();
    servDescriptor.catHref = par("catUri").stringValue();
    servDescriptor.catVersion = par("catVersion").stringValue();


    servDescriptor.scopeOfLocality = par("scopeOfLocality").stringValue();
    servDescriptor.isConsumedLocallyOnly = par("consumedLocalOnly").boolValue();


    servDescriptor.addr = localAdd;
    servDescriptor.port = localPort;

    if(mecPlatformManager_ == nullptr)
    {
        EV << "MecServiceBase::handleStartOperation - MEC Orchestrator not present. Register directly to the host Service Registry"<< endl;
        servRegistry_->registerMecService(servDescriptor);
    }
    else
    {
        EV << "MecServiceBase::handleStartOperation - registering MEC service via MEC Orchestrator"<< endl;
        mecPlatformManager_->registerMecService(servDescriptor);
    }

    // e.g. 1.2.3.4:5050
    std::stringstream hostStream;
    hostStream << localAddress<< ":" << localPort;
    host_ = hostStream.str();

    serverSocket.setOutputGate(gate("socketOut"));
    serverSocket.setCallback(this);
    //serverSocket.bind(localAddress[0] ? L3Address(localAddress) : L3Address(), localPort);
    serverSocket.bind(inet::L3Address(), localPort); // bind socket for any address

    serverSocket.listen();
}

void MecServiceBase::handleStopOperation(inet::LifecycleOperation *operation)
{
    for (auto thread : threadSet)
           thread->getSocket()->close();
    serverSocket.close();
//    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void MecServiceBase::handleCrashOperation(inet::LifecycleOperation *operation)
{
    while (!threadSet.empty()) {
        auto thread = *threadSet.begin();
        // TODO destroy!!!
        thread->getSocket()->close();
        removeConnection(thread);
    }
    // TODO always?
    if (operation->getRootModule() != getContainingNode(this))
        serverSocket.destroy();
}

void MecServiceBase::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        EV << " MecServiceBase::handleMessageWhenUp - "<< msg->getName() << endl;
        if(strcmp(msg->getName(), "serveSubscription") == 0)
        {
//            maybe the service wants to perform some operations in case the subId is not present,
//            so the service base just calls the manageSubscription() method.
//            bool res = false;
//            // fixme check if the subscription is still active, i.e. the client did not disconnected
//            if(subscriptions_.find(currentSubscriptionServed_->getSubId()) != subscriptions_.end() )
//                res = manageSubscription();
//            else
//            {
//                // subscription id not present, client disconnected, do not manage.
//                if(currentSubscriptionServed_!= nullptr)
//                        delete currentSubscriptionServed_;
//                currentSubscriptionServed_ = nullptr;
//                return false;
//            }
//      }
            bool res = manageSubscription();
            scheduleNextEvent(!res);
        }
        else if(strcmp(msg->getName(), "serveRequest") == 0)
        {
            bool res = manageRequest();
            scheduleNextEvent(!res);
        }
        else
        {
            delete msg;
        }
    }
    else {
        inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.findSocketFor(msg));
        if (socket)
            socket->processMessage(msg);
        else if (serverSocket.belongsToSocket(msg))
            serverSocket.processMessage(msg);
        else {
    //            throw cRuntimeError("Unknown incoming message: '%s'", msg->getName());
            EV_ERROR << "message " << msg->getFullName() << "(" << msg->getClassName() << ") arrived for unknown socket \n";
            delete msg;
        }
    }

}

void MecServiceBase::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
{
    // new TCP connection -- create new socket object and server process
    inet::TcpSocket *newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut"));

    const char *serverThreadClass = par("serverThreadClass");
    cModuleType *moduleType = cModuleType::get(serverThreadClass);
    char name[80];
    sprintf(name, "thread_%i", newSocket->getSocketId());
    SocketManager *proc = check_and_cast<SocketManager *>(moduleType->create(name, this));
    proc->finalizeParameters();
    proc->callInitialize();

    newSocket->setCallback(proc);
    proc->init(this, newSocket);

    socketMap.addSocket(newSocket);
    threadSet.insert(proc);
    EV << "New socket added - ["<< newSocket->getRemoteAddress() <<":"<< newSocket->getRemotePort() << " connId: "<<newSocket->getSocketId()<<"]" << endl;

    socket->accept(availableInfo->getNewSocketId());
}

void MecServiceBase::socketClosed(inet::TcpSocket *socket)
{
//    if (operationalState == State::STOPPING_OPERATION && threadSet.empty() && !serverSocket.isOpen())
//        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

bool MecServiceBase::manageRequest()
{
    EV_INFO <<" MecServiceBase::manageRequest" << endl;
  //  EV << "MecServiceBase::manageRequest - start manageRequest" << endl;
    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(currentRequestMessageServed_->getSockId()));
    if(socket)
    {
        /*
         * Manage backgroundRequest
         */

        if(currentRequestMessageServed_->isBackgroundRequest())
        {
            if(currentRequestMessageServed_->isLastBackgroundRequest())
            {
                Http::send200Response(socket, "{Done}"); //notify the client last bg request served
            }
        }
        else
        {
            handleRequest(socket);
            simtime_t responseTime = simTime() - currentRequestMessageServed_->getArrivalTime();
            EV_INFO <<" MecServiceBase::manageRequest - Response time - " << responseTime << endl;
            emit(responseTimeSignal_, responseTime);
        }

        if(currentRequestMessageServed_ != nullptr)
        {
            delete currentRequestMessageServed_;
            currentRequestMessageServed_ = nullptr;
        }
        return true;
    }
    else // socket has been closed or some error occurred, discard request
    {
        // I should schedule immediately a new request execution
        if(currentRequestMessageServed_ != nullptr)
        {
            delete currentRequestMessageServed_;
            currentRequestMessageServed_ = nullptr;
        }
        return false;
    }
}

void MecServiceBase::scheduleNextEvent(bool now)
{
    EV <<"MecServiceBase::scheduleNextEvent"<< endl;
    // schedule next event
    if(subscriptionEvents_.size() != 0 && !subscriptionService_->isScheduled() && !requestService_->isScheduled())
    {
        EV <<"MecServiceBase::scheduleNextEvent - subscription branch"<< endl;
        currentSubscriptionServed_ = subscriptionEvents_.front();
        subscriptionEvents_.pop();
        if(now)
            scheduleAt(simTime() + 0 , subscriptionService_);
        else
        {
            double serviceTime = calculateSubscriptionServiceTime(); //must be >0
            EV <<"MecServiceBase::scheduleNextEvent- subscription service time: "<< serviceTime << endl;
            scheduleAt(simTime() + serviceTime , subscriptionService_);
        }
    }
    else if (requests_.getLength() != 0 && !requestService_->isScheduled() && !subscriptionService_->isScheduled())
    {
        EV <<"MecServiceBase::scheduleNextEvent - request branch"<< endl;
        currentRequestMessageServed_ = check_and_cast<HttpRequestMessage*>(requests_.pop());

        if(loadGenerator_ && !currentRequestMessageServed_->isBackgroundRequest())
        {
            EV <<"MecServiceBase::scheduleNextEvent - load generator is on, use the response time in the packet"<< endl;
            /*
             * If the loadGenerator flag is active, use the responseTime calculated at arriving time
             */
            scheduleAt(simTime() + currentRequestMessageServed_->getResponseTime() , requestService_);
            return;
        }

        if(now)
            scheduleAt(simTime() + 0 , subscriptionService_);
        else
        {
            //calculate the serviceTime base on the type | parameters
            double serviceTime = calculateRequestServiceTime(); //must be >0
            EV <<"MecServiceBase::scheduleNextEvent- request service time: "<< serviceTime << endl;
            scheduleAt(simTime() + serviceTime , requestService_);
        }
    }
}

void MecServiceBase::handleRequestQueueFull(HttpRequestMessage *msg)
{
    EV << " MecServiceBase::handleQueueFull" << endl;
    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(msg->getSockId()));
    if(!socket)
    {
        throw cRuntimeError ("MecServiceBase::handleRequestQueueFull - socket not found, this should not happen.");
    }

    if(msg->isBackgroundRequest() && !msg->isLastBackgroundRequest())
    {
        delete msg;
        return;
    }

    delete msg;
    std::string reason("{Request server queue full}");
    Http::send503Response(socket, reason.c_str());
}

void MecServiceBase::newRequest(HttpRequestMessage *msg)
{
    EV << "Queue length: " << requests_.getLength() << endl;
    // If queue is full respond 503 queue full
    if(requestQueueSize_ != 0 && requests_.getLength() == requestQueueSize_){
        EV << "MecServiceBase::newRequest - queue is full" << endl;
       handleRequestQueueFull(msg);
       return;
    }

    /*
     * If the loadGenerator flag is active, it means that arriving FG requests
     * needs to wait until other requests (i.e. background requests (BG)) have been served.
     * Our implementation assumes BG requests arrive exponentially, but you can
     * manage the load generation as you prefer.
     *
     * When the FG request queue is empty, upon a FG arrive the number of requests
     * already in the system is calculated supposing a M/M/1 system with service mu.
     */
    if(loadGenerator_)
    {
        int numOfBGReqs;

        if(requests_.getLength() == 0)
        {
            // debug
            numOfBGReqs = geometric((1-rho_), 0);
            EV << "MecServiceBase::newRequest - number of BG requests in front of this FG request: " << numOfBGReqs << endl;

        }
        else
        {
            simtime_t deltaTime = simTime() - lastFGRequestArrived_;
            numOfBGReqs = poisson(deltaTime.dbl()*numBGApps_*lambda_, 0); // BG requests arrived in period of time deltaTime
            // debug
            EV << "MecServiceBase::newRequest - number of BG requests between this and the last FG request: " << numOfBGReqs << endl;
        }

        double sumOfresponseTimes = 0;
        for(int i = 0 ; i < numOfBGReqs+1; ++i)
        {
            sumOfresponseTimes += exponential(requestServiceTime_, REQUEST_RNG);
        }

        EV << "MecServiceBase::newRequest - FG request service time is (sumOfresponseTimes): " << sumOfresponseTimes << endl;

        msg->setResponseTime(sumOfresponseTimes);
        lastFGRequestArrived_ = simTime();
        emit(requestQueueSizeSignal_, numOfBGReqs);
    }

    requests_.insert(msg);
    scheduleNextEvent();
}

void MecServiceBase::newSubscriptionEvent(EventNotification* event)
{
    EV << "Queue length: " << subscriptionEvents_.size() << endl;
    // If queue is full delete event
    if(subscriptionQueueSize_ != 0 && subscriptionEvents_.size() == subscriptionQueueSize_){
        EV << "MecServiceBase::newSubscriptionEvent - subscription queue is full. Deleting event..." << endl;
        delete event;
        return;
    }

    subscriptionEvents_.push(event);
    scheduleNextEvent();
}

bool MecServiceBase::manageSubscription()
{
    // TODO redefine for managing the subscription
    return false;
}

// TODO method not used
void MecServiceBase::triggeredEvent(EventNotification* event)
{
    newSubscriptionEvent(event);
}

double MecServiceBase::calculateRequestServiceTime()
{
    double time;
    /*
     * Manage the case it is a background request
     */
    if(currentRequestMessageServed_->isBackgroundRequest())
    {
        time = exponential(requestServiceTime_, REQUEST_RNG);
    }
    else
    {
        time = exponential(requestServiceTime_, REQUEST_RNG);
    }
//    return (time*1e-6);
    EV << "MecServiceBase::calculateRequestServiceTime - time: " << time << endl;
    return time;
}

double MecServiceBase::calculateSubscriptionServiceTime()
{
    double time;
    time = exponential(subscriptionServiceTime_, SUBSCRIPTION_RNG);
//    return (time*1e-6);
    return time;

}

void MecServiceBase::handleCurrentRequest(inet::TcpSocket *socket){}


void MecServiceBase::handleRequest(inet::TcpSocket *socket){
    EV << "MecServiceBase::handleRequest" << endl;

     if(currentRequestMessageServed_->getState() == CORRECT){ // request-line is well formatted
         if(std::strcmp(currentRequestMessageServed_->getMethod(),"GET") == 0)
         {
             handleGETRequest(currentRequestMessageServed_, socket); // pass URI
         }
         else if(std::strcmp(currentRequestMessageServed_->getMethod(),"POST") == 0) //subscription
         {
             handlePOSTRequest(currentRequestMessageServed_,  socket); // pass URI
         }

         else if(std::strcmp(currentRequestMessageServed_->getMethod(),"PUT") == 0)
         {
            handlePUTRequest(currentRequestMessageServed_,  socket); // pass URI
         }
         else if(std::strcmp(currentRequestMessageServed_->getMethod(),"DELETE") == 0)
        {
         handleDELETERequest(currentRequestMessageServed_,  socket); // pass URI
        }
         else
         {
             Http::send405Response(socket);
         }
     }
     else
     {
         Http::send400Response(socket);
     }
 }

void MecServiceBase::refreshDisplay() const
{
// TODO
    return;
}

void MecServiceBase::getConnectedBaseStations(){

    EV <<"MecServiceBase::getConnectedBaseStations" << endl;

    //getting the list of BS associated to this mec system from NED
    if(meHost_->hasPar("bsList") && strcmp(meHost_->par("bsList").stringValue(), "")){

       char* token = strtok ( (char*)meHost_->par("bsList").stringValue(), ", ");            // split by commas
       while (token != NULL)
       {
           EV <<"MecServiceBase::getConnectedBaseStations " << token << endl;
           cModule *bsModule = getSimulation()->getModuleByPath(token);
           EV << "add2: " << bsModule << endl;
           eNodeB_.insert(bsModule);
           token = strtok (NULL, ", ");
       }
    }
    return;
}


void MecServiceBase::closeConnection(SocketManager * connection)
{
        EV << "MecServiceBase::closeConnection"<<endl;
    // remove socket
       socketMap.removeSocket(connection->getSocket());
       threadSet.erase(connection);
       socketClosed(connection->getSocket());
       // remove thread object
       connection->deleteModule();
}

void MecServiceBase::removeConnection(SocketManager *connection)
{
    EV << "MecServiceBase::removeConnection"<<endl;
    // remove socket
    inet::TcpSocket * sock = check_and_cast< inet::TcpSocket*>( socketMap.removeSocket(connection->getSocket()));
    sock->close();
    delete sock;
    // remove thread object
    threadSet.erase(connection);
    connection->deleteModule();
}

void MecServiceBase::finish()
{
    EV << "MecServiceBase::finish()" << endl;
    serverSocket.close();
    while (!threadSet.empty())
    {
        removeConnection(*threadSet.begin());
    }
}

MecServiceBase::~MecServiceBase(){
    std::cout << "~"<< serviceName_ << std::endl;
        // remove and delete threads

    cancelAndDelete(requestService_);
    cancelAndDelete(subscriptionService_);
    delete currentRequestMessageServed_;
    delete currentSubscriptionServed_;

    while(!requests_.isEmpty())
    {
        delete requests_.pop();;
    }

    while(!subscriptionEvents_.empty())
    {
        EventNotification *notEv = subscriptionEvents_.front();
        subscriptionEvents_.pop();
        delete notEv;
    }
    std::cout << "~"<< serviceName_<<" Subscriptions list length: " << subscriptions_.size() << std::endl;
    Subscriptions::iterator it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        std::cout << serviceName_<<" Deleting subscription with id: " << it->second->getSubscriptionId() << std::endl;
        delete it->second;
        subscriptions_.erase(it++);
    }
    std::cout << "~"<< serviceName_<<" Subscriptions list length: " << subscriptions_.size() << std::endl;
}

void MecServiceBase::emitRequestQueueLength()
{
   // emit(requestQueueSizeSignal_, requests_.getLength());
}


void MecServiceBase::removeSubscritions(int connId)
{
    Subscriptions::iterator it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        if (it->second->getSocketConnId() == connId) {
            EV << "Remove subscription with id = " << it->second->getSubscriptionId();
            delete it->second;
            subscriptions_.erase(it++);
            EV << " list length: " << subscriptions_.size() << std::endl;
        } else {
           ++it;
        }
    }
}


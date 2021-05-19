// TODO intro

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include "common/utils/utils.h"

#include "../MeServiceBase/MeServiceBase.h"
#include "nodes/mec/MEPlatform/ServiceRegistry/ServiceRegistry.h"

#include "nodes/mec/MEPlatform/MeServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"

#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"
#include "nodes/mec/MEPlatform/MeServices/MeServiceBase/SocketManager.h"

#include "nodes/mec/MEPlatform/EventNotification/EventNotification.h"


using namespace omnetpp;

MeServiceBase::MeServiceBase()
{
    binder_ = nullptr;
    meHost_ = nullptr;
    servRegistry_ = nullptr;
}

void MeServiceBase::initialize(int stage)
{

    EV << "MeServiceBase::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        EV << "MeServiceBase::initialize" << endl;

        serviceName_ = par("serviceName").stringValue();
        requestServiceTime_ = par("requestServiceTime");
        requestService_ = new cMessage("serveRequest");
        requestQueueSize_ = par("requestQueueSize");

        subscriptionServiceTime_ = par("subscriptionServiceTime");
        subscriptionService_ = new cMessage("serveSubscription");
        subscriptionQueueSize_ = par("subscriptionQueueSize");
        currentRequestMessageServed_ = nullptr;
        currentSubscriptionServed_ = nullptr;

        subscriptionId_ = 0;
        subscriptions_.clear();

        requestQueueSizeSignal_ = registerSignal("requestQueueSize");
        binder_ = getBinder();
        meHost_ = getParentModule() // MePlatform
                ->getParentModule(); // MeHost


        int found = getParentModule()->findSubmodule("serviceRegistry");
        if(found == -1)
            throw cRuntimeError("MeServiceBase::initialize - ServiceRegistry not present!");
        servRegistry_ = check_and_cast<ServiceRegistry*>(getParentModule()->getSubmodule("serviceRegistry"));

        // get the gnb connected to the mehost
        getConnectedEnodeB();
    }
    inet::ApplicationBase::initialize(stage);
}

void MeServiceBase::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "MeServiceBase::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;
    inet::L3Address localAdd(inet::L3AddressResolver().resolve(localAddress));
    EV << "Local Address resolved: "<< localAdd << endl;

    ServiceDescriptor servDescriptor;
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


    servDescriptor.addr = localAdd;
    servDescriptor.port = localPort;

    servRegistry_->registerMeService(servDescriptor);

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

void MeServiceBase::handleStopOperation(inet::LifecycleOperation *operation)
{
    for (auto thread : threadSet)
           thread->getSocket()->close();
    serverSocket.close();
//    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void MeServiceBase::handleCrashOperation(inet::LifecycleOperation *operation)
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

void MeServiceBase::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        EV << "isSelfMessage" << endl;
        if(strcmp(msg->getName(), "serveSubscription") == 0)
        {
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

void MeServiceBase::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
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

void MeServiceBase::socketClosed(inet::TcpSocket *socket)
{
//    if (operationalState == State::STOPPING_OPERATION && threadSet.empty() && !serverSocket.isOpen())
//        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

bool MeServiceBase::manageRequest()
{
    EV_INFO <<" MeServiceBase::manageRequest" << endl;
  //  EV << "MeServiceBase::manageRequest - start manageRequest" << endl;
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

            //        handleCurrentRequest(socket);
            handleRequest(socket);
        }

        if(currentRequestMessageServed_ != nullptr)
        {
            delete currentRequestMessageServed_;
            currentRequestMessageServed_ = nullptr;
        }
        return true;
    }
    else // socket has been closed or some error occured, discard request
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

void MeServiceBase::scheduleNextEvent(bool now)
{
    // schedule next event
    if(subscriptionEvents_.size() != 0 && !subscriptionService_->isScheduled() && !requestService_->isScheduled())
    {
        currentSubscriptionServed_ = subscriptionEvents_.front();
        subscriptionEvents_.pop();
        if(now)
            scheduleAt(simTime() + 0 , subscriptionService_);
        else
        {
            double time = poisson(subscriptionServiceTime_, REQUEST_RNG);
            EV <<"time: "<< time*1e-6 << endl;
            scheduleAt(simTime() + time*1e-6 , subscriptionService_);
        }
    }
    else if (requests_.getLength() != 0 && !requestService_->isScheduled() && !subscriptionService_->isScheduled())
    {
        currentRequestMessageServed_ = check_and_cast<HttpRequestMessage*>(requests_.pop());
        //calculate the serviceTime base on the type | parameters
        double serviceTime = calculateRequestServiceTime(); //must be >0
        scheduleAt(simTime() + serviceTime , requestService_);
       // EV << "scheduleNextEvent - Request execution started" << endl;
        EV <<"request service time: "<< serviceTime << endl;

    }
}

void MeServiceBase::handleRequestQueueFull(HttpRequestMessage *msg)
{
    EV << " MeServiceBase::handleQueueFull" << endl;
    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(msg->getSockId()));
    if(!socket)
    {
        throw cRuntimeError ("MeServiceBase::handleRequestQueueFull - socket not found, this should not happen.");
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

void MeServiceBase::newRequest(HttpRequestMessage *msg)
{
    EV << "Queue length: " << requests_.getLength() << endl;
    // If queue is full respond 503 queue full
    if(requestQueueSize_ != 0 && requests_.getLength() == requestQueueSize_){
       handleRequestQueueFull(msg);
       return;
    }

    requests_.insert(msg);
    scheduleNextEvent();
}



void MeServiceBase::newSubscriptionEvent(EventNotification* event)
{
    EV << "Queue length: " << subscriptionEvents_.size() << endl;
    // If queue is full delete event
    if(subscriptionQueueSize_ != 0 && subscriptionEvents_.size() == subscriptionQueueSize_){
        delete event;
        return;
    }

    subscriptionEvents_.push(event);
    scheduleNextEvent();
}

bool MeServiceBase::manageSubscription()
{
    // TODO redefine for managing the subscription
    return false;
}

// TODO method not used
void MeServiceBase::triggeredEvent(EventNotification* event)
{
    newSubscriptionEvent(event);
}

double MeServiceBase::calculateRequestServiceTime()
{
    double time;
    /*
     * Manage the case it is a background request
     */
    if(currentRequestMessageServed_->isBackgroundRequest())
    {
        time = poisson(requestServiceTime_, REQUEST_RNG);
    }
    else
    {
        time = poisson(requestServiceTime_, REQUEST_RNG);
    }
    return (time*1e-6);
}

void MeServiceBase::handleCurrentRequest(inet::TcpSocket *socket){}

// old version, before request parsing. Not used anymore
void MeServiceBase::handleRequest(inet::TcpSocket *socket){
    EV << "MeServiceBase::handleRequest" << endl;

     if(currentRequestMessageServed_->getState() == CORRECT){ // request-line is well formatted
         if(std::strcmp(currentRequestMessageServed_->getMethod(),"GET") == 0)
         {
             std::string uri(currentRequestMessageServed_->getUri());
             handleGETRequest(uri, socket); // pass URI
         }
         else if(std::strcmp(currentRequestMessageServed_->getMethod(),"POST") == 0) //subscription
         {
             std::string uri(currentRequestMessageServed_->getUri());
             std::string body(currentRequestMessageServed_->getBody());
             handlePOSTRequest(uri, body,  socket); // pass URI
         }

         else if(std::strcmp(currentRequestMessageServed_->getMethod(),"PUT") == 0)
         {
            std::string uri(currentRequestMessageServed_->getUri());
            std::string body(currentRequestMessageServed_->getBody());
            handlePUTRequest(uri, body,  socket); // pass URI
         }
         else if(std::strcmp(currentRequestMessageServed_->getMethod(),"DELETE") == 0)
        {
         std::string uri(currentRequestMessageServed_->getUri());
         handleDELETERequest(uri,  socket); // pass URI
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

void MeServiceBase::refreshDisplay() const
{
// TODO
    return;
}

void MeServiceBase::getConnectedEnodeB(){
    int eNodeBsize = meHost_->gateSize("pppENB");
    for(int i = 0; i < eNodeBsize ; ++i){
        cModule *eNodebName = meHost_->gate("pppENB$o", i) // pppENB[i] output
                                     ->getNextGate()       // eNodeB module connected gate
                                     ->getOwnerModule();   // eBodeB module
        eNodeB_.push_back(eNodebName);
    }
    return;
}


void MeServiceBase::closeConnection(SocketManager * connection)
{
        EV << "MeServiceBase::closeConnection"<<endl;
    // remove socket
       socketMap.removeSocket(connection->getSocket());
       threadSet.erase(connection);

       socketClosed(connection->getSocket());

       // remove thread object
       connection->deleteModule();
}

void MeServiceBase::removeConnection(SocketManager *connection)
{


    EV << "MeServiceBase::removeConnection"<<endl;
    // remove socket
    delete socketMap.removeSocket(connection->getSocket());
    // remove thread object
    threadSet.erase(connection);
    connection->deleteModule();
}

void MeServiceBase::finish()
{
    EV << "MeServiceBase::finish()" << endl;
    serverSocket.close();
    while (!threadSet.empty())
        {
            removeConnection(*threadSet.begin());
        }
}

MeServiceBase::~MeServiceBase(){

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
    std::cout << serviceName_<<" Subscriptions list length: " << subscriptions_.size() << std::endl;
    Subscriptions::iterator it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        std::cout << serviceName_<<" Deleting subscription with id: " << it->second->getSubscriptionId() << std::endl;
        // stop periodic notification timer
//        cMessage *msg =it->second->getNotificationTrigger();
//        if(msg!= nullptr && msg->isScheduled())
//            cancelAndDelete(it->second->getNotificationTrigger());
        delete it->second;
        subscriptions_.erase(it++);
    }
    std::cout << serviceName_<<" Subscriptions list length: " << subscriptions_.size() << std::endl;
}

void MeServiceBase::emitRequestQueueLength()
{
    emit(requestQueueSizeSignal_, requests_.getLength());
}


void MeServiceBase::removeSubscritions(int connId)
{
    Subscriptions::iterator it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        if (it->second->getSocketConnId() == connId) {
            std::cout << "Remnove subscription with id = " << it->second->getSubscriptionId();
            delete it->second;
            subscriptions_.erase(it++);
            std::cout << " list length: " << subscriptions_.size() << std::endl;
        } else {
           ++it;
        }
    }
}


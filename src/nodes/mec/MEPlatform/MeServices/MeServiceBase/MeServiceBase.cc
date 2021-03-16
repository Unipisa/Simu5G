// TODO intro

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
//#include "inet/transportlayer/contract/tcp/TCPCommand_m.h"
//#include "inet/common/RawPacket.h"

//#include "common/utils/utils.h
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include "common/utils/utils.h"

#include "../MeServiceBase/MeServiceBase.h"

#include "nodes/mec/MEPlatform/MeServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MEPlatform/MeServices/packets/HttpRequestMessage/HttpRequestMessage.h"

#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"
#include "nodes/mec/MEPlatform/MeServices/MeServiceBase/SocketManager.h"

using namespace omnetpp;

MeServiceBase::MeServiceBase(){}

void MeServiceBase::initialize(int stage)
{
    inet::ApplicationBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        return;
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {

        requestServiceTime_ = par("requestServiceTime");
        requestService_ = new cMessage("serveRequest");
        requestQueueSize_ = par("requestQueueSize");

        subscriptionServiceTime_ = par("subscriptionServiceTime");
        subscriptionService_ = new cMessage("serveSubscription");
        subscriptionQueueSize_ = par("subscriptionQueueSize");
        currentRequestServed_ = nullptr;
        currentSubscriptionServed_ = nullptr;

        subscriptionId_ = 0;
        subscriptions_.clear();

        requestQueueSizeSignal_ = registerSignal("requestQueueSize");
        binder_ = getBinder();
        meHost_ = getParentModule() // virtualizationInfrastructure
                ->getParentModule(); // MeHost

        // get the gnb connected to the mehost
        getConnectedEnodeB();
    }
}


void MeServiceBase::handleStartOperation(inet::LifecycleOperation *operation)
{
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;

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
    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(currentRequestMessage_->getSockId()));
    if(socket)
    {
        EV_INFO <<" MeServiceBase::manageRequest" << endl;
//        handleCurrentRequest(socket);
        handleRequest(socket);
        if(currentRequestMessage_ != nullptr)
            delete currentRequestMessage_;
//        if(currentRequestServed_!= nullptr)
//            delete currentRequestServed_;
//        currentRequestServed_ = nullptr;
//        currentRequestServedmap_.clear();
//        currentRequestState_= UNDEFINED;
        return true;
    }
    else // socket has been closed or some error occured, discard request
    {
        // I should schedule immediately a new request execution
        if(currentRequestServed_!= nullptr)
            delete currentRequestServed_;
        return false;
    }
}

void MeServiceBase::scheduleNextEvent(bool now)
{
    // schedule next event
    if(subscriptionEvents_.getLength() != 0 && !subscriptionService_->isScheduled())
    {
        currentSubscriptionServed_ = check_and_cast<cMessage *>(subscriptionEvents_.pop());
        if(now)
            scheduleAt(simTime() + 0 , subscriptionService_);
        else
        {
            double time = poisson(subscriptionServiceTime_, REQUEST_RNG);
            EV <<"time: "<< time*1e-6 << endl;
            scheduleAt(simTime() + time*1e-6 , subscriptionService_);
        }
    }
    else if (requests_.getLength() != 0 && !requestService_->isScheduled() )
    {
//        currentRequestServed_ = check_and_cast<cMessage *>(requests_.pop());
        currentRequestMessage_ = check_and_cast<HttpRequestMessage*>(requests_.pop());
        //calculate the serviceTime base on the type | parameters
        double serviceTime = calculateRequestServiceTime(); //must be >0
        scheduleAt(simTime() + serviceTime , requestService_);
       // EV << "scheduleNextEvent - Request execution started" << endl;
        EV <<"request service time: "<< serviceTime << endl;

    }
}

void MeServiceBase::handleRequestQueueFull(cMessage *msg)
{
    EV << " MeServiceBase::handleQueueFull" << endl;
    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.findSocketFor(msg));
    if(!socket)
    {
        throw cRuntimeError ("MeServiceBase::handleRequestQueueFull - socket not found, this should not happen.");
    }
    delete msg;
    std::string reason("{Request server queue full}");
    Http::send503Response(socket, reason.c_str());
}

void MeServiceBase::handleRequestQueueFull(HttpBaseMessage *msg)
{
    EV << " MeServiceBase::handleQueueFull" << endl;
    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(msg->getSockId()));
    if(!socket)
    {
        throw cRuntimeError ("MeServiceBase::handleRequestQueueFull - socket not found, this should not happen.");
    }
    delete msg;
    std::string reason("{Request server queue full}");
    Http::send503Response(socket, reason.c_str());
}



void MeServiceBase::newRequest(cMessage *msg)
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

void MeServiceBase::newRequest(HttpBaseMessage *msg)
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



void MeServiceBase::newSubscriptionEvent(cMessage *msg)
{
    EV << "Queue length: " << subscriptionEvents_.getLength() << endl;
    // If queue is full delete event
    if(requestQueueSize_ != 0 && subscriptionEvents_.getLength() == subscriptionQueueSize_){
        delete msg;
        return;
    }

    subscriptionEvents_.insert(msg);
    scheduleNextEvent();
}

bool MeServiceBase::manageSubscription()
{
    return handleSubscriptionType(currentSubscriptionServed_);
}

// TODO method not used
void MeServiceBase::triggeredEvent(short int event)
{
    cMessage *msg = new cMessage("subscriptionEvent", event);
    newSubscriptionEvent(msg);
}

double MeServiceBase::calculateRequestServiceTime()
{
    double time = poisson(requestServiceTime_, REQUEST_RNG);
                return (time*1e-6);

    EV << "MeServiceBase::calculateRequestServiceTime()" << endl;
    parseCurrentRequest();
    if(currentRequestState_ == CORRECT)
    {
        if(currentRequestServedmap_.at("method").compare("GET") == 0)
        {
            double time = poisson(requestServiceTime_, REQUEST_RNG);
            return (time*1e-6);
        }
    }
    else
    {
        double time = poisson(requestServiceTime_, REQUEST_RNG);
        return (time*1e-6);
    }
    return 0;
}

void MeServiceBase::handleCurrentRequest(inet::TcpSocket *socket){
    if(currentRequestState_ == CORRECT){ // request-line is well formatted
         if(currentRequestServedmap_.at("method").compare("GET") == 0)
             handleGETRequest(currentRequestServedmap_.at("uri"), socket); // pass URI
         else if(currentRequestServedmap_.at("method").compare("POST") == 0) //subscription
             handlePOSTRequest(currentRequestServedmap_.at("uri"), currentRequestServedmap_.at("body"),  socket); // pass URI
         else if(currentRequestServedmap_.at("method").compare("PUT") == 0)
             handlePUTRequest(currentRequestServedmap_.at("uri"), currentRequestServedmap_.at("body"),  socket); // pass URI
         else if(currentRequestServedmap_.at("method").compare("DELETE") == 0)
             handleDELETERequest(currentRequestServedmap_.at("uri"),  socket); // pass URI
         else if(currentRequestServedmap_.at("method").compare("HEAD") == 0)
             Http::send405Response(socket);

         else if(currentRequestServedmap_.at("method").compare("CONNECT") == 0)
             Http::send405Response(socket);

         else if(currentRequestServedmap_.at("method").compare("TRACE") == 0)
             Http::send405Response(socket);

         else if(currentRequestServedmap_.at("method").compare("PATCH") == 0)
             Http::send405Response(socket);

         else if(currentRequestServedmap_.at("method").compare("OPTIONS") == 0)
             Http::send405Response(socket);
         else
         {
             throw cRuntimeError("MeServiceBase::HTTP verb %s non recognised", currentRequestServedmap_.at("method").c_str());
         }
     }
    else
    {
        EV << "MeServiceBase::handleCurrentRequest - Incorrect Request" << endl;
        Http::send405Response(socket);
    }
 }

// It only works with complete HTTP messages in a single TCP segment
void MeServiceBase::parseCurrentRequest(){

    EV_INFO << "MeServiceBase::parseCurrentRequest() - Start parseRequest" << endl;
//    std::string packet = lte::utils::getPacketPayload(currentRequestServed_);
    inet::Packet* pkt = check_and_cast<inet::Packet*>(currentRequestServed_);
    std::vector<uint8_t> bytes =  pkt->peekDataAsBytes()->getBytes();
            std::string packet(bytes.begin(), bytes.end());
    std::vector<std::string> splitting = lte::utils::splitString(packet, "\r\n\r\n"); // bound between header and body
    std::string header;
    std::string body;

    if(splitting.size() == 2)
    {
        header = splitting[0];
        body   = splitting[1];
        currentRequestServedmap_.insert( std::pair<std::string, std::string>("body", body) );

    }
    else if(splitting.size() == 1) // no body
    {
        header = splitting[0];
    }
    else //incorrect request
    {
      currentRequestState_ = BAD_REQUEST;
      return;
    }

    std::vector<std::string> line;
    std::vector<std::string> lines = lte::utils::splitString(header, "\r\n");
    std::vector<std::string>::iterator it = lines.begin();
    line = lte::utils::splitString(*it, " ");  // Request-Line GET / HTTP/1.1
    if(line.size() != 3 ){
        currentRequestState_ =  BAD_REQ_LINE;
    return;
    }
    if(!Http::checkHttpVersion(line[2])){
        currentRequestState_ =  BAD_HTTP;//send 505Response
        return;
    }

    currentRequestServedmap_.insert( std::pair<std::string, std::string>("method", line[0]) );
    currentRequestServedmap_.insert( std::pair<std::string, std::string>("uri", line[1]) );
    currentRequestServedmap_.insert( std::pair<std::string, std::string>("http", line[2]) );

    for(++it; it != lines.end(); ++it) {
        line = lte::utils::splitString(*it, ": ");
        if(line.size() == 2)
            currentRequestServedmap_.insert( std::pair<std::string, std::string>(line[0], line[1]) );
        else
        {
            currentRequestState_ =  BAD_HEADER;
            return;
        }
    }
//    if(currentRequestServedmap_.at("Host").compare(host_) != 0)
//    {
//        currentRequestState_ =  DIFF_HOST;
//        return;
//    }

    EV << "MeServiceBase::parseCurrentRequest - URI" << currentRequestServedmap_.at("uri") << endl;
        currentRequestState_ =  CORRECT;
        return;
}
//

// old version, before request parsing. Not used anymore
void MeServiceBase::handleRequest(inet::TcpSocket *socket){
    EV << "MeServiceBase::handleRequest" << endl;

     if(currentRequestMessage_->getState() == eCORRECT){ // request-line is well formatted
         if(std::strcmp(currentRequestMessage_->getMethod(),"GET") == 0)
         {
             std::string uri(currentRequestMessage_->getUri());
             handleGETRequest(uri, socket); // pass URI
         }
         else if(std::strcmp(currentRequestMessage_->getMethod(),"POST") == 0) //subscription
         {
             std::string uri(currentRequestMessage_->getUri());
             std::string body(currentRequestMessage_->getBody());
             handlePOSTRequest(uri, body,  socket); // pass URI
         }

         else if(std::strcmp(currentRequestMessage_->getMethod(),"PUT") == 0)
         {
            std::string uri(currentRequestMessage_->getUri());
            std::string body(currentRequestMessage_->getBody());
            handlePUTRequest(uri, body,  socket); // pass URI
         }
         else if(std::strcmp(currentRequestMessage_->getMethod(),"DELETE") == 0)
        {
         std::string uri(currentRequestMessage_->getUri());
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

// old version, before current request parsing. Not used anymore
bool MeServiceBase::parseRequest(std::string& packet_, inet::TcpSocket *socket, reqMap* request){
    EV_INFO << "MeServiceBase::parseRequest - Start parseRequest" << endl;
    EV_INFO << "MeServiceBase::parseRequest - payload: " << packet_ << endl;

//    std::string packet(packet_);
    std::vector<std::string> splitting = lte::utils::splitString(packet_, "\r\n\r\n"); // bound between header and body
    std::string header;
    std::string body;
    
    if(splitting.size() == 2)
    {
        header = splitting[0];
        body   = splitting[1];
        request->insert( std::pair<std::string, std::string>("body", body) );
    }
    else if(splitting.size() == 1) // no body
    {
        header = splitting[0];
    }
    else //incorrect request
    {
       Http::send400Response(socket); // bad request
       return false;
    }
    
    std::vector<std::string> line;
    std::vector<std::string> lines = lte::utils::splitString(header, "\r\n");
    std::vector<std::string>::iterator it = lines.begin();
    line = lte::utils::splitString(*it, " ");  // Request-Line GET / HTTP/1.1
    if(line.size() != 3 ){
        Http::send400Response(socket);
        return false;
    }
    if(!Http::checkHttpVersion(line[2])){
        Http::send505Response(socket);
        return false;
    }

    request->insert( std::pair<std::string, std::string>("method", line[0]) );
    request->insert( std::pair<std::string, std::string>("uri", line[1]) );
    request->insert( std::pair<std::string, std::string>("http", line[2]) );

    for(++it; it != lines.end(); ++it) {
        line = lte::utils::splitString(*it, ": ");
        if(line.size() == 2)
            request->insert( std::pair<std::string, std::string>(line[0], line[1]) );
        else
        {
            Http::send400Response(socket); // bad request
            return false;
        }
    }
//    if(request->at("Host").compare(host_) != 0)
//        return false;
    return true;
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
    while (!threadSet.empty())
        {
            removeConnection(*threadSet.begin());
        }
}

MeServiceBase::~MeServiceBase(){

        // remove and delete threads

    cancelAndDelete(requestService_);
    cancelAndDelete(subscriptionService_);
    delete currentRequestServed_;
    delete currentSubscriptionServed_;

    while(!requests_.isEmpty())
    {
        delete requests_.pop();;
    }

    while(!subscriptionEvents_.isEmpty())
    {
        delete subscriptionEvents_.pop();;

    }
    std::cout << "Subscriptions list length: " << subscriptions_.size() << std::endl;
    Subscriptions::iterator it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        std::cout << "Deleting subscription with id: " << it->second->getSubscriptionId() << std::endl;
        // stop periodic notification timer
        cMessage *msg =it->second->getNotificationTrigger();
        if(msg!= nullptr && msg->isScheduled())
            cancelAndDelete(it->second->getNotificationTrigger());
        delete it->second;
        subscriptions_.erase(it++);
    }
    std::cout << "Subscriptions list length: " << subscriptions_.size() << std::endl;
}
void MeServiceBase::emitRequestQueueLength()
{
    emit(requestQueueSizeSignal_, requests_.getLength());
}


//Http::DataType MeServiceBase::getDataType(std::string& packet_){
//    // HTTP request or HTTP response
//    if (packet_.rfind("HTTP", 0) == 0) { // is a response
//        // parse it
//        return Http::A;
//    }
//    else if(packet_.rfind("GET", 0) == 0 || packet_.rfind("POST", 0) == 0 ||
//            packet_.rfind("DELETE", 0) == 0 || packet_.rfind("PUT", 0) == 0)
//    {
//        return Http::B;
//        // is a request
//    }
//    else
//    {
//        return Http::C;
//    }
//
//}

void MeServiceBase::removeSubscritions(int connId)
{
    Subscriptions::iterator it = subscriptions_.begin();
    while (it != subscriptions_.end()) {
        if (it->second->getSocketConnId() == connId) {
            std::cout << "Remnove subscription with id = " << it->second->getSubscriptionId();
            // stop periodic notification timer
            cMessage *msg =it->second->getNotificationTrigger();
            if(msg!= nullptr && msg->isScheduled())
                cancelAndDelete(it->second->getNotificationTrigger());
            else if(msg!= nullptr && !msg->isScheduled())
                delete msg;                
            delete it->second;
            subscriptions_.erase(it++);
            std::cout << " list length: " << subscriptions_.size() << std::endl;
        } else {
           ++it;
        }
    }
}


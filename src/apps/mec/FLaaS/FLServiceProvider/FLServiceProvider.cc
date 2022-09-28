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

#include "apps/mec/FLaaS/FLServiceProvider/FLServiceProvider.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
//#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include <iostream>
#include <string>
#include <vector>
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"

//#include "apps/mec/MECServices/packets/HttpResponsePacket.h"
#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/EventNotification/EventNotification.h"

// REST resources
#include "apps/mec/FLaaS/FLServiceProvider/resources/FLServiceInfo.h"

Define_Module(FLServiceProvider);


FLServiceProvider::FLServiceProvider(){
    baseUriQueries_ = "/example/flaas/v1/queries";
    baseUriSubscriptions_ = "/example/location/v2/subscriptions";
    baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
    subscriptionId_ = 0;
    subscriptions_.clear();
    supportedQueryParams_.insert("address");
    supportedQueryParams_.insert("latitude");
    supportedQueryParams_.insert("longitude");
    supportedQueryParams_.insert("zone");

    // supportedQueryParams_s_.insert("ue_ipv6_address");
}

void FLServiceProvider::initialize(int stage)
{
    MecServiceBase::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        EV << "Host: " << host_+baseUriQueries_ << endl;

        subscriptionTimer_ = new AperiodicSubscriptionTimer("subscriptionTimer", 0.1);
    }
}

bool FLServiceProvider::manageSubscription()
{
    int subId = currentSubscriptionServed_->getSubId();
    if(subscriptions_.find(subId) != subscriptions_.end())
    {
        EV << "FLServiceProvider::manageSubscription() - subscription with id: " << subId << " found" << endl;
        SubscriptionBase * sub = subscriptions_[subId]; //upcasting (getSubscriptionType is in Subscriptionbase)
        sub->sendNotification(currentSubscriptionServed_);
        if(currentSubscriptionServed_!= nullptr)
            delete currentSubscriptionServed_;
        currentSubscriptionServed_ = nullptr;
        return true;
    }

    else{
        EV << "FLServiceProvider::manageSubscription() - subscription with id: " << subId << " not found. Removing from subscriptionTimer.." << endl;
        // the subscription has been deleted, e.g. due to closing socket
        // remove subId from AperiodicSubscription timer
        subscriptionTimer_->removeSubId(subId);
        if(subscriptionTimer_->getSubIdSetSize() == 0)
            cancelEvent(subscriptionTimer_);
        if(currentSubscriptionServed_!= nullptr)
                delete currentSubscriptionServed_;
        currentSubscriptionServed_ = nullptr;
        return false;
    }
}

void FLServiceProvider::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        if(msg->isName("subscriptionTimer"))
        {
            EV << "subscriptionTimer" << endl;
            AperiodicSubscriptionTimer *subTimer = check_and_cast<AperiodicSubscriptionTimer*>(msg);
            std::set<int> subIds = subTimer->getSubIdSet(); // TODO pass it as reference
            for(auto sub : subIds)
            {
                if(subscriptions_.find(sub) != subscriptions_.end())
                {
                    EV << "subscriptionTimer for subscription: " << sub << endl;
                    SubscriptionBase * subscription = subscriptions_[sub]; //upcasting (getSubscriptionType is in Subscriptionbase)
                    EventNotification *event = subscription->handleSubscription();
                    if(event != nullptr)
                        newSubscriptionEvent(event);
                }
                else
                {
                    EV << "remove subId " << sub << " from aperiodic trimer" << endl;
                    subTimer->removeSubId(sub);
                }
            }
            if(subTimer->getSubIdSetSize() > 0)
                scheduleAt(simTime()+subTimer->getPeriod(), msg);
            return;
        }

    }
    MecServiceBase::handleMessage(msg);
}

void FLServiceProvider::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV_INFO << "FLServiceProvider::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_+"/availableServices") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = lte::utils::splitString(params, "&");

            // parameters can be trainMode or category
            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> param;
            std::vector<std::string> splittedParam;

            // for now it only possible to have ONE parameters
            for(; it != end; ++it){
                if(it->rfind("trainMode", 0) == 0) // accessPointId=par1,par2
                {
                    EV <<"FLServiceProvider::handleGETReques - parameters: " << endl;
                    param = lte::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                       Http::send400Response(socket);
                       return;
                    }

                    //check param = SYNC ASYN or BOTH

                    FLServiceInfo flServiceListResource = FLServiceInfo(&flServiceList);
                    Http::send200Response(socket, flServiceListResource.toJsonUe(ues).dump(0).c_str());


                }
            }
        }
    }
    else if (uri.compare(baseUriQueries_+"/activeProcesses") == 0) // return the list of the subscriptions
    {
    }
    else if (uri.compare(baseUriQueries_+"/controllerEndpoint") == 0) // return the list of the subscriptions
    {
    }
    else // not found
    {
        Http::send404Response(socket);
    }
}

void FLServiceProvider::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "FLServiceProvider::handlePOSTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();

//    // uri must be in form example/location/v2/subscriptions/sub_type
//    // or
//    // example/location/v2/subscriptions/type/sub_type () e.g /area/circle
//    std::size_t lastPart = uri.find_last_of("/");
//    if(lastPart == std::string::npos)
//    {
//        EV << "FLServiceProvider::handlePOSTRequest - incorrect URI" << endl;
//        Http::send404Response(socket); //it is not a correct uri
//        return;
//    }
//    // find_last_of does not take in to account if the uri has a last /
//    // in this case subscriptionType would be empty and the baseUri == uri
//    // by the way the next if statement solves this problem
//    std::string baseUri = uri.substr(0,lastPart);
//    std::string subscriptionType =  uri.substr(lastPart+1);
//
//    EV << "FLServiceProvider::handlePOSTRequest - baseuri: "<< baseUri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(uri.compare(baseUriSubscriptions_+"/area/circle") == 0)
    {

    }
    else
    {
        Http::send404Response(socket); //resource not found
    }
}

void FLServiceProvider::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){
}

void FLServiceProvider::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
//    DELETE /exampleAPI/location/v1/subscriptions/area/circle/sub123 HTTP/1.1
//    Accept: application/xml
//    Host: example.com

    EV << "FLServiceProvider::handleDELETERequest" << endl;
    // uri must be in form example/location/v2/subscriptions/sub_type/subId
    // or
    // example/location/v2/subscriptions/type/sub_type/subId
    std::string uri = currentRequestMessageServed->getUri();
    std::size_t lastPart = uri.find_last_of("/");
//    if(lastPart == std::string::npos)
//    {
//        EV << "1" << endl;
//        Http::send404Response(socket); //it is not a correct uri
//        return;
//    }

    // find_last_of does not take in to account if the uri has a last /
    // in this case subscriptionType would be empty and the baseUri == uri
    // by the way the next if statement solve this problem
    std::string baseUri = uri.substr(0,lastPart);
    std::string ssubId =  uri.substr(lastPart+1);

    EV << "baseuri: "<< baseUri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(baseUri.compare(baseUriSubscriptions_+"/area/circle") == 0)
    {
        int subId = std::stoi(ssubId);
        Subscriptions::iterator it = subscriptions_.find(subId);
        if(it != subscriptions_.end())
        {
            // CircleNotificationSubscription *sub = (CircleNotificationSubscription*) it->second;
            subscriptionTimer_->removeSubId(subId);
            if(subscriptionTimer_->getSubIdSetSize() == 0 && subscriptionTimer_->isScheduled())
                cancelEvent(subscriptionTimer_);
            delete it->second;
            subscriptions_.erase(it);
            Http::send204Response(socket);
        }
        else
        {
            Http::send400Response(socket);
        }
    }
    else
    {
        Http::send404Response(socket);
    }
}

void FLServiceProvider::finish()
{
    MecServiceBase::finish();
    return;
}

FLServiceProvider::~FLServiceProvider(){
    cancelAndDelete(subscriptionTimer_);
return;
}





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

#include "nodes/mec/MECPlatform/MECServices/LocationService/LocationService.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/CircleNotificationSubscription.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
//#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include <iostream>
#include <string>
#include <vector>
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer.h"
//#include "apps/mec/MECServices/packets/HttpResponsePacket.h"
#include "common/utils/utils.h"



Define_Module(LocationService);


LocationService::LocationService(){
    baseUriQueries_ = "/example/location/v2/queries";
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

void LocationService::initialize(int stage)
{
    MecServiceBase::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        LocationResource_.addEnodeB(eNodeB_);
        LocationResource_.addBinder(binder_);
        LocationResource_.setBaseUri(host_+baseUriQueries_);
        EV << "Host: " << host_+baseUriQueries_ << endl;
        LocationSubscriptionEvent_ = new cMessage("LocationSubscriptionEvent");
        LocationSubscriptionPeriod_ = par("LocationSubscriptionPeriod");

        subscriptionTimer_ = new AperiodicSubscriptionTimer("subscriptionTimer", 0.1);
    }
}

bool LocationService::manageSubscription()
{
    int subId = currentSubscriptionServed_->getSubId();
    if(subscriptions_.find(subId) != subscriptions_.end())
    {
        EV << "LocationService::manageSubscription() - subscription with id: " << subId << " found" << endl;
        SubscriptionBase * sub = subscriptions_[subId]; //upcasting (getSubscriptionType is in Subscriptionbase)
        sub->sendNotification(currentSubscriptionServed_);
        if(currentSubscriptionServed_!= nullptr)
            delete currentSubscriptionServed_;
        currentSubscriptionServed_ = nullptr;
        return true;
    }

    else{
        EV << "LocationService::manageSubscription() - subscription with id: " << subId << " not found. Removing from subscriptionTimer.." << endl;
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

void LocationService::handleMessage(cMessage *msg)
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

void LocationService::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV_INFO << "LocationService::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
//    std::cout << uri << std::endl;
//    std::vector<std::string> splittedUri = lte::utils::splitString(uri, "?");
//    // uri must be in form example/v2/location/queries/resource
//    std::size_t lastPart = splittedUri[0].find_last_of("/");
//    if(lastPart == std::string::npos)
//    {
//        Http::send404Response(socket); //it is not a correct uri
//        return;
//    }
//    // find_last_of does not take in to account if the uri has a last /
//    // in this case resourceType would be empty and the baseUri == uri
//    // by the way the next if statement solves this problem
//    std::string baseUri = splittedUri[0].substr(0,lastPart);
//    std::string resourceType =  splittedUri[0].substr(lastPart+1);

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_+"/users") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = lte::utils::splitString(params, "&");
            /*
            * supported paramater:
            * - ue_ipv4_address
            * - accessPointId
            */

            std::vector<MacCellId> cellIds;
            std::vector<inet::Ipv4Address> ues;

            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> params;
            std::vector<std::string> splittedParams;
            for(; it != end; ++it){
                if(it->rfind("accessPointId", 0) == 0) // accessPointId=par1,par2
                {
                    EV <<"LocationService::handleGETReques - parameters: " << endl;
                    params = lte::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = lte::utils::splitString(params[1], ","); //it can an array, e.g param=v1,v2,v3
                    std::vector<std::string>::iterator pit  = splittedParams.begin();
                    std::vector<std::string>::iterator pend = splittedParams.end();
                    for(; pit != pend; ++pit){
                        EV << "cellId: " <<*pit << endl;
                        cellIds.push_back((MacCellId)std::stoi(*pit));
                    }
                }
                else if(it->rfind("address", 0) == 0)
                {
                    params = lte::utils::splitString(*it, "=");
                    splittedParams = lte::utils::splitString(params[1], ","); //it can an array, e.g param=v1,v2,v3
                    std::vector<std::string>::iterator pit  = splittedParams.begin();
                    std::vector<std::string>::iterator pend = splittedParams.end();
                    for(; pit != pend; ++pit){
                        std::vector<std::string> address = lte::utils::splitString((*pit), ":");
                        if(address.size()!= 2) //must be param=acr:values
                        {
                            Http::send400Response(socket);
                            return;
                        }
                       //manage ipv4 address without any macnode id
                        //or do the conversion inside Location..
                       ues.push_back(inet::Ipv4Address(address[1].c_str()));
                    }
                }
                else // bad parameters
                {
                    Http::send400Response(socket);
                    return;
                }

            }

            //send response
            if(!ues.empty() && !cellIds.empty())
            {
                EV <<"LocationService::handleGETReques - toJson(cellIds, ues) " << endl;
                Http::send200Response(socket, LocationResource_.toJson(cellIds, ues).dump(0).c_str());
            }
            else if(ues.empty() && !cellIds.empty())
            {
                EV <<"LocationService::handleGETReques - toJson(cellIds) " << endl;
                Http::send200Response(socket, LocationResource_.toJsonCell(cellIds).dump(0).c_str());
            }
            else if(!ues.empty() && cellIds.empty())
           {
               EV <<"LocationService::handleGETReques - toJson(ues) " << endl;
               Http::send200Response(socket, LocationResource_.toJsonUe(ues).dump(0).c_str());
           }
           else
           {
               Http::send400Response(socket);
           }

        }
        else
        { //no query params
            EV <<"LocationService::handleGETReques - toJson() " << endl;
            Http::send200Response(socket,LocationResource_.toJson().dump(0).c_str());
            return;
        }
    }
    else if (uri.compare(baseSubscriptionLocation_) == 0) // return the list of the subscriptions
    {
        // TODO implement list of subscriptions
        Http::send404Response(socket);
    }
    else // not found
    {
        Http::send404Response(socket);
    }

}

void LocationService::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "LocationService::handlePOSTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();

//    // uri must be in form example/location/v2/subscriptions/sub_type
//    // or
//    // example/location/v2/subscriptions/type/sub_type () e.g /area/circle
//    std::size_t lastPart = uri.find_last_of("/");
//    if(lastPart == std::string::npos)
//    {
//        EV << "LocationService::handlePOSTRequest - incorrect URI" << endl;
//        Http::send404Response(socket); //it is not a correct uri
//        return;
//    }
//    // find_last_of does not take in to account if the uri has a last /
//    // in this case subscriptionType would be empty and the baseUri == uri
//    // by the way the next if statement solves this problem
//    std::string baseUri = uri.substr(0,lastPart);
//    std::string subscriptionType =  uri.substr(lastPart+1);
//
//    EV << "LocationService::handlePOSTRequest - baseuri: "<< baseUri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(uri.compare(baseUriSubscriptions_+"/area/circle") == 0)
    {
        nlohmann::json jsonBody;
        try
        {
            jsonBody = nlohmann::json::parse(body); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
            std::cout << "LocationService::handlePOSTRequest" << e.what() << "\n" << body << std::endl;
            // body is not correctly formatted in JSON, manage it
            Http::send400Response(socket); // bad body JSON
            return;
        }

        CircleNotificationSubscription* newSubscription  = new CircleNotificationSubscription(subscriptionId_, socket , baseSubscriptionLocation_,  eNodeB_);
        bool res = newSubscription->fromJson(jsonBody);
        //correct subscription post
        if(res)
        {
            EV << serviceName_ << " - correct subscription created!" << endl;
            // add resource url and send back the response
            nlohmann::ordered_json response = jsonBody;
            std::string resourceUrl = newSubscription->getResourceUrl();
            response["circleNotificationSubscription"]["resourceURL"] = resourceUrl;
            std::pair<std::string, std::string> p("Location: ", resourceUrl);
            Http::send201Response(socket, response.dump(2).c_str(), p);

            subscriptions_[subscriptionId_] = newSubscription;

            if(newSubscription->getCheckImmediate())
            {
                EventNotification *event = newSubscription->handleSubscription();
                if(event != nullptr)
                    newSubscriptionEvent(event);
            }
            //start timer
            subscriptionTimer_->insertSubId(subscriptionId_);
            if(!subscriptionTimer_->isScheduled())
                scheduleAt(simTime() + subscriptionTimer_->getPeriod(), subscriptionTimer_);
            subscriptionId_ ++;
        }
        else
        {
            delete newSubscription;
            return;
        }
    }
    else
    {
        Http::send404Response(socket); //resource not found
    }
}

void LocationService::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){
    EV << "LocationService::handlePUTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();

    // uri must be in form example/location/v2/subscriptions/sub_type/subId
    // or
    // example/location/v2/subscriptions/type/sub_type/subId
//    std::size_t lastPart = uri.find_last_of("/");
//    if(lastPart == std::string::npos)
//    {
//        EV << "1" << endl;
//        Http::send404Response(socket); //it is not a correct uri
//        return;
//    }
    // find_last_of does not take in to account if the uri has a last /
    // in this case subscriptionType would be empty and the baseUri == uri
    // by the way the next if statement solves this problem

    std::size_t lastPart = uri.find_last_of("/"); // split at contextId
    std::string baseUri = uri.substr(0,lastPart); // uri
    int subId =  std::stoi(uri.substr(lastPart+1));

    EV << "LocationService::handlePUTRequest - baseuri: "<< baseUri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(baseUri.compare(baseUriSubscriptions_+"/area/circle") == 0)
    {
       Subscriptions::iterator it = subscriptions_.find(subId);
       if(it != subscriptions_.end())
       {
           nlohmann::json jsonBody;
           try
           {
               jsonBody = nlohmann::json::parse(body); // get the JSON structure
           }
           catch(nlohmann::detail::parse_error e)
           {
               std::cout <<  e.what() << std::endl;
               // body is not correctly formatted in JSON, manage it
               Http::send400Response(socket); // bad body JSON
               return;
           }

           CircleNotificationSubscription *sub = (CircleNotificationSubscription*) it->second;
           int id = sub->getSubscriptionId();
           CircleNotificationSubscription* newSubscription  = new CircleNotificationSubscription(id, socket , baseSubscriptionLocation_,  eNodeB_, sub->getFirstNotification(), sub->getLastoNotification());
           bool res = newSubscription->fromJson(jsonBody);
           if(res == true)
           {
               nlohmann::ordered_json response = jsonBody;
               std::string resourceUrl = newSubscription->getResourceUrl();
               response["circleNotificationSubscription"]["resourceURL"] = resourceUrl;
               Http::send200Response(socket, response.dump(2).c_str());
               delete it->second; // remove old subscription
               subscriptions_[id] = newSubscription; // replace with the new subscription
           }
           else
           {
               delete newSubscription; // delete the new created subscription
           }
       }
    }
    else
    {
        Http::send404Response(socket); // bad body JSON
    }

}

void LocationService::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
//    DELETE /exampleAPI/location/v1/subscriptions/area/circle/sub123 HTTP/1.1
//    Accept: application/xml
//    Host: example.com

    EV << "LocationService::handleDELETERequest" << endl;
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

void LocationService::finish()
{
    MecServiceBase::finish();
    return;
}

LocationService::~LocationService(){
    cancelAndDelete(LocationSubscriptionEvent_);
    cancelAndDelete(subscriptionTimer_);
return;
}





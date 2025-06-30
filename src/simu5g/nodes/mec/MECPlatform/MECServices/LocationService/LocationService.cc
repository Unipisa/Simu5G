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

#include <iostream>
#include <string>
#include <vector>

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/tcp/TcpSocket.h>

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/LocationService.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/CircleNotificationSubscription.h"
#include "nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer_m.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

Define_Module(LocationService);

LocationService::LocationService() {
    baseUriQueries_ = "/example/location/v2/queries";
    baseUriSubscriptions_ = "/example/location/v2/subscriptions";
    baseSubscriptionLocation_ = host_ + baseUriSubscriptions_ + "/";
    subscriptionId_ = 0;
    supportedQueryParams_.insert("address");
    supportedQueryParams_.insert("latitude");
    supportedQueryParams_.insert("longitude");
    supportedQueryParams_.insert("zone");

    // supportedQueryParams_s_.insert("ue_ipv6_address");
}

void LocationService::initialize(int stage)
{
    MecServiceBase2::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        LocationResource_.addEnodeB(eNodeB_);
        LocationResource_.addBinder(binder_);
        LocationResource_.setBaseUri(host_ + baseUriQueries_);
        EV << "Host: " << host_ + baseUriQueries_ << endl;
        LocationSubscriptionEvent_ = new cMessage("LocationSubscriptionEvent");
        LocationSubscriptionPeriod_ = par("LocationSubscriptionPeriod");

        subscriptionTimer_ = new AperiodicSubscriptionTimer("subscriptionTimer");
        subscriptionTimer_->setPeriod(0.1);
    }
}

bool LocationService::manageSubscription()
{
    int subId = currentSubscriptionServed_->getSubId();
    if (subscriptions_.find(subId) != subscriptions_.end()) {
        EV << "LocationService::manageSubscription() - subscription with id: " << subId << " found" << endl;
        SubscriptionBase *sub = subscriptions_[subId];//upcasting (getSubscriptionType is in SubscriptionBase)
        sub->sendNotification(currentSubscriptionServed_);
        if (currentSubscriptionServed_ != nullptr)
            delete currentSubscriptionServed_;
        currentSubscriptionServed_ = nullptr;
        return true;
    }
    else {
        EV << "LocationService::manageSubscription() - subscription with id: " << subId << " not found. Removing from subscriptionTimer.." << endl;
        // the subscription has been deleted, e.g. due to closing socket
        // remove subId from AperiodicSubscription timer
        subscriptionTimer_->removeSubId(subId);
        if (subscriptionTimer_->getSubIdSetSize() == 0)
            cancelEvent(subscriptionTimer_);
        if (currentSubscriptionServed_ != nullptr)
            delete currentSubscriptionServed_;
        currentSubscriptionServed_ = nullptr;
        return false;
    }
}

void LocationService::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == subscriptionTimer_) {
            EV << "subscriptionTimer" << endl;
            std::set<int> subIds = subscriptionTimer_->getSubIdSet(); // TODO pass it as reference
            for (auto sub : subIds) {
                if (subscriptions_.find(sub) != subscriptions_.end()) {
                    EV << "subscriptionTimer for subscription: " << sub << endl;
                    SubscriptionBase *subscription = subscriptions_[sub];//upcasting (getSubscriptionType is in SubscriptionBase)
                    EventNotification *event = subscription->handleSubscription();
                    if (event != nullptr)
                        newSubscriptionEvent(event);
                }
                else {
                    EV << "remove subId " << sub << " from aperiodic timer" << endl;
                    subscriptionTimer_->removeSubId(sub);
                }
            }
            if (subscriptionTimer_->getSubIdSetSize() > 0)
                scheduleAt(simTime() + subscriptionTimer_->getPeriod(), subscriptionTimer_);
            return;
        }
    }
    MecServiceBase::handleMessage(msg);
}

void LocationService::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)
{
    EV_INFO << "LocationService::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    // check it is a GET for a query or a subscription
    if (uri == baseUriQueries_ + "/users") { //queries
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if (!params.empty()) {
            std::vector<std::string> queryParameters = simu5g::utils::splitString(params, "&");
            /*
             * supported parameters:
             * - ue_ipv4_address
             * - accessPointId
             */

            std::vector<MacCellId> cellIds;
            std::vector<inet::Ipv4Address> ues;

            std::vector<std::string> params;
            std::vector<std::string> splittedParams;
            for (const auto& queryParam : queryParameters) {
                if (queryParam.rfind("accessPointId", 0) == 0) { // accessPointId=par1,par2
                    EV << "LocationService::handleGETRequest - parameters: " << endl;
                    params = simu5g::utils::splitString(queryParam, "=");
                    if (params.size() != 2) { //must be param=values
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = simu5g::utils::splitString(params[1], ","); //it can be an array, e.g param=v1,v2,v3
                    for (const auto& cellIdStr : splittedParams) {
                        EV << "cellId: " << cellIdStr << endl;
                        cellIds.push_back((MacCellId)std::stoi(cellIdStr));
                    }
                }
                else if (queryParam.rfind("address", 0) == 0) {
                    params = simu5g::utils::splitString(queryParam, "=");
                    splittedParams = simu5g::utils::splitString(params[1], ","); //it can be an array, e.g param=v1,v2,v3
                    for (const auto& addressStr : splittedParams) {
                        std::vector<std::string> address = simu5g::utils::splitString(addressStr, ":");
                        if (address.size() != 2) { //must be param=acr:values
                            Http::send400Response(socket);
                            return;
                        }
                        //manage ipv4 address without any macnode id
                        //or do the conversion inside Location..
                        ues.push_back(inet::Ipv4Address(address[1].c_str()));
                    }
                }
                else { // bad parameters
                    Http::send400Response(socket);
                    return;
                }
            }

            //send response
            if (!ues.empty() && !cellIds.empty()) {
                EV << "LocationService::handleGETRequest - toJson(cellIds, ues) " << endl;
                Http::send200Response(socket, LocationResource_.toJson(cellIds, ues).dump(0).c_str());
            }
            else if (ues.empty() && !cellIds.empty()) {
                EV << "LocationService::handleGETRequest - toJson(cellIds) " << endl;
                Http::send200Response(socket, LocationResource_.toJsonCell(cellIds).dump(0).c_str());
            }
            else if (!ues.empty() && cellIds.empty()) {
                EV << "LocationService::handleGETRequest - toJson(ues) " << endl;
                Http::send200Response(socket, LocationResource_.toJsonUe(ues).dump(0).c_str());
            }
            else {
                Http::send400Response(socket);
            }
        }
        else { //no query params
            EV << "LocationService::handleGETRequest - toJson() " << endl;
            Http::send200Response(socket, LocationResource_.toJson().dump(0).c_str());
            return;
        }
    }
    else if (uri == baseSubscriptionLocation_) { // return the list of the subscriptions
        // TODO implement list of subscriptions
        Http::send404Response(socket);
    }
    else { // not found
        Http::send404Response(socket);
    }
}

void LocationService::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)
{
    EV << "LocationService::handlePOSTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();

//
    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if (uri == (baseUriSubscriptions_ + "/area/circle")) {
        nlohmann::json jsonBody;
        try {
            jsonBody = nlohmann::json::parse(body); // get the JSON structure
        }
        catch (nlohmann::detail::parse_error e) {
            std::cout << "LocationService::handlePOSTRequest" << e.what() << "\n" << body << std::endl;
            // body is not correctly formatted in JSON, manage it
            Http::send400Response(socket); // bad body JSON
            return;
        }
        CircleNotificationSubscription *newSubscription = new CircleNotificationSubscription(binder_, subscriptionId_, socket, baseSubscriptionLocation_, eNodeB_);
        bool res = newSubscription->fromJson(jsonBody);
        //correct subscription post
        if (res) {
            EV << serviceName_ << " - correct subscription created!" << endl;
            // add resource url and send back the response
            nlohmann::ordered_json response = jsonBody;
            std::string resourceUrl = newSubscription->getResourceUrl();
            response["circleNotificationSubscription"]["resourceURL"] = resourceUrl;
            std::pair<std::string, std::string> p("Location: ", resourceUrl);
            Http::send201Response(socket, response.dump(2).c_str(), p);

            subscriptions_[subscriptionId_] = newSubscription;

            if (newSubscription->getCheckImmediate()) {
                EventNotification *event = newSubscription->handleSubscription();
                if (event != nullptr)
                    newSubscriptionEvent(event);
            }
            //start timer
            subscriptionTimer_->insertSubId(subscriptionId_);
            if (!subscriptionTimer_->isScheduled())
                scheduleAt(simTime() + subscriptionTimer_->getPeriod(), subscriptionTimer_);
            subscriptionId_++;
        }
        else {
            delete newSubscription;
            return;
        }
    }
    else {
        Http::send404Response(socket); //resource not found
    }
}

void LocationService::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {
    EV << "LocationService::handlePUTRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();

    // uri must be in form example/location/v2/subscriptions/sub_type/subId
    // or
    // example/location/v2/subscriptions/type/sub_type/subId
    // find_last_of does not take into account if the uri has a last /
    // in this case subscriptionType would be empty and the baseUri == uri
    // by the way the next if statement solves this problem

    std::size_t lastPart = uri.find_last_of("/"); // split at contextId
    std::string baseUri = uri.substr(0, lastPart); // uri
    int subId = std::stoi(uri.substr(lastPart + 1));

    EV << "LocationService::handlePUTRequest - baseuri: " << baseUri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if (baseUri == (baseUriSubscriptions_ + "/area/circle")) {
        Subscriptions::iterator it = subscriptions_.find(subId);
        if (it != subscriptions_.end()) {
            nlohmann::json jsonBody;
            try {
                jsonBody = nlohmann::json::parse(body); // get the JSON structure
            }
            catch (nlohmann::detail::parse_error e) {
                std::cout << e.what() << std::endl;
                // body is not correctly formatted in JSON, manage it
                Http::send400Response(socket); // bad body JSON
                return;
            }
            CircleNotificationSubscription *sub = check_and_cast<CircleNotificationSubscription *>(it->second);
            int id = sub->getSubscriptionId();
            CircleNotificationSubscription *newSubscription = new CircleNotificationSubscription(binder_, id, socket, baseSubscriptionLocation_, eNodeB_, sub->getFirstNotification(), sub->getLastNotification());
            bool res = newSubscription->fromJson(jsonBody);
            if (res) {
                nlohmann::ordered_json response = jsonBody;
                std::string resourceUrl = newSubscription->getResourceUrl();
                response["circleNotificationSubscription"]["resourceURL"] = resourceUrl;
                Http::send200Response(socket, response.dump(2).c_str());
                delete it->second; // remove old subscription
                subscriptions_[id] = newSubscription; // replace with the new subscription
            }
            else {
                delete newSubscription; // delete the new created subscription
            }
        }
    }
    else {
        Http::send404Response(socket); // bad body JSON
    }
}

void LocationService::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)
{
    EV << "LocationService::handleDELETERequest" << endl;
    // URI must be in the form example/location/v2/subscriptions/sub_type/subId
    // or
    // example/location/v2/subscriptions/type/sub_type/subId
    std::string uri = currentRequestMessageServed->getUri();
    std::size_t lastPart = uri.find_last_of("/");
    // find_last_of does not take into account if the URI has a last /
    // in this case subscriptionType would be empty and the baseUri == uri
    // by the way, the next if statement solves this problem
    std::string baseUri = uri.substr(0, lastPart);
    std::string ssubId = uri.substr(lastPart + 1);

    EV << "baseuri: " << baseUri << endl;

    // it has to be managed the case when the subscription is /area/circle (it has two slashes)
    if (baseUri == (baseUriSubscriptions_ + "/area/circle")) {
        int subId = std::stoi(ssubId);
        Subscriptions::iterator it = subscriptions_.find(subId);
        if (it != subscriptions_.end()) {
            // CircleNotificationSubscription *sub = check_and_cast<CircleNotificationSubscription *>(it->second);
            subscriptionTimer_->removeSubId(subId);
            if (subscriptionTimer_->getSubIdSetSize() == 0 && subscriptionTimer_->isScheduled())
                cancelEvent(subscriptionTimer_);
            delete it->second;
            subscriptions_.erase(it);
            Http::send204Response(socket);
        }
        else {
            Http::send400Response(socket);
        }
    }
    else {
        Http::send404Response(socket);
    }
}

void LocationService::finish()
{
    MecServiceBase::finish();
}

LocationService::~LocationService() {
    cancelAndDelete(LocationSubscriptionEvent_);
    cancelAndDelete(subscriptionTimer_);
}

} //namespace


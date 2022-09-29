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
#include "apps/mec/FLaaS/FLServiceProvider/resources/FLProcessInfo.h"


Define_Module(FLServiceProvider);


FLServiceProvider::FLServiceProvider(){
    baseUriQueries_ = "/example/flaas/v1/";
    baseUriSubscriptions_ = "/example/location/v2/subscriptions";
    baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
    subscriptionId_ = 0;
    subscriptions_.clear();
}

void FLServiceProvider::initialize(int stage)
{
    MecServiceBase::initialize(stage);
    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        EV << "Host: " << host_+baseUriQueries_ << endl;
//        subscriptionTimer_ = new AperiodicSubscriptionTimer("subscriptionTimer", 0.1);

        // onboard FLservices at initialization time
        onboardFLServices();

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
    if(uri.compare(baseUriQueries_+"/queries/availableServices") == 0 ) //queries
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
                    EV <<"FLServiceProvider::handleGETReques - parameters: trainMode " << endl;
                    param = lte::utils::splitString(*it, "=");
                    if(param.size()!= 2) //must be param=values
                    {
                       Http::send400Response(socket);
                       return;
                    }

                    //check param = SYNC ASYN or BOTH
                    if(isTrainingMode(param[1]))
                    {
                        FLServiceInfo flServiceListResource = FLServiceInfo(&flServices_);
                        Http::send200Response(socket, flServiceListResource.toJson(toTrainingMode(param[1])).dump(0).c_str());
                    }
                    else
                    {
                        Http::send404Response(socket);
                        return;
                    }
                }
                else if(it->rfind("category", 0) == 0) // accessPointId=par1,par2
                {
                    EV <<"FLServiceProvider::handleGETReques - parameters: category " << endl;
                    param = lte::utils::splitString(*it, "=");
                    if(param.size()!= 2) //must be param=values
                    {
                       Http::send400Response(socket);
                       return;
                    }
                    FLServiceInfo flServiceListResource = FLServiceInfo(&flServices_);
                    Http::send200Response(socket, flServiceListResource.toJson(param[1]).dump(0).c_str());
                }
                else
                {
                    Http::send404Response(socket);
                    return;
                }
            }
        }
        else
        {
            FLServiceInfo flServiceListResource = FLServiceInfo(&flServices_);
            Http::send200Response(socket, flServiceListResource.toJson().dump(0).c_str());
        }
    }
    else if (uri.compare(baseUriQueries_+"/queries/activeProcesses") == 0) // return the list of the subscriptions
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
                   EV <<"FLServiceProvider::handleGETReques - parameters: trainMode " << endl;
                   param = lte::utils::splitString(*it, "=");
                   if(param.size()!= 2) //must be param=values
                   {
                      Http::send400Response(socket);
                      return;
                   }

                   //check param = SYNC ASYN or BOTH
                   if(isTrainingMode(param[1]))
                   {
                       FLProcessInfo flProcessListResource = FLProcessInfo(&flProcesses_);
                       Http::send200Response(socket, flProcessListResource.toJson(toTrainingMode(param[1])).dump(0).c_str());
                   }
                   else
                   {
                       Http::send404Response(socket);
                       return;
                   }
               }
               else if(it->rfind("category", 0) == 0) // accessPointId=par1,par2
               {
                   EV <<"FLServiceProvider::handleGETReques - parameters: category " << endl;
                   param = lte::utils::splitString(*it, "=");
                   if(param.size()!= 2) //must be param=values
                   {
                      Http::send400Response(socket);
                      return;
                   }
                   FLProcessInfo flProcessListResource = FLProcessInfo(&flProcesses_);
                   Http::send200Response(socket, flProcessListResource.toJson(param[1]).dump(0).c_str());
               }
               else
               {
                   Http::send404Response(socket);
                   return;
               }
           }
        }
        else
        {
            FLProcessInfo flProcessListResource = FLProcessInfo(&flProcesses_);
            Http::send200Response(socket, flProcessListResource.toJson().dump(0).c_str());
        }

    }
    else if (uri.compare(baseUriQueries_+"/queries/controllerEndpoint") == 0) // return the list of the subscriptions
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
            for(; it != end; ++it){
                if(it->rfind("processId", 0) == 0) // controllerId=par1
                {
                    EV <<"FLServiceProvider::handleGETReques - parameters: processId" << endl;
                    param = lte::utils::splitString(*it, "=");
                    if(param.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }

                    // check if the ID is present
                    bool found = false;
                    for(const auto& process : flProcesses_)
                    {
                        if(process.second.isFLProcessActive() && process.second.getFLProcessId().compare(param[1]))
                        {
                            found = true;
                            break;
                        }
                    }
                    //check param = SYNC ASYN or BOTH
                    if(found)
                    {
                        FLProcessInfo flProcessListResource = FLProcessInfo(&flProcesses_);
                        Http::send200Response(socket, flProcessListResource.toJson(toTrainingMode(param[1])).dump(0).c_str());
                    }
                    else
                    {
                        Http::send404Response(socket);
                        return;
                    }
                }
            }
        }
        else
        {
            Http::send404Response(socket);
            return;
        }

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

//    // uri must be in form example/flaas/v1/operations/res

    EV << "FLServiceProvider::handlePOSTRequest - baseuri: "<< uri << endl;

    if(uri.compare(baseUriQueries_+"/operations/addFLService") == 0)
    {

        nlohmann::json jsonBody;
        try
        {
            jsonBody = nlohmann::json::parse(body); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
            std::cout << "FLServiceProvider::handlePOSTRequest" << e.what() << "\n" << body << std::endl;
            // body is not correctly formatted in JSON, manage it
            Http::send400Response(socket); // bad body JSON
            return;
        }

        std::string name = jsonBody["name"];
        std::string flServiceId = jsonBody["serviceId"];
        std::string description = jsonBody["description"];
        std::string category = jsonBody["category"];

        std::string trainingMode = jsonBody["training mode"];
        FLTrainingMode trainingMode_enum ;
        if(isTrainingMode(trainingMode))
            trainingMode_enum = toTrainingMode(trainingMode);
        else
        {
            Http::send400Response(socket); // bad body JSON
            return;
        }

        std::string flControllerUri = jsonBody["controller URI"];

        FLService newFLService(name, flServiceId, description, category, trainingMode_enum);

        if(flServices_.find(newFLService.getFLServiceId()) != flServices_.end())
        {
            EV << "FLServiceProvider::handlePOSTRequest- FL service with serviceId [" << newFLService.getFLServiceId() << "] already present" << endl;
            Http::send400Response(socket); // bad body JSON
            return;
        }
        else
        {
            EV << "FLServiceProvider::handlePOSTRequest- nre FL service with serviceId [" << newFLService.getFLServiceId() << "] added!" << endl;
            flServices_[newFLService.getFLServiceId()] = newFLService;
            std::string resourceUrl = baseUriSubscriptions_+"/operations/addFLService/" + newFLService.getFLServiceId();
            jsonBody["resourceURL"] = resourceUrl;
            std::pair<std::string, std::string> locationHeader("Location: ", resourceUrl);
            Http::send201Response(socket, jsonBody.dump(2).c_str(), locationHeader);
        }
    }
    else if(uri.compare(baseUriSubscriptions_+"/operations/startFLProcess") == 0)
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

    std::string baseUri = uri.substr(0,lastPart);
    std::string flService =  uri.substr(lastPart+1);

    EV << "baseuri: "<< baseUri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(baseUri.compare(baseUriQueries_+"/operations/deleteFLService") == 0)
    {
        if(flServices_.find(flService) != flServices_.end())
        {
            flServices_.erase(flService);
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


void FLServiceProvider::onboardFLServices()
{
    //getting the list of mec hosts associated to this mec system from parameter
    if(this->hasPar("flServicesList") && strcmp(par("flServicesList").stringValue(), "")){
        std::string flServiceList = par("flServicesList").stdstringValue();
        char* token = strtok((char*)flServiceList.c_str() , ", "); // split by commas
        while (token != NULL)
        {
            int len = strlen(token);
            char buf[len+strlen(".json")+strlen("FLServices/")+1];
            strcpy(buf,"FLServices/");
            strcat(buf,token);
            strcat(buf,".json");
            onboardFLService(buf);
            token = strtok (NULL, ", ");
        }
    }
    else{
        EV << "FLServiceProvider::onboardFLServices - No flServicesList found" << endl;
    }
}

const FLService& FLServiceProvider::onboardFLService(const char* fileName)
{
    EV <<"FLServiceProvider::onboardFLService - onboarding FLService: "<< fileName << endl;
    FLService flService(fileName);
    if(flServices_.find(flService.getFLServiceId()) != flServices_.end())
    {
        EV << "FLServiceProvider::onboardFLService - FLService with name ["<< fileName << "] is already present.\n" << endl;
    //        throw cRuntimeError("FLServiceProvider::onboardFLService - FLService with name [%s] is already present.\n"
    //                            "Duplicate FLServiceId or FLService already onboarded?", fileName);
    }
    else
    {
        flServices_[flService.getFLServiceId()] = flService; // add to the FLServices
    }

    return flServices_[flService.getFLServiceId()];
}

double FLServiceProvider::calculateRequestServiceTime()
{
    EV << "FLServiceProvider::calculateRequestServiceTime" << endl;
    if(std::strcmp(currentRequestMessageServed_->getMethod(),"POST") == 0)
    {
        std::string uri = currentRequestMessageServed_->getUri();
        if(uri.compare(baseUriSubscriptions_+"/operations/addFLService") == 0)
        {
            EV << "FLServiceProvider::calculateRequestServiceTime - addFLService" << endl;
            return 1.; // time to instantiate MEC apps
        }
        else
        {
            return MecServiceBase::calculateRequestServiceTime();
        }
    }
    else
    {
        return MecServiceBase::calculateRequestServiceTime();
    }
}


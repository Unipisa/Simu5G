//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


// INET
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

#include <iostream>
#include <string>
#include <vector>

// Utils
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "common/utils/utils.h"

// MEC system
#include "nodes/mec/LCMProxy/LcmProxy.h"
#include "nodes/mec/MECOrchestrator/MecOrchestrator.h"
#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"


// Messages
#include "nodes/mec/LCMProxy/LCMProxyMessages/CreateContextAppMessage.h"
#include "nodes/mec/LCMProxy/LCMProxyMessages/CreateContextAppAckMessage.h"
#include "nodes/mec/LCMProxy/LCMProxyMessages/LcmProxyMessages_m.h"
#include "nodes/mec/LCMProxy/LCMProxyMessages/LCMProxyMessages_types.h"

Define_Module(LcmProxy);

LcmProxy::LcmProxy()
{
    baseUriQueries_ = "/example/dev_app/v1";
    baseUriSubscriptions_ = baseUriQueries_;
    supportedQueryParams_.insert("app_list");
    supportedQueryParams_.insert("app_contexts");
    scheduledSubscription = false;
    mecOrchestrator_ = nullptr;
    requestSno = 0;
    subscriptionId_ = 0;
    subscriptions_.clear();
}

void LcmProxy::initialize(int stage)
{
    EV << "LcmProxy::initialize stage " << stage << endl;
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
        currentRequestMessageServed_ = nullptr;
        currentSubscriptionServed_ = nullptr;

        requestQueueSizeSignal_ = registerSignal("requestQueueSize");
        binder_ = getBinder();
    }

    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
        mecOrchestrator_ = check_and_cast<MecOrchestrator*>(getSimulation()->getModuleByPath("mecOrchestrator"));
    }

    inet::ApplicationBase::initialize(stage);
}

void LcmProxy::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "LcmProxy::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;
    inet::L3Address localAdd(inet::L3AddressResolver().resolve(localAddress));
    EV << "Local Address resolved: "<< localAdd << endl;

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

void LcmProxy::handleMessageWhenUp(cMessage *msg)
{
    if(msg->arrivedOn("fromMecOrchestrator"))
    {
        // manage here the messages from mecOrchestrator
        LcmProxyMessage * lcmMsg = check_and_cast<LcmProxyMessage*>(msg);
        if(strcmp(lcmMsg->getType(), ACK_CREATE_CONTEXT_APP) == 0)
        {
            handleCreateContextAppAckMessage(lcmMsg);
        }
        else if(strcmp(lcmMsg->getType(), ACK_DELETE_CONTEXT_APP) == 0)
        {
            handleDeleteContextAppAckMessage(lcmMsg);
        }

        pendingRequests.erase(lcmMsg->getRequestId());
        delete msg;

        return;
    }
    else
    {
        MecServiceBase::handleMessageWhenUp(msg);

    }
}

void LcmProxy::handleCreateContextAppAckMessage(LcmProxyMessage *msg)
{
    CreateContextAppAckMessage* ack = check_and_cast<CreateContextAppAckMessage*>(msg);
    unsigned int reqSno = ack->getRequestId();

    EV << "LcmProxy::handleCreateContextAppAckMessage - reqSno: " << reqSno << endl;

    auto req = pendingRequests.find(reqSno);
    if(req == pendingRequests.end())
    {
        EV << "LcmProxy::handleCreateContextAppAckMessage - reqSno: " << reqSno<< " not present in pendingRequests. Discarding... "<<endl;
        return;
    }

    int connId = req->second.connId;

    EV << "LcmProxy::handleCreateContextAppAckMessage - reqSno: " << reqSno << " related to connid: "<< connId << endl;

    nlohmann::json jsonBody = req->second.appCont;

    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(connId));

    if(socket)
    {
        if(ack->getSuccess())
        {

            jsonBody["contextId"] = std::to_string(ack->getContextId());
            jsonBody["appInfo"]["userAppInstanceInfo"]["appInstanceId"] = ack->getAppInstanceId();
            jsonBody["appInfo"]["userAppInstanceInfo"]["referenceURI"]  = ack->getAppInstanceUri(); // add the end point
//            jsonBody["appInfo"]["userAppInstanceInfo"]["appLocation"]; // TODO not implemented yet
            std::stringstream uri;
            uri << baseUriQueries_<<"/app_contexts/"<< ack->getContextId();
            std::pair<std::string, std::string> locHeader("Location: ", uri.str());
            Http::send201Response(socket, jsonBody.dump().c_str(), locHeader);

        }
        else
        {
            Http::ProblemDetailBase pd;
            pd.type = "Request not succesfully completed";
            pd.title = "CreateContext request result";
            pd.detail = "the Mec system was not able to instantiate the mec application";
            pd.status = "500";
            Http::send500Response(socket, pd.toJson().dump().c_str());
        }

    }
    return;
}

void LcmProxy::handleDeleteContextAppAckMessage(LcmProxyMessage *msg)
{
    DeleteContextAppAckMessage* ack = check_and_cast<DeleteContextAppAckMessage*>(msg);

    unsigned int reqSno = ack->getRequestId();

    auto req = pendingRequests.find(reqSno);
    if(req == pendingRequests.end())
    {
        EV << "LcmProxy::handleDeleteContextAppAckMessage - reqSno: " << reqSno<< " not present in pendingRequests. Discarding... "<<endl;
        return;
    }

    int connId = req->second.connId;
    EV << "LcmProxy::handleDeleteContextAppAckMessage - reqSno: " << reqSno << " related to connid: "<< connId << endl;

    inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.getSocketById(connId));

    if(socket)
    {
        if(ack->getSuccess() == true)
            Http::send204Response(socket);
        else
        {
            Http::ProblemDetailBase pd;
            pd.type = "Request not succesfully completed";
            pd.title = "DeleteContext request result";
            pd.detail = "the Mec system was not able to terminate the mec application";
            pd.status = "500";
            Http::send500Response(socket, pd.toJson().dump().c_str());
        }

    }
    return;
}

void LcmProxy::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    EV << "LcmProxy::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_+"/app_list") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
       //look for query parameters
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = lte::utils::splitString(params, "&");
            /*
            * supported paramater:
            * - appName
            */

            std::vector<std::string> appNames;

            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> params;
            std::vector<std::string> splittedParams;

            for(; it != end; ++it){
                if(it->rfind("appName", 0) == 0) // cell_id=par1,par2
                {
                    EV <<"LcmProxy::handleGETRequest - parameters: " << endl;
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
                        EV << "appName: " <<*pit << endl;
                        appNames.push_back(*pit);
                    }
                }
                else // bad parameters
                {
                    Http::send400Response(socket);
                    return;
                }
            }

            nlohmann::ordered_json appList;

            // construct the result based on the appName vector
            for(auto appName : appNames)
            {
                const ApplicationDescriptor* appDesc = mecOrchestrator_->getApplicationDescriptorByAppName(appName);
                if(appDesc != nullptr)
                {
                    appList["appList"].push_back(appDesc->toAppInfo());
                }
            }
            // if the appList is empty, send an empty 200 response
            Http::send200Response(socket, appList.dump().c_str());
        }


        else { //no query params
            nlohmann::ordered_json appList;
            auto appDescs = mecOrchestrator_->getApplicationDescriptors();
            auto it = appDescs->begin();
            for(; it != appDescs->end() ; ++it)
            {
                appList["appList"].push_back(it->second.toAppInfo());
            }

            Http::send200Response(socket, appList.dump().c_str());
        }
    }

    else //bad uri
    {
        Http::send404Response(socket);

    }

}

void LcmProxy::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    std::string uri = currentRequestMessageServed->getUri();
    std::string body = currentRequestMessageServed->getBody();
    EV << "LcmProxy::handlePOSTRequest - uri: "<< uri << endl;

    // it has to be managed the case when the sub is /area/circle (it has two slashes)
    if(uri.compare(baseUriSubscriptions_+"/app_contexts") == 0)
    {
        nlohmann::json jsonBody;
        try
        {
            jsonBody = nlohmann::json::parse(body); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
            throw cRuntimeError("LcmProxy::handlePOSTRequest - %s", e.what());
            // body is not correctly formatted in JSON, manage it
            Http::send400Response(socket); // bad body JSON
            return;
        }

        // Parse the JSON  body to organize the App instantion
        CreateContextAppMessage * createContext = parseContextCreateRequest(jsonBody);
        if(createContext != nullptr)
        {
            createContext->setType(CREATE_CONTEXT_APP);
            createContext->setRequestId(requestSno);
            createContext->setConnectionId(socket->getSocketId());
            createContext->setAppContext(jsonBody);

            createContext->setUeIpAddress(socket->getRemoteAddress().str().c_str());
            pendingRequests[requestSno] = {socket->getSocketId(), requestSno, jsonBody};

            EV << "POST request number: " << requestSno << " related to connId: " << socket->getSocketId() << endl;

            requestSno++;

            send(createContext, "toMecOrchestrator");
        }
        else
        {
            Http::send400Response(socket); // bad body JSON
        }
    }
    else
    {
        Http::send404Response(socket); //
    }
}

void LcmProxy::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){
    Http::send404Response(socket, "PUT not implemented, yet");
}

void LcmProxy::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    //    DELETE /exampleAPI/location/v1/subscriptions/area/circle/sub123 HTTP/1.1
    //    Accept: application/xml
    //    Host: example.com

    EV << "LocationService::handleDELETERequest" << endl;
    // uri must be in form /example/dev_app/v1/app_context/contextId
    std::string uri = currentRequestMessageServed->getUri();

//    // it has to be managed the case when the sub is /area/circle (it has two slashes)

    std::size_t lastPart = uri.find_last_of("/"); // split at contextId
    std::string baseUri = uri.substr(0,lastPart); // uri
    std::string contextId =  uri.substr(lastPart+1); // contextId

    if(baseUri.compare(baseUriSubscriptions_+"/app_contexts") == 0)
    {
        DeleteContextAppMessage * deleteContext = new DeleteContextAppMessage();
        deleteContext->setRequestId(requestSno);
        deleteContext->setType(DELETE_CONTEXT_APP);
        deleteContext->setContextId(stoi(contextId));

        pendingRequests[requestSno].connId = socket->getSocketId();
        pendingRequests[requestSno].requestId = requestSno;
        EV << "DELETE request number: " << requestSno << " related to connId: " << socket->getSocketId() << endl;

        requestSno++;

        send(deleteContext, "toMecOrchestrator");
    }
    else
    {
        Http::send404Response(socket);
    }

}

CreateContextAppMessage* LcmProxy::parseContextCreateRequest(const nlohmann::json& jsonBody)
{
    if(!jsonBody.contains("appInfo"))
    {
        EV << "LcmProxy::parseContextCreateRequest - appInfo attribute not found" << endl;
        return nullptr;
    }

    nlohmann::json appInfo = jsonBody["appInfo"];

    if(!jsonBody.contains("associateDevAppId"))
    {
       EV << "LcmProxy::parseContextCreateRequest - associateDevAppId attribute not found" << endl;
       return nullptr;
    }

    std::string devAppId = jsonBody["associateDevAppId"];

    // the mec app package is already onboarded (from the device application pov)
    if(appInfo.contains("appDId"))
    {
        CreateContextAppMessage* createContext = new CreateContextAppMessage();
        createContext->setOnboarded(true);
        createContext->setDevAppId(devAppId.c_str());
        std::string appDId = appInfo["appDId"];
        createContext->setAppDId(appDId.c_str());
        return  createContext;
    }

    // the mec app package is not onboarded, but the uri of the app package is provided
    else if (appInfo.contains("appPackageSource"))
    {
        CreateContextAppMessage* createContext = new CreateContextAppMessage();
        createContext->setOnboarded(false);
        createContext->setDevAppId(devAppId.c_str());
        std::string appPackageSource = appInfo["appPackageSource"];
        createContext->setAppPackagePath(appPackageSource.c_str());
        return  createContext;
    }
    else
    {
    // neither the two is present and this is not allowed
        return nullptr;
    }
}

void LcmProxy::finish()
{
    return;
}

LcmProxy::~LcmProxy(){
}

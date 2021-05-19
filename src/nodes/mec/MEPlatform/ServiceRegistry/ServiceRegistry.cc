//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "ServiceRegistry.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "common/utils/utils.h"


Define_Module(ServiceRegistry);

using namespace omnetpp;

ServiceRegistry::ServiceRegistry() {
    baseUriQueries_ = "/example/mec_service_mgmt/v1";
    baseUriSubscriptions_ = baseUriQueries_;
    supportedQueryParams_.insert("app_list");
    supportedQueryParams_.insert("app_contexts");
    mecServices_.clear();
    uuidBase = "123e4567-e89b-12d3-a456-4266141"; // last 5 digits are missing and used to create uniquely id in a quicker way
    servIdCounter = 10000; // incremented every news service and concatened to the uuidBase
}

ServiceRegistry::~ServiceRegistry() {
    // TODO Auto-generated destructor stub
}

void ServiceRegistry::initialize(int stage)
{
    EV << "ServiceRegistry::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    inet::ApplicationBase::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    binder_ = getBinder();

    mePlatform = getParentModule();
    if(mePlatform != NULL){
        meHost = mePlatform->getParentModule();
    }
    else{
        EV << "ServiceRegistry::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("ServiceRegistry::initialize - \tFATAL! Error when getting getParentModule()");
    }


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

    baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";

}


void ServiceRegistry::handleStartOperation(inet::LifecycleOperation *operation)
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


void ServiceRegistry::handleGETRequest(const std::string& uri, inet::TcpSocket* socket){
    EV << "ServiceRegistry::handleGETRequest" << endl;
       std::vector<std::string> splittedUri = lte::utils::splitString(uri, "?");
       // uri must be in form example/mec_service_mgmt/v1
       std::size_t lastPart = splittedUri[0].find_last_of("/");
       if(lastPart == std::string::npos)
       {
           Http::send404Response(socket); //it is not a correct uri
           return;
       }
       // find_last_of does not take in to account if the uri has a last /
       // in this case resourceType would be empty and the baseUri == uri
       // by the way the next if statement solve this problem
       std::string baseUri = splittedUri[0].substr(0,lastPart);
       std::string resourceType =  splittedUri[0].substr(lastPart+1);

       // check it is a GET for a query or a subscription
       if(baseUri.compare(baseUriQueries_) == 0 ) //queries
       {
           if(resourceType.compare("services") == 0 )
           {
               //look for query parameters
               if(splittedUri.size() == 2) // uri has parameters eg. uriPath?param=value&param1=value
               {
                   std::vector<std::string> queryParameters = lte::utils::splitString(splittedUri[1], "&");
                   /*
                   * supported paramater:
                   * - ser_instance_id
                   * - ser_name
                   */

                   std::vector<std::string> ser_name;
                   std::vector<std::string> ser_instance_id;

                   std::vector<std::string>::iterator it  = queryParameters.begin();
                   std::vector<std::string>::iterator end = queryParameters.end();
                   std::vector<std::string> params;
                   std::vector<std::string> splittedParams;
                   for(; it != end; ++it){
                       if(it->rfind("ser_instance_id", 0) == 0) // cell_id=par1,par2
                        {
                          EV <<"ServiceRegistry::handleGETReques - parameters: " << endl;
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
                              EV << "ser_instance_id: " <<*pit << endl;
                              ser_instance_id.push_back(*pit);
                          }
                        }
                        else if(it->rfind("ser_name", 0) == 0)
                        {
                          params = lte::utils::splitString(*it, "=");
                          splittedParams = lte::utils::splitString(params[1], ","); //it can an array, e.g param=v1,v2,v3
                          std::vector<std::string>::iterator pit  = splittedParams.begin();
                          std::vector<std::string>::iterator pend = splittedParams.end();
                          for(; pit != pend; ++pit){
                              EV << "ser_name: " <<*pit << endl;
                              ser_name.push_back(*pit);
                          }
                       }
                       else // bad parameters
                       {
                           Http::send400Response(socket);
                           return;
                       }
                   }

                   nlohmann::ordered_json jsonResBody;

                       for(auto sName : ser_name)
                       {
                           if(mecServices_.find(sName) != mecServices_.end())
                           {
                               jsonResBody.push_back(mecServices_.at(sName).toJson());
                           }
                       }
                       Http::send200Response(socket, jsonResBody.dump().c_str());
                }

                else if (splittedUri.size() == 1 ){ //no query params
                    nlohmann::ordered_json jsonResBody;
                    for(auto& sName : mecServices_)
                    {
                        jsonResBody.push_back(sName.second.toJson());
                    }
                Http::send200Response(socket, jsonResBody.dump().c_str());
               }
           }
       }
}


void ServiceRegistry::handlePOSTRequest(const std::string& uri, const std::string& body, inet::TcpSocket* socket){};
void ServiceRegistry::handlePUTRequest(const std::string& uri, const std::string& body, inet::TcpSocket* socket) {};
void ServiceRegistry::handleDELETERequest(const std::string& uri, inet::TcpSocket* socket) {};

void ServiceRegistry::registerMeService(const ServiceDescriptor& servDesc)
{
    if(mecServices_.find(servDesc.name) != mecServices_.end())
    {
        throw cRuntimeError("ServiceRegistry::registerMeService - %s is already present!", servDesc.name.c_str());
    }

    /*
     * create a ServiceInfo object to maintain the service information
     */

    EndPointInfo endPoint(servDesc.addr.str(), servDesc.port);
    TransportInfo tInfo(servDesc.transportId, servDesc.transportName, servDesc.transportType, servDesc.transportProtocol, endPoint);
    CategoryRef catRef(servDesc.catHref, servDesc.catId, servDesc.catName, servDesc.catVersion);

    std::string serInstanceId = uuidBase + std::to_string(servIdCounter++);
    ServiceInfo servInfo(serInstanceId, servDesc.name, catRef, servDesc.version, "ACTIVE" , tInfo, servDesc.serialize, "MEC_HOST", true, true);

    mecServices_[servDesc.name] = servInfo;

    EV << "ServiceRegistry::registerMeService - "<< servInfo.toJson().dump(2) << " added!" << endl;


}
//
SockAddr ServiceRegistry::retrieveMeService(const std::string& MeServiceName)
{

    SockAddr e;
    return e;

}

const MecServicesMap* ServiceRegistry::getAvailableServices() const
{
    return &mecServices_;
}















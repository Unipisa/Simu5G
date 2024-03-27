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
    servIdCounter = 10000; // incremented every new service and concatenate it to the uuidBase
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

    /* since Mec Service will register at init stage INITSTAGE_APPLICATION_LAYER,
     * be sure that the ServiceRegistry is ready
     */
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER - 1)
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


    EV << "ServiceRegistry::initialize" << endl;

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

    baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
}


void ServiceRegistry::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "ServiceRegistry::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;

    // e.g. 1.2.3.4:5050
    std::stringstream hostStream;
    hostStream << localAddress<< ":" << localPort;
    host_ = hostStream.str();

    serverSocket.setOutputGate(gate("socketOut"));
    serverSocket.setCallback(this);
    serverSocket.bind(inet::L3Address(), localPort); // bind socket for any address

    serverSocket.listen();
}


void ServiceRegistry::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){
    EV << "ServiceRegistry::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_+"/services") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = lte::utils::splitString(params, "&");
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
                for(const auto& serv : mecServices_)
                {
                    if(serv.getName().compare(sName) == 0)
                        jsonResBody.push_back(serv.toJson());
                }
            }

            // TODO add for for serviceid!
            Http::send200Response(socket, jsonResBody.dump().c_str());
        }

        else //no query params
        {
            nlohmann::ordered_json jsonResBody;
            for(auto& sName : mecServices_)
            {
                jsonResBody.push_back(sName.toJson());
            }
            Http::send200Response(socket, jsonResBody.dump().c_str());
        }
   }
   else
   {
       Http::send404Response(socket); //it is not a correct uri
   }
}


void ServiceRegistry::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){};
void ServiceRegistry::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) {};
void ServiceRegistry::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) {};

void ServiceRegistry::registerMecService(const ServiceDescriptor& servDesc)
{

    for(const auto& serv : mecServices_)
    {
        if(serv.getName().compare(servDesc.name) == 0 && serv.getMecHost().compare(servDesc.mecHostname) == 0)
        {
            throw cRuntimeError("ServiceRegistry::registerMeService - %s is already present in MEC host %s!", servDesc.name.c_str(), servDesc.mecHostname.c_str());
        }
    }

    /*
     * create a ServiceInfo object to maintain the service information
     */

    EndPointInfo endPoint(servDesc.addr.str(), servDesc.port);
    TransportInfo tInfo(servDesc.transportId, servDesc.transportName, servDesc.transportType, servDesc.transportProtocol, endPoint);
    CategoryRef catRef(servDesc.catHref, servDesc.catId, servDesc.catName, servDesc.catVersion);

    std::string serInstanceId = uuidBase + std::to_string(servIdCounter++);


    bool isLocal = (strcmp(meHost->getName(), servDesc.mecHostname.c_str()) == 0) ? true : false;



    ServiceInfo servInfo(serInstanceId, servDesc.name, catRef, servDesc.version, "ACTIVE" , tInfo, servDesc.serialize, servDesc.mecHostname ,servDesc.scopeOfLocality , servDesc.isConsumedLocallyOnly, isLocal);

    mecServices_.push_back(servInfo);

    EV << "ServiceRegistry::registerMeService - "<< servInfo.toJson().dump(2) << "\nadded!" << endl;


}

const std::vector<ServiceInfo>* ServiceRegistry::getAvailableMecServices() const
{
    return &mecServices_;
}














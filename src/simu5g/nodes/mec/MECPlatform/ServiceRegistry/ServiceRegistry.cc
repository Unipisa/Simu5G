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

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include <inet/networklayer/common/L3AddressResolver.h>

#include "common/utils/utils.h"

namespace simu5g {

Define_Module(ServiceRegistry);

using namespace omnetpp;

ServiceRegistry::ServiceRegistry()
{
    baseUriQueries_ = "/example/mec_service_mgmt/v1";
    baseUriSubscriptions_ = baseUriQueries_;
    supportedQueryParams_.insert("app_list");
    supportedQueryParams_.insert("app_contexts");
}


void ServiceRegistry::initialize(int stage)
{
    EV << "ServiceRegistry::initialize - stage " << stage << endl;

    MecServiceBase::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER - 1) {
        baseSubscriptionLocation_ = host_ + baseUriSubscriptions_ + "/";
    }
}

void ServiceRegistry::handleStartOperation(inet::LifecycleOperation *operation)
{
    EV << "ServiceRegistry::handleStartOperation" << endl;
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");
    EV << "Local Address: " << localAddress << " port: " << localPort << endl;

    // e.g. 1.2.3.4:5050
    std::stringstream hostStream;
    hostStream << localAddress << ":" << localPort;
    host_ = hostStream.str();

    serverSocket.setOutputGate(gate("socketOut"));
    serverSocket.setCallback(this);
    serverSocket.bind(inet::L3Address(), localPort); // bind socket for any address

    int tos = par("tos");
    if (tos != -1)
        serverSocket.setTos(tos);

    serverSocket.listen();
}

void ServiceRegistry::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {
    EV << "ServiceRegistry::handleGETRequest" << endl;
    std::string uri = currentRequestMessageServed->getUri();

    // check if it is a GET for a query or a subscription
    if (uri == (baseUriQueries_ + "/services")) { //queries
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if (!params.empty()) {
            std::vector<std::string> queryParameters = simu5g::utils::splitString(params, "&");
            /*
             * supported parameters:
             * - ser_instance_id
             * - ser_name
             */

            std::vector<std::string> ser_name;
            std::vector<std::string> ser_instance_id;

            for (const auto& queryParam : queryParameters) {
                if (queryParam.rfind("ser_instance_id", 0) == 0) { // cell_id=par1,par2
                    EV << "ServiceRegistry::handleGETRequest - parameters: " << endl;
                    auto params = simu5g::utils::splitString(queryParam, "=");
                    if (params.size() != 2) { //must be param=values
                        Http::send400Response(socket);
                        return;
                    }
                    auto splittedParams = simu5g::utils::splitString(params[1], ","); //it can be an array, e.g param=v1,v2,v3
                    for (const auto& pit : splittedParams) {
                        EV << "ser_instance_id: " << pit << endl;
                        ser_instance_id.push_back(pit);
                    }
                }
                else if (queryParam.rfind("ser_name", 0) == 0) {
                    auto params = simu5g::utils::splitString(queryParam, "=");
                    auto splittedParams = simu5g::utils::splitString(params[1], ","); //it can be an array, e.g param=v1,v2,v3
                    for (const auto& pit : splittedParams) {
                        EV << "ser_name: " << pit << endl;
                        ser_name.push_back(pit);
                    }
                }
                else { // bad parameters
                    Http::send400Response(socket);
                    return;
                }
            }

            nlohmann::ordered_json jsonResBody;

            for (const auto& sName : ser_name) {
                for (const auto& serv : mecServices_) {
                    if (serv.getName() == sName)
                        jsonResBody.push_back(serv.toJson());
                }
            }

            // TODO: add for serviceid!
            Http::send200Response(socket, jsonResBody.dump().c_str());
        }
        else { //no query params
            nlohmann::ordered_json jsonResBody;
            for (auto& sName : mecServices_) {
                jsonResBody.push_back(sName.toJson());
            }
            Http::send200Response(socket, jsonResBody.dump().c_str());
        }
    }
    else {
        Http::send404Response(socket); //it is not a correct uri
    }
}

void ServiceRegistry::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {};
void ServiceRegistry::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {};
void ServiceRegistry::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {};

void ServiceRegistry::registerMecService(const ServiceDescriptor& servDesc)
{
    for (const auto& serv : mecServices_) {
        if (serv.getName() == servDesc.name && serv.getMecHost() == servDesc.mecHostname) {
            throw cRuntimeError("ServiceRegistry::registerMecService - %s is already present in MEC host %s!", servDesc.name.c_str(), servDesc.mecHostname.c_str());
        }
    }

    /*
     * create a ServiceInfo object to maintain the service information
     */

    EndPointInfo endPoint(servDesc.addr.str(), servDesc.port);
    TransportInfo tInfo(servDesc.transportId, servDesc.transportName, servDesc.transportType, servDesc.transportProtocol, endPoint);
    CategoryRef catRef(servDesc.catHref, servDesc.catId, servDesc.catName, servDesc.catVersion);

    std::string serInstanceId = uuidBase + std::to_string(servIdCounter++);

    bool isLocal = (servDesc.mecHostname == meHost_->getName());

    ServiceInfo servInfo(serInstanceId, servDesc.name, catRef, servDesc.version, "ACTIVE", tInfo, servDesc.serialize, servDesc.mecHostname, servDesc.scopeOfLocality, servDesc.isConsumedLocallyOnly, isLocal);

    mecServices_.push_back(servInfo);

    EV << "ServiceRegistry::registerMecService - " << servInfo.toJson().dump(2) << "\nadded!" << endl;
}

const std::vector<ServiceInfo> *ServiceRegistry::getAvailableMecServices() const
{
    return &mecServices_;
}

} //namespace


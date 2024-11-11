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

#include "nodes/mec/MECPlatform/MECServices/RNIService/RNIService.h"

#include <iostream>
#include <string>
#include <vector>

#include <inet/applications/tcpapp/GenericAppMsg_m.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/lifecycle/NodeStatus.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/contract/ipv4/Ipv4Address.h>
#include <inet/transportlayer/contract/tcp/TcpCommand_m.h>
#include <inet/transportlayer/contract/tcp/TcpSocket.h>

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"

namespace simu5g {

Define_Module(RNIService);

RNIService::RNIService():L2MeasResource_() {
    baseUriQueries_ = "/example/rni/v2/queries";
    baseUriSubscriptions_ = "/example/rni/v2/subscriptions";
    supportedQueryParams_.insert("cell_id");
    supportedQueryParams_.insert("ue_ipv4_address");
    // supportedQueryParams_.insert("ue_ipv6_address");
}

void RNIService::initialize(int stage)
{
    MecServiceBase2::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        L2MeasResource_.setBinder(binder_);
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        L2MeasResource_.addEnodeB(eNodeB_);
        baseSubscriptionLocation_ = host_ + baseUriSubscriptions_ + "/";
    }
}

void RNIService::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)
{
    std::string uri = currentRequestMessageServed->getUri();

    // check if it is a GET for a query or a subscription
    if (uri == (baseUriQueries_ + "/layer2_meas")) { //queries
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if (!params.empty()) {
            std::vector<std::string> queryParameters = simu5g::utils::splitString(params, "&");
            /*
             * supported parameters:
             * - cell_id
             * - ue_ipv4_address
             * - ue_ipv6_address // not implemented yet
             */

            std::vector<MacNodeId> cellIds;
            std::vector<inet::Ipv4Address> ues;

            std::map<std::string, std::vector<std::string>> queryParamsMap; // e.g cell_id -> [0, 1]

            std::vector<std::string> params;
            std::vector<std::string> splittedParams;
            for (const auto &queryParam : queryParameters) {
                if (queryParam.rfind("cell_id", 0) == 0) { // cell_id=par1,par2
                    params = simu5g::utils::splitString(queryParam, "=");
                    if (params.size() != 2) { //must be param=values
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = simu5g::utils::splitString(params[1], ",");
                    for (const auto &cellIdParam : splittedParams) {
                        cellIds.push_back((MacNodeId)std::stoi(cellIdParam));
                    }
                }
                else if (queryParam.rfind("ue_ipv4_address", 0) == 0) {
                    // TO DO manage acr:10.12
                    params = simu5g::utils::splitString(queryParam, "=");
                    if (params.size() != 2) { //must be param=values
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = simu5g::utils::splitString(params[1], ",");
                    for (const auto &ueAddress : splittedParams)
                        ues.push_back(inet::Ipv4Address(ueAddress.c_str()));
                }
                else { // bad parameters
                    Http::send400Response(socket);
                    return;
                }
            }

            //send response
            if (!ues.empty() && !cellIds.empty()) {
                Http::send200Response(socket, L2MeasResource_.toJson(cellIds, ues).dump().c_str());
            }
            else if (ues.empty() && !cellIds.empty()) {
                Http::send200Response(socket, L2MeasResource_.toJsonCell(cellIds).dump().c_str());
            }
            else if (!ues.empty() && cellIds.empty()) {
                Http::send200Response(socket, L2MeasResource_.toJsonUe(ues).dump().c_str());
            }
            else {
                Http::send400Response(socket);
            }
        }
        else {
            //no query params
            Http::send200Response(socket, L2MeasResource_.toJson().dump().c_str());
            return;
        }
    }
    else if (uri == baseUriSubscriptions_) { //subs
        // TODO implement subscription?
        Http::send404Response(socket);
    }
    else { // not found
        Http::send404Response(socket);
    }
}

void RNIService::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {}

void RNIService::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) {}

void RNIService::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)
{
}

void RNIService::finish()
{
}


} //namespace


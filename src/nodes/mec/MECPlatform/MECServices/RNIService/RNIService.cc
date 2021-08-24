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

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include <iostream>
#include "nodes/mec/MECPlatform/MECServices/RNIService/RNIService.h"

#include <string>
#include <vector>
//#include "apps/mec/MECServices/packets/HttpResponsePacket.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "common/utils/utils.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
Define_Module(RNIService);


RNIService::RNIService():L2MeasResource_(){
    baseUriQueries_ = "/example/rni/v2/queries";
    baseUriSubscriptions_ = "/example/rni/v2/subscriptions";
    supportedQueryParams_.insert("cell_id");
    supportedQueryParams_.insert("ue_ipv4_address");
    // supportedQueryParams_s_.insert("ue_ipv6_address");
}

void RNIService::initialize(int stage)
{
    MecServiceBase::initialize(stage);

    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        L2MeasResource_.addEnodeB(eNodeB_);
        baseSubscriptionLocation_ = host_+ baseUriSubscriptions_ + "/";
    }
}


void RNIService::handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
    std::string uri = currentRequestMessageServed->getUri();
    // std::vector<std::string> splittedUri = lte::utils::splitString(uri, "?");
    // // uri must be in form example/v1/rni/queries/resource
    // std::size_t lastPart = splittedUri[0].find_last_of("/");
    // if(lastPart == std::string::npos)
    // {
    //     Http::send404Response(socket); //it is not a correct uri
    //     return;
    // }
    // // find_last_of does not take in to account if the uri has a last /
    // // in this case resourceType would be empty and the baseUri == uri
    // // by the way the next if statement solve this problem
    // std::string baseUri = splittedUri[0].substr(0,lastPart);
    // std::string resourceType =  splittedUri[0].substr(lastPart+1);

    // check it is a GET for a query or a subscription
    if(uri.compare(baseUriQueries_ + "/layer2_meas") == 0 ) //queries
    {
        std::string params = currentRequestMessageServed->getParameters();
        //look for query parameters
        if(!params.empty())
        {
            std::vector<std::string> queryParameters = lte::utils::splitString(params, "&");
            /*
            * supported paramater:
            * - cell_id
            * - ue_ipv4_address
            * - ue_ipv6_address // not implemented yet
            */

            std::vector<MacNodeId> cellIds;
            std::vector<inet::Ipv4Address> ues;

            typedef std::map<std::string, std::vector<std::string>> queryMap;
            queryMap queryParamsMap; // e.g cell_id -> [0, 1]

            std::vector<std::string>::iterator it  = queryParameters.begin();
            std::vector<std::string>::iterator end = queryParameters.end();
            std::vector<std::string> params;
            std::vector<std::string> splittedParams;
            for(; it != end; ++it){
                if(it->rfind("cell_id", 0) == 0) // cell_id=par1,par2
                {
                    params = lte::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = lte::utils::splitString(params[1], ",");
                    std::vector<std::string>::iterator pit  = splittedParams.begin();
                    std::vector<std::string>::iterator pend = splittedParams.end();
                    for(; pit != pend; ++pit){
                        cellIds.push_back((MacNodeId)std::stoi(*pit));
                    }
                }
                else if(it->rfind("ue_ipv4_address", 0) == 0)
                {
                    // TO DO manage acr:10.12
                    params = lte::utils::splitString(*it, "=");
                    if(params.size()!= 2) //must be param=values
                    {
                        Http::send400Response(socket);
                        return;
                    }
                    splittedParams = lte::utils::splitString(params[1], ",");
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
                Http::send200Response(socket, L2MeasResource_.toJson(cellIds, ues).dump().c_str());
            }
            else if(ues.empty() && !cellIds.empty())
            {
                Http::send200Response(socket, L2MeasResource_.toJsonCell(cellIds).dump().c_str());
            }
            else if(!ues.empty() && cellIds.empty())
           {
              Http::send200Response(socket, L2MeasResource_.toJsonUe(ues).dump().c_str());
           }
           else
           {
               Http::send400Response(socket);
           }

        }
        else{
            //no query params
            Http::send200Response(socket,L2MeasResource_.toJson().dump().c_str());
            return;
        }
    }

    else if (uri.compare(baseUriSubscriptions_) == 0) //subs
    {
        // TODO implement subscription?
        Http::send404Response(socket);
    }
    else // not found
    {
        Http::send404Response(socket);
    }

}

void RNIService::handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){}

void RNIService::handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket){}

void RNIService::handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)
{
}

void RNIService::finish()
{
// TODO
    return;
}

RNIService::~RNIService(){

}







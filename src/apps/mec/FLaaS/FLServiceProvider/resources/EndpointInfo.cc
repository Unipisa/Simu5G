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


#include "EndpointInfo.h"

EndpointInfo::EndpointInfo()
{
    flService_ = nullptr;
}

EndpointInfo::EndpointInfo(FLService* flService)
{
    flService_ = flService;
}


nlohmann::ordered_json EndpointInfo::toJson() const
{
    if(flService_ == nullptr)
           throw omnetpp::cRuntimeError("EndpointInfo::toJson - flService_ is null!");

    nlohmann::ordered_json address;

    Endpoint ep = flService_->getFLControllerEndpoint();
    address["address"]["host"] = ep.addr.str();
    address["address"]["port"] = ep.port;

    return address;
}

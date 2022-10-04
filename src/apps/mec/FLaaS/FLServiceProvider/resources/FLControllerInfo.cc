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


#include "FLControllerInfo.h"

FLControllerInfo::FLControllerInfo()
{
    flProcess_ = nullptr;
}

FLControllerInfo::FLControllerInfo(FLProcess* flProcess)
{
    flProcess_ = flProcess;
}


nlohmann::ordered_json FLControllerInfo::toJson() const
{
    if(flProcess_ == nullptr)
           throw omnetpp::cRuntimeError("EndpointInfo::toJson - flProcess_ is null!");

    nlohmann::ordered_json controllerInfo;

    controllerInfo["FLProcessId"] = flProcess_->getFLProcessId();
    Endpoint ep = flProcess_->getFLControllerEndpoint();
    controllerInfo["timestamp"] = omnetpp::simTime().dbl();
    controllerInfo["address"]["host"] = ep.addr.str();
    controllerInfo["address"]["port"] = ep.port;
//    controllerInfo["token"] = "null";

    return controllerInfo;
}




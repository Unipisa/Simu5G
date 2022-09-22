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


#include "apps/mec/FLaaS/FLServiceProvider/FLService.h"

FLService::FLService(std::string& name, std::string& flServiceId, std::string description, std::string& category,  FLTrainingMode traininigMode)
{
    name_ = name;
    flServiceId_ = flServiceId;
    description_ = description;
    isActive_ = false;
    flControllerEndpoint_.addr = inet::L3Address();
    flControllerEndpoint_.port = 0;
    category_ = category;
    trainingMode_ = traininigMode;
}

std::string FLService::getFLTrainingModeStr()
{
    if(trainingMode_ == SYNCHRONOUS)
        return "SYNCHRONOUS";
    else if(trainingMode_ == ASYNCHRONOUS)
        return "ASYNCHRONOUS";
    else if(trainingMode_ == BOTH)
            return "BOTH";
    else
        throw omnetpp::cRuntimeError("FLService::getFLTrainingModeStr() - FL Training Mode not recognized ");
}


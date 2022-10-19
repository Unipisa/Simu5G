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
#include <fstream>
#include <string>
#include "nodes/mec/utils/httpUtils/json.hpp"

int FLService::staticFlServiceIdNumeric_ = 0;

FLService::FLService()
{
    flServiceIdNumeric_ = staticFlServiceIdNumeric_++;
}


FLService::FLService(const char* fileName)
{
    // read a JSON file
    std::ifstream rawFile(fileName);
    if(!rawFile.good())
        throw omnetpp::cRuntimeError("FLService::FLService file: %s does not exist", fileName);

    nlohmann::json jsonFile;
    rawFile >> jsonFile;

    name_ = jsonFile["name"];
    flServiceId_ = jsonFile["serviceId"];
    description_ = jsonFile["description"];
    category_ = jsonFile["category"];

    std::string trainingMode = jsonFile["training mode"];
    if(isTrainingMode(trainingMode))
        trainingMode_ = toTrainingMode(trainingMode);
    else
        throw omnetpp::cRuntimeError("FLService::FLService - training mode %s not correct!", trainingMode.c_str());
    flControllerUri_ = jsonFile["controller URI"];
//    this->printApplicationDescriptor();
}

FLService::FLService(std::string& name, std::string& flServiceId, std::string description, std::string& category,  FLTrainingMode traininigMode)
{
    name_ = name;
    flServiceId_ = flServiceId;
    description_ = description;
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


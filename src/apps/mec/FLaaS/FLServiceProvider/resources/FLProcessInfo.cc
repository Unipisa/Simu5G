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


#include "apps/mec/FLaaS/FLServiceProvider/resources/FLProcessInfo.h"


FLProcessInfo::FLProcessInfo()
{
    flServices_ = nullptr;
}

FLProcessInfo::FLProcessInfo(std::map<std::string, FLService>* flServices, std::set<std::string>* activeServices)
{
    flServices_ = flServices;
    flActiveServices_ = activeServices;
}


nlohmann::ordered_json FLProcessInfo::toJson() const
{
    if(flServices_ == nullptr || flActiveServices_== nullptr)
        throw omnetpp::cRuntimeError("FLProcessInfo::toJson - flServices_ is null!");

    nlohmann::ordered_json serviceArray;
    nlohmann::ordered_json val ;
    nlohmann::ordered_json flProcessList;

    auto service = flServices_->begin();
    for(; service != flServices_->end(); ++service)
    {
        if(service->second.isFlServiceActive())
        {
            nlohmann::ordered_json serviceJson;
            serviceJson["name"] = service->second.getFlServiceName();
            serviceJson["id"] = service->second.getFlServiceId();
            serviceJson["description"] = service->second.getFlServiceDescription();
            serviceJson["category"] = service->second.getFLCategory();
            serviceJson["FLTrainingMode"] = service->second.getFLTrainingModeStr();
            serviceJson["currentAccuracy"] = service->second.getFLCurrentModelAccuracy();
            serviceArray.push_back(serviceJson);
        }
    }
    //TODO check is array is empty
    flProcessList["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
    flProcessList["FLServiceList"] = serviceArray;
    return flProcessList;
}

nlohmann::ordered_json FLProcessInfo::toJson(std::set<std::string>& serviceIds) const
{
    if(flServices_ == nullptr)
        throw omnetpp::cRuntimeError("FLProcessInfo::toJson - flServices_ is null!");

    nlohmann::ordered_json serviceArray;
    nlohmann::ordered_json val ;
    nlohmann::ordered_json flProcessList;

    auto it = serviceIds.begin();
    for(; it != serviceIds.end(); ++it)
    {
        auto service = flServices_->find(*it);
        if(service != flServices_->end() && service->second.isFlServiceActive())
        {
            nlohmann::ordered_json serviceJson;
            serviceJson["name"] = service->second.getFlServiceName();
            serviceJson["id"] = service->second.getFlServiceId();
            serviceJson["description"] = service->second.getFlServiceDescription();
            serviceJson["category"] = service->second.getFLCategory();
            serviceJson["FLTrainingMode"] = service->second.getFLTrainingModeStr();
            serviceJson["currentAccuracy"] = service->second.getFLCurrentModelAccuracy();
            serviceArray.push_back(serviceJson);
        }
    }

    //TODO check is array is empty
    flProcessList["timestamp"] = omnetpp::simTime().dbl();
    flProcessList["FLServiceList"] = serviceArray;
    return flProcessList;
}

nlohmann::ordered_json FLProcessInfo::toJson(FLTrainingMode mode) const
{
    if(flServices_ == nullptr)
            throw omnetpp::cRuntimeError("FLProcessInfo::toJson - flServices_ is null!");

        nlohmann::ordered_json serviceArray;
        nlohmann::ordered_json val ;
        nlohmann::ordered_json flProcessList;

        auto it = flServices_->begin();
        for(; it != flServices_->end(); ++it)
        {
            if(it->second.getFLTrainingMode() == mode && it->second.isFlServiceActive())
            {
                nlohmann::ordered_json service;
                service["name"] = it->second.getFlServiceName();
                service["id"] = it->second.getFlServiceId();
                service["description"] = it->second.getFlServiceDescription();
                service["category"] = it->second.getFLCategory();
                service["FLTrainingMode"] = it->second.getFLTrainingModeStr();
                serviceArray.push_back(service);
            }
        }
        //TODO check is array is empty
        flProcessList["timestamp"] = omnetpp::simTime().dbl();
        flProcessList["FLServiceList"] = serviceArray;
        return flProcessList;
}





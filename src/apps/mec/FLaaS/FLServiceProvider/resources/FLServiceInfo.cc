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


#include "apps/mec/FLaaS/FLServiceProvider/resources/FLServiceInfo.h"


FLServiceInfo::FLServiceInfo()
{
    flServices_ = nullptr;
}

FLServiceInfo::FLServiceInfo(std::map<std::string, FLService>* flServices)
{
    flServices_ = flServices;
}


nlohmann::ordered_json FLServiceInfo::toJson() const
{
    if(flServices_ == nullptr)
        throw omnetpp::cRuntimeError("FLServiceInfo::toJson - flServices_ is null!");

    nlohmann::ordered_json serviceArray;
    nlohmann::ordered_json val ;
    nlohmann::ordered_json flServiceList;

    auto it = flServices_->begin();
    for(; it != flServices_->end(); ++it)
    {
        nlohmann::ordered_json service;
        service["name"] = it->second.getFLServiceName();
        service["id"] = it->second.getFLServiceId();
        service["description"] = it->second.getFLServiceDescription();
        service["category"] = it->second.getFLCategory();
        service["FLTrainingMode"] = it->second.getFLTrainingModeStr();
        serviceArray.push_back(service);
    }
    //TODO check is array is empty
    flServiceList["FLServiceList"] = serviceArray;
    return flServiceList;
}

nlohmann::ordered_json FLServiceInfo::toJson(std::set<std::string>& serviceIds) const
{
    if(flServices_ == nullptr)
        throw omnetpp::cRuntimeError("FLServiceInfo::toJson - flServices_ is null!");

    nlohmann::ordered_json serviceArray;
    nlohmann::ordered_json val ;
    nlohmann::ordered_json flServiceList;

    auto it = serviceIds.begin();
    for(; it != serviceIds.end(); ++it)
    {
        auto service = flServices_->find(*it);
        if(service != flServices_->end())
        {
            nlohmann::ordered_json serviceJson;
            serviceJson["name"] = service->second.getFLServiceName();
            serviceJson["id"] = service->second.getFLServiceId();
            serviceJson["description"] = service->second.getFLServiceDescription();
            serviceJson["category"] = service->second.getFLCategory();
            serviceJson["FLTrainingMode"] = service->second.getFLTrainingModeStr();
            serviceArray.push_back(serviceJson);
        }
    }

    //TODO check is array is empty
    flServiceList["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
    flServiceList["FLServiceList"] = serviceArray;
    return flServiceList;
}

nlohmann::ordered_json FLServiceInfo::toJson(FLTrainingMode mode) const
{
    if(flServices_ == nullptr)
            throw omnetpp::cRuntimeError("FLServiceInfo::toJson - flServices_ is null!");

        nlohmann::ordered_json serviceArray;
        nlohmann::ordered_json val ;
        nlohmann::ordered_json flServiceList;

        auto it = flServices_->begin();
        for(; it != flServices_->end(); ++it)
        {
            if(it->second.getFLTrainingMode() == mode || mode == BOTH)
            {
                nlohmann::ordered_json service;
                service["name"] = it->second.getFLServiceName();
                service["id"] = it->second.getFLServiceId();
                service["description"] = it->second.getFLServiceDescription();
                service["category"] = it->second.getFLCategory();
                service["FLTrainingMode"] = it->second.getFLTrainingModeStr();
                serviceArray.push_back(service);
            }
        }
        //TODO check is array is empty
        flServiceList["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
        flServiceList["FLServiceList"] = serviceArray;
        return flServiceList;
}

nlohmann::ordered_json FLServiceInfo::toJson(std::string& category) const
{
    if(flServices_ == nullptr)
            throw omnetpp::cRuntimeError("FLServiceInfo::toJson - flServices_ is null!");

        nlohmann::ordered_json serviceArray;
        nlohmann::ordered_json val ;
        nlohmann::ordered_json flServiceList;

        auto it = flServices_->begin();
        for(; it != flServices_->end(); ++it)
        {
            if(it->second.getFLCategory().compare(category) == 0)
            {
                nlohmann::ordered_json service;
                service["name"] = it->second.getFLServiceName();
                service["id"] = it->second.getFLServiceId();
                service["description"] = it->second.getFLServiceDescription();
                service["category"] = it->second.getFLCategory();
                service["FLTrainingMode"] = it->second.getFLTrainingModeStr();
                serviceArray.push_back(service);
            }
        }
        //TODO check is array is empty
        flServiceList["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
        flServiceList["FLServiceList"] = serviceArray;
        return flServiceList;
}

//nlohmann::ordered_json FLServiceInfo::toJson(std::string& mode) const
//{
//    if(mode.compare("SYNCHRONUS") == 0)
//    {
//        return toJson(SYNCHRONOUS);
//    }
//    else if(mode.compare("ASYNCHRONUS") == 0)
//    {
//        return toJson(ASYNCHRONOUS);
//    }
//    else if(mode.compare("BOTH") == 0)
//    {
//        return toJson(BOTH);
//    }
//}





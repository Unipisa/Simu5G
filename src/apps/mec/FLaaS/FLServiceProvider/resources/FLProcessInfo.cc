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
#include "apps/mec/FLaaS/FLServiceProvider/resources/FLControllerInfo.h"


FLProcessInfo::FLProcessInfo()
{
    flprocesses_ = nullptr;
}

FLProcessInfo::FLProcessInfo(std::map<std::string, FLProcess>* flprocesses)
{
    flprocesses_ = flprocesses;
}


nlohmann::ordered_json FLProcessInfo::toJson() const
{
    if(flprocesses_ == nullptr)
        throw omnetpp::cRuntimeError("FLProcessInfo::toJson - flprocesses_ is null!");

    nlohmann::ordered_json serviceArray;
    nlohmann::ordered_json val ;
    nlohmann::ordered_json flProcessList;

    auto service = flprocesses_->begin();
    for(; service != flprocesses_->end(); ++service)
    {
        if(service->second.isFLProcessActive())
        {
            nlohmann::ordered_json serviceJson;
            serviceJson["name"] = service->second.getFLProcessName();
            serviceJson["processId"] = service->second.getFLProcessId();
            serviceJson["serviceId"] = service->second.getFLServiceId();
            serviceJson["category"] = service->second.getFLCategory();
            serviceJson["FLTrainingMode"] = service->second.getFLTrainingModeStr();
            serviceJson["currentAccuracy"] = service->second.getFLCurrentModelAccuracy();
            serviceArray.push_back(serviceJson);
        }
    }
    //TODO check is array is empty
    flProcessList["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
    flProcessList["flProcessList"] = serviceArray;
    return flProcessList;
}

nlohmann::ordered_json FLProcessInfo::toJson(std::set<std::string>& serviceIds) const
{
    if(flprocesses_ == nullptr)
        throw omnetpp::cRuntimeError("FLProcessInfo::toJson - flprocesses_ is null!");

    nlohmann::ordered_json serviceArray;
    nlohmann::ordered_json val ;
    nlohmann::ordered_json flProcessList;

    auto it = serviceIds.begin();
    for(; it != serviceIds.end(); ++it)
    {
        auto service = flprocesses_->find(*it);
        if(service != flprocesses_->end() && service->second.isFLProcessActive())
        {
            nlohmann::ordered_json serviceJson;
            serviceJson["name"] = service->second.getFLProcessName();
            serviceJson["processId"] = service->second.getFLProcessId();
            serviceJson["serviceId"] = service->second.getFLServiceId();
            serviceJson["category"] = service->second.getFLCategory();
            serviceJson["FLTrainingMode"] = service->second.getFLTrainingModeStr();
            serviceJson["currentAccuracy"] = service->second.getFLCurrentModelAccuracy();
            serviceArray.push_back(serviceJson);
        }
    }

    //TODO check is array is empty
    flProcessList["timestamp"] = omnetpp::simTime().dbl();
    flProcessList["flProcessList"] = serviceArray;
    return flProcessList;
}

nlohmann::ordered_json FLProcessInfo::toJson(FLTrainingMode mode) const
{
    if(flprocesses_ == nullptr)
            throw omnetpp::cRuntimeError("FLProcessInfo::toJson - flprocesses_ is null!");

        nlohmann::ordered_json serviceArray;
        nlohmann::ordered_json val ;
        nlohmann::ordered_json flProcessList;

        auto it = flprocesses_->begin();
        for(; it != flprocesses_->end(); ++it)
        {
            if(it->second.getFLTrainingMode() == mode && it->second.isFLProcessActive())
            {
                nlohmann::ordered_json service;
                service["name"] = it->second.getFLProcessName();
                service["processId"] = it->second.getFLProcessId();
                service["serviceId"] = it->second.getFLServiceId();
                service["category"] = it->second.getFLCategory();
                service["FLTrainingMode"] = it->second.getFLTrainingModeStr();
                serviceArray.push_back(service);
            }
        }
        //TODO check is array is empty
        flProcessList["timestamp"] = omnetpp::simTime().dbl();
        flProcessList["flProcessList"] = serviceArray;
        return flProcessList;
}

nlohmann::ordered_json FLProcessInfo::toJson(const std::string& category) const
{
    if(flprocesses_ == nullptr)
            throw omnetpp::cRuntimeError("FLServiceInfo::toJson - flprocesses_ is null!");

        nlohmann::ordered_json serviceArray;
        nlohmann::ordered_json val ;
        nlohmann::ordered_json flProcessList;

        auto it = flprocesses_->begin();
        for(; it != flprocesses_->end(); ++it)
        {
            if(it->second.getFLCategory().compare(category) == 0 && it->second.isFLProcessActive())
            {
                nlohmann::ordered_json service;
                service["name"] = it->second.getFLProcessName();
                service["processId"] = it->second.getFLProcessId();
                service["serviceId"] = it->second.getFLServiceId();
                service["category"] = it->second.getFLCategory();
                service["FLTrainingMode"] = it->second.getFLTrainingModeStr();
                serviceArray.push_back(service);
            }
        }
        //TODO check is array is empty
        flProcessList["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
        flProcessList["flProcessList"] = serviceArray;
        return flProcessList;
}


nlohmann::ordered_json FLProcessInfo::toJsonFLProcess(const std::string& flProcessId, bool controllerEndpoint) const
{
    if(flprocesses_ == nullptr)
            throw omnetpp::cRuntimeError("FLServiceInfo::toJsonFlProcess - flprocesses_ is null!");

    nlohmann::ordered_json process;
    auto it = flprocesses_->begin();
    for(; it != flprocesses_->end(); ++it)
    {
        if(it->second.getFLProcessId().compare(flProcessId) == 0 && it->second.isFLProcessActive())
        {
            process["name"] = it->second.getFLProcessName();
            process["processId"] = it->second.getFLProcessId();
            process["serviceId"] = it->second.getFLServiceId();
            process["category"] = it->second.getFLCategory();
            process["FLTrainingMode"] = it->second.getFLTrainingModeStr();
            if(controllerEndpoint)
            {
               FLControllerInfo flController(&(it->second));
               process["FLControllerEndpoint"] = flController.toJson();
            }
            //TODO check is array is empty
            process["timestamp"] = omnetpp::simTime().dbl(); //TODO define nice timestamp
            break;
        }
    }
    return process;
}



/*
 * ApplicationDescriptor.cc
 *
 *  Created on: May 6, 2021
 *      Author: linofex
 */



#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"

#include  <iostream>

using namespace omnetpp;
ApplicationDescriptor::ApplicationDescriptor(const char* fileName)
{
    int len = strlen(fileName);
    char buf[len+strlen(".json")+strlen("ApplicationDescriptors/")+1];
    strcpy(buf,"ApplicationDescriptors/");
    strcat(buf,fileName);
    strcat(buf,".json");
    // read a JSON file
    std::ifstream rawFile(buf);

    nlohmann::json jsonFile;
    rawFile >> jsonFile;

    appDId_ = jsonFile["appDid"];
    appName_ = jsonFile["appName"];
    appProvider_ = jsonFile["appProvider"];
    appInfoName_ = jsonFile["appInfoName"];
    appDescription_ = jsonFile["appDescription"];
    virtualResourceDescritor_.cpu  = jsonFile["virtualComputeDescriptor"]["virtualCpu"];
    virtualResourceDescritor_.disk = jsonFile["virtualComputeDescriptor"]["virtualDisk"];
    virtualResourceDescritor_.ram  = jsonFile["virtualComputeDescriptor"]["virtualMemory"];

    if(jsonFile.contains("appServiceRequired"))
    {
        if(jsonFile["appServiceRequired"].is_array())
        {
            nlohmann::json serviceVector = jsonFile["appServiceRequired"];
            for(int i = 0; i < serviceVector.size(); ++i)
            {
                std::string serviceName = serviceVector.at(i);
                appServicesRequired_.push_back(serviceName);
            }
        }
        else
        {
            appServicesRequired_.push_back((std::string)jsonFile["appServiceRequired"]);
        }
    }

    if(jsonFile.contains("appServiceProvided"))
    {
        if(jsonFile["appServiceProvided"].is_array())
        {
            nlohmann::json serviceVector = jsonFile["appServiceProvided"];
            for(int i = 0; i < serviceVector.size(); ++i)
            {
                appServicesProduced_.push_back((std::string)serviceVector.at(i));
            }
        }
        else
        {
            appServicesProduced_.push_back((std::string)jsonFile["appServiceProvided"]);
        }
    }

    this->printApplicationDescriptor();

}
ApplicationDescriptor::ApplicationDescriptor(const std::string& appDid,const std::string& appName,const std::string& appProvider,const std::string& appInfoName, const std::string& appDescription,
                              const ResourceDescriptor& resources, std::vector<std::string> appServicesRequired, std::vector<std::string> appServicesProduced)
{
    appDId_ = appDid;
    appName_ = appName;
    appProvider_ = appProvider;
    appInfoName_ = appInfoName;
    appDescription_ = appDescription;
    virtualResourceDescritor_ = resources;
    appServicesRequired_ = appServicesRequired;
    appServicesProduced_ = appServicesProduced;
}


nlohmann::ordered_json ApplicationDescriptor::toAppInfo() const
{
    nlohmann::ordered_json appInfo;
    appInfo["appDId"]  = appDId_;
    appInfo["appName"] = appName_;
    appInfo["appProvider"] = appProvider_;
    appInfo["appDescrition"] = appDescription_;

    appInfo["appCharcs"]["memory"] = virtualResourceDescritor_.ram;
    appInfo["appCharcs"]["storage"] = virtualResourceDescritor_.disk;
    appInfo["appCharcs"]["cpu"] = virtualResourceDescritor_.cpu;


    return appInfo;
}

void ApplicationDescriptor::printApplicationDescriptor() const
{
    EV <<   "appDId: " << appDId_ << "\n" <<
            "appName: " << appName_ << "\n" <<
            "appProvider: " << appProvider_ << "\n" <<
            "appInfoName: " << appInfoName_ << "\n" <<
            "appDescription: " << appDescription_ << "\n" <<
            "virtualResourceDescritor: \n\t\t" <<
            "memory: " << virtualResourceDescritor_.ram << "\n\t\t" <<
            "cpu: " << virtualResourceDescritor_.cpu << "\n\t\t" <<
            "disk: " << virtualResourceDescritor_.disk << "\n";
}

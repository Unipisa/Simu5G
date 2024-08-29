//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"
#include <math.h>       // pow
#include  <iostream>

namespace simu5g {

using namespace omnetpp;
ApplicationDescriptor::ApplicationDescriptor(const char *fileName)
{
    // read a JSON file
    std::ifstream rawFile(fileName);
    if (!rawFile.good())
        throw cRuntimeError("ApplicationDescriptor file: %s does not exist", fileName);

    nlohmann::json jsonFile;
    rawFile >> jsonFile;

    appDId_ = jsonFile["appDid"];
    appName_ = jsonFile["appName"];
    appProvider_ = jsonFile["appProvider"];
    appInfoName_ = jsonFile["appInfoName"];
    appDescription_ = jsonFile["appDescription"];
    virtualResourceDescritor_.cpu = jsonFile["virtualComputeDescriptor"]["virtualCpu"];
    virtualResourceDescritor_.disk = pow(10, 6) * double(jsonFile["virtualComputeDescriptor"]["virtualDisk"]);   // this quantity is in MB
    virtualResourceDescritor_.ram = pow(10, 6) * double(jsonFile["virtualComputeDescriptor"]["virtualMemory"]); // this quantity is in MB

    if (jsonFile.contains("appServiceRequired")) {
        if (jsonFile["appServiceRequired"].is_array()) {
            nlohmann::json serviceVector = jsonFile["appServiceRequired"];
            for (auto serviceDep : serviceVector) {
                appServicesRequired_.push_back((std::string)serviceDep["ServiceDependency"]["serName"]);
            }
        }
        else {
            nlohmann::json serviceDep = jsonFile["appServiceRequired"];
            appServicesRequired_.push_back((std::string)serviceDep["ServiceDependency"]["serName"]);
        }
    }

    if (jsonFile.contains("appServiceProvided")) {
        if (jsonFile["appServiceProvided"].is_array()) {
            nlohmann::json serviceVector = jsonFile["appServiceProvided"];
            for (const auto & i : serviceVector) {
                appServicesProduced_.push_back((std::string)i);
            }
        }
        else {
            appServicesProduced_.push_back((std::string)jsonFile["appServiceProvided"]);
        }
    }

    /*
     * There could exist MEC services implemented as simple OMNeT++ modules, that communicate with the MEC application
     * through OMNeT-like messages. The application descriptor JSON file has a section to take them into account.
     * The list can still be found in the service registry
     */

    if (jsonFile.contains("omnetppServiceRequired")) {
        omnetppServiceRequired_ = jsonFile["omnetppServiceRequired"];
    }

    /*
     * If the application descriptor refers to a MEC application running outside the simulator, i.e. emulation mode,
     * the fields address and port refer to the endpoint to communicate with the MEC application
     */
    if (jsonFile.contains("emulatedMecApplication")) {
        isEmulated = true;
        externalAddress = jsonFile["emulatedMecApplication"]["ipAddress"];
        externalPort = jsonFile["emulatedMecApplication"]["port"];
    }
    else {
        isEmulated = false;
        externalAddress.clear();
        externalPort = 0;
    }

    this->printApplicationDescriptor();
}

ApplicationDescriptor::ApplicationDescriptor(const std::string& appDid, const std::string& appName, const std::string& appProvider, const std::string& appInfoName, const std::string& appDescription,
        const ResourceDescriptor& resources, std::vector<std::string> appServicesRequired, std::vector<std::string> appServicesProduced) : appDId_(appDid), appName_(appName), appProvider_(appProvider), appInfoName_(appInfoName), appDescription_(appDescription), virtualResourceDescritor_(resources), appServicesRequired_(appServicesRequired), appServicesProduced_(appServicesProduced)
{
}

nlohmann::ordered_json ApplicationDescriptor::toAppInfo() const
{
    nlohmann::ordered_json appInfo;
    appInfo["appDId"] = appDId_;
    appInfo["appName"] = appName_;
    appInfo["appProvider"] = appProvider_;
    appInfo["appDescription"] = appDescription_;

    appInfo["appCharcs"]["memory"] = virtualResourceDescritor_.ram;
    appInfo["appCharcs"]["storage"] = virtualResourceDescritor_.disk;
    appInfo["appCharcs"]["cpu"] = virtualResourceDescritor_.cpu;

    return appInfo;
}

void ApplicationDescriptor::printApplicationDescriptor() const
{
    EV << "appDId: " << appDId_ << "\n" <<
        "appName: " << appName_ << "\n" <<
        "appProvider: " << appProvider_ << "\n" <<
        "appInfoName: " << appInfoName_ << "\n" <<
        "appDescription: " << appDescription_ << "\n" <<
        "virtualResourceDescritor: \n\t\t" <<
        "memory: " << virtualResourceDescritor_.ram << "\n\t\t" <<
        "cpu: " << virtualResourceDescritor_.cpu << "\n\t\t" <<
        "disk: " << virtualResourceDescritor_.disk << "\n";
}

} //namespace


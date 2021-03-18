//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "ServiceRegistry.h"

Define_Module(ServiceRegistry);

using namespace omnetpp;

ServiceRegistry::ServiceRegistry() {
    MeServicesMap_.clear();
}

ServiceRegistry::~ServiceRegistry() {
    // TODO Auto-generated destructor stub
}

void ServiceRegistry::initialize(int stage)
{
    EV << "ServiceRegistry::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    binder_ = getBinder();

    mePlatform = getParentModule();
    if(mePlatform != NULL){
        meHost = mePlatform->getParentModule();
    }
    else{
        EV << "ServiceRegistry::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("ServiceRegistry::initialize - \tFATAL! Error when getting getParentModule()");
    }

}

void ServiceRegistry::handleMessage(omnetpp::cMessage *msg)
{
}

void ServiceRegistry::finish()
{
}


void ServiceRegistry::registerMeService(const std::string& MeServiceName,const SockAddr& sockAddr)
{
    if(MeServicesMap_.find(MeServiceName) != MeServicesMap_.end())
    {
        throw cRuntimeError("ServiceRegistry::registerMeService - %s is already present!", MeServiceName.c_str());
    }
    EV << "ServiceRegistry::registerMeService - "<< MeServiceName << " [" << sockAddr.str()<<"] added!" << endl;
    MeServicesMap_[MeServiceName] = sockAddr;
}

SockAddr ServiceRegistry::retrieveMeService(const std::string& MeServiceName)
{
    if(MeServicesMap_.find(MeServiceName) == MeServicesMap_.end())
    {
       EV << "ServiceRegistry::retrieveMeService - "<< MeServiceName << " is not present!" << endl;
       SockAddr sockAddr = {inet::L3Address(), 0};
       return sockAddr;
    }
    else
    {
        SockAddr sockAddr = MeServicesMap_[MeServiceName];
        EV << "ServiceRegistry::retrieveMeService - "<< MeServiceName << " [" << sockAddr.str()<<"] added!" << endl;
        return sockAddr;
    }

}

















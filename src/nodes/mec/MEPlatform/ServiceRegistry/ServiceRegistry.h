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

#ifndef NODES_MEC_MEPLATFORM_SERVICEREGISTRY_H_
#define NODES_MEC_MEPLATFORM_SERVICEREGISTRY_H_

#include <omnetpp.h>
#include "inet/networklayer/common/L3Address.h"
#include "nodes/binder/LteBinder.h"
#include <map>

struct SockAddr
{
    inet::L3Address addr;
    int port;

    std::string str() const { return addr.str() + ":" + std::to_string(port);}
};

class ServiceRegistry: public omnetpp::cSimpleModule
{
public:
    ServiceRegistry();
    virtual ~ServiceRegistry();

    void registerMeService(const std::string& MeServiceName,const SockAddr& sockAddr);
    SockAddr retrieveMeService(const std::string& MeServiceName);

private:
    std::map<const std::string, SockAddr> MeServicesMap_;

    //LteBinder (oracle module)
    LteBinder* binder_;

    //parent modules
    omnetpp::cModule* mePlatform;
    omnetpp::cModule* meHost;


protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    virtual void handleMessage(omnetpp::cMessage *msg);
    virtual void finish();


};


#endif /* NODES_MEC_MEPLATFORM_SERVICEREGISTRY_H_ */

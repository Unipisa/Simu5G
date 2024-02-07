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

#ifndef __SIMU5G_GLOBALDATA_H_
#define __SIMU5G_GLOBALDATA_H_

#include <omnetpp.h>
#include <inet/common/packet/Packet.h>
using namespace omnetpp;
struct InterfaceTable{
   InterfaceTable(){};
   std::string interfaceId;
   const char *macAddress;
   //const char *ipAddress;
   std::string ipAddress;
   std::string name;
   std::string connectedUeIpAddress;

};

struct QosConfiguration{
    std::string trafficType;
    std::string pcp;
    std::string fiveQi;
};
class GlobalData : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    std::map<std::string, InterfaceTable> ueEthInterfaceMapping;
    std::map<std::string, QosConfiguration> qosMappingTable;
    public:
   void readIpXmlConfig();
   std::map<std::string, InterfaceTable> getUeEthInterfaceMapping(){
       return ueEthInterfaceMapping;
   }
   void readQosXmlConfiguration();
   std::map<std::string, QosConfiguration> getQosMappingTable(){
       return this->qosMappingTable;
   }
   int convertPcpToFiveqi(inet::Packet *datagram);
   int convertFiveqiToPcp(inet::Packet *datagram);

};

#endif

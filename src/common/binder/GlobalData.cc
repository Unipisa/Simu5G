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

#include "GlobalData.h"

Define_Module(GlobalData);
using namespace omnetpp;
using namespace inet;
void GlobalData::initialize()
{
    readIpXmlConfig();
    readQosXmlConfiguration();
}

void GlobalData::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
void GlobalData::readIpXmlConfig(){
    cXMLElement* root = par("config");
    cXMLElementList interfaceElements = root->getChildrenByTagName("interface");
    for (auto& element: interfaceElements){
        InterfaceTable iface;
        iface.name = element->getAttribute("name");
        iface.ipAddress = element->getAttribute("ipAddressoftheDestinationNode");
        iface.macAddress = element->getAttribute("macAddressOfTheDestinationNode");
         iface.interfaceId = element->getAttribute("interfaceIdOfTheUe");
         if (element->getAttribute("connectedUeIpAddress")){
             iface.connectedUeIpAddress = element->getAttribute("connectedUeIpAddress");
         }
         else{
             iface.connectedUeIpAddress = "";
         }

         this->ueEthInterfaceMapping[iface.ipAddress]=iface;

    }
}
void GlobalData::readQosXmlConfiguration(){
    cXMLElement *root = par("qosMappingConfig");
    cXMLElementList trafficElements = root->getChildrenByTagName("TrafficType");
        for (auto& element: trafficElements){
            QosConfiguration qosConfiguration;
            qosConfiguration.trafficType = element->getAttribute("name");
            qosConfiguration.pcp = element->getAttribute("pcp");
            qosConfiguration.fiveQi = element->getAttribute("fiveQi");

             this->qosMappingTable[qosConfiguration.trafficType]=qosConfiguration;

        }
}
int GlobalData::convertPcpToFiveqi(Packet *datagram){
    std::string packetName = datagram->getName();
    size_t found = -1;
    for (auto& element: this->qosMappingTable){
       found = packetName.find(element.second.trafficType);
       if (found != std::string::npos){
           return std::stoi(element.second.fiveQi);
       }
    }
    return -1;
}
int GlobalData::convertFiveqiToPcp(Packet *datagram){
    std::string packetName = datagram->getName();
        size_t found = -1;
        for (auto& element: this->qosMappingTable){
           found = packetName.find(element.second.trafficType);
           if (found != std::string::npos){
               return std::stoi(element.second.pcp);
           }
        }
        return -1;
}

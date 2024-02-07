/*
 * qosHandlers.cc
 *
 *  Created on: Mar 21, 2023
 *      Author: devika
 */

#include <iostream>
#include "stack/sdap/QosChecker.h"


using namespace std;

using namespace omnetpp;
using namespace inet;
QosChecker::QosChecker(){
    // get reference to the binder
     binder_ = getBinder();
}

TrafficFlowTemplateId QosChecker::qosCheckerUpf(const inet::Ipv4Address &destAddr){
    std::vector<inet::Ipv4Address> UeEthDevice = binder_->getUeConnectedEthernetDevices();
    int flag = 0;
    for (int i=0;i<UeEthDevice.size();++i){
            if (destAddr.str() == UeEthDevice[i].str()){
                EV<<"UeEthernetDeviceFound!!!!!"<<endl;
                TrafficFlowTemplateId tftId = 1;
                flag = 0;
                return tftId;
            }
        }
    if (flag == 0)
            return -1;
}
TrafficFlowTemplateId QosChecker::qosCheckerGnb(const inet::Ipv4Address &destAddr){
    std::vector<inet::Ipv4Address> UeEthDevice = binder_->getUeConnectedEthernetDevices();
            for (int i=0;i<UeEthDevice.size();++i){
                if (destAddr.str() == UeEthDevice[i].str()){
                    EV<<"UeEthernetDeviceFound!!!!!"<<endl;
                    TrafficFlowTemplateId tftId = 1;
                    return tftId;
                }
            }
}
TrafficFlowTemplateId QosChecker::qosCheckerUe(const inet::Ipv4Address &destAddr){

}

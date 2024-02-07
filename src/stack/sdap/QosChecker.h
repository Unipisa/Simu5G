/*
 * qosHandlers.h
 *
 *  Created on: Mar 21, 2023
 *      Author: devika
 */

#ifndef STACK_SDAP_QOSHANDLERS_H_
#define STACK_SDAP_QOSHANDLERS_H_
#include <omnetpp.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "common/binder/Binder.h"
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "common/binder/GlobalData.h"
#include <map>

using namespace omnetpp;

struct QoSInfo{
    void QosInfo(Direction dir){
        this->dir = dir;
    };
    LteTrafficClass trafficClass;
    int pcp = 0;
    int qfi = 0;
    Direction dir;
};
const signed int dcGbr[9] = {82, 83, 84, 85, 86, 87, 88, 89, 90};
const signed int gbr[13] = {1, 2, 3, 4, 65, 66, 67, 75, 71, 72, 73, 74, 76};
const signed int nonGbr[10] = {5, 6, 7, 8, 9, 10, 69, 70, 79, 80};
struct FiveQI{
    FiveQI(){};
 //   std::string fiveQIvalue;
  //  std::string resource_type;
    signed int priority;
    signed int pdb;
    signed int per;
    signed int daw;
};

class QosChecker{
    // reference to the LTE Binder module
        Binder* binder_;
public:
    //std::map<std::string, std::array<int,5>> pcpToQfi;
    std::map<std::string, int> pcpToQfi = {
            {"best effort", 6},
            {"audio", 1},
//            {"video", 2},
            {"video", 74},
            {"configuration", 3},
            {"diagnostics", 3},
            {"events", 3},
            {"control", 82},
            {"cyclic", 83},
            {"isochronous", 82},
            {"network control", 82}
    };
    std::map<std::string, int> qfiToPcp = {
            {"best effort", 0},
            {"audio", 1},
            {"video", 1},
            {"configuration", 2},
            {"diagnostics", 3},
            {"events", 4},
            {"control", 5},
            {"cyclic", 6},
            {"isochronous", 7},
            {"network control", 8}
        };
    QosChecker();
    TrafficFlowTemplateId qosCheckerUpf(const inet::Ipv4Address &destAddr);
    TrafficFlowTemplateId qosCheckerGnb(const inet::Ipv4Address &destAddr);
    TrafficFlowTemplateId qosCheckerUe(const inet::Ipv4Address &destAddr);

};


#endif /* STACK_SDAP_QOSHANDLERS_H_ */

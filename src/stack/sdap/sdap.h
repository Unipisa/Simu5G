/*
 * sdap.h
 *
 *  Created on: Apr 12, 2023
 *      Author: devika
 */

#ifndef STACK_SDAP_SDAP_H_
#define STACK_SDAP_SDAP_H_

#include <omnetpp.h>
#include "common/LteControlInfo.h"
#include "common/binder/Binder.h"

using namespace omnetpp;

class Sdap{
public:
     void setTrafficFlowInformation(cPacket *pkt, FlowControlInfo *lteInfo, Binder *binder_);

};



#endif /* STACK_SDAP_SDAP_H_ */

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

#ifndef __TRAFFICFLOWFILTER_H_
#define __TRAFFICFLOWFILTER_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

/**
 * The objective of the Traffic Flow Filter is to map IP 4-Tuples to TFT identifiers. This commonly means identifying a bearer and
 * associating it with an ID that will be recognized by the first GTP-U entity.
 *
 * This simplified traffic filter queries the Binder to find the destination of the packet.
 * It resides at both the eNodeB and the PGW. At the PGW, when a packet comes to the traffic flow filter, the latter finds the
 * destination endpoint (the eNodeB serving the destination UE) of the GTP tunnel that needs to be established. At the eNodeB,
 * the destination endpoint is always the PGW. However, if the fastForwarding flag is enabled and the destination of the packet
 * is within the same cell, the packet is just relayed to the Radio interface.
 */
class TrafficFlowFilter : public cSimpleModule
{
    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    // the filterTable_ will be indexed differently depending on this parameter
    CoreNodeType ownerType_;

    // reference to the LTE Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // if this flag is set, each packet received from the radio network, having the same radio network as destination
    // must be re-sent down without going through the Internet
    bool fastForwarding_;

    // store the name of the gateway node (for MEC Hosts and base stations only)
    std::string gateway_;

    CoreNodeType selectOwnerType(const char *type);

    // === MEC support === //

    // only if owner type is ENB or GNB
    std::string meHost;
    inet::L3Address meHostAddress;
    // only if owner type is GTPENDPOINT
    inet::L3Address eNodeBAddress;

    //@author Alessandro Noferi
    //
    // for emulation when the MEC host is directly connected to the BS
    inet::L3Address meAppsExtAddress_;
    int meAppsExtAddressMask_;

  protected:
    int numInitStages() const override { return inet::INITSTAGE_LAST + 1; }
    void initialize(int stage) override;

    // The TrafficFlowFilter module may receive messages only from the input interface of its compound module
    void handleMessage(cMessage *msg) override;

    // functions for managing filter tables
    TrafficFlowTemplateId findTrafficFlow(inet::L3Address srcAddress, inet::L3Address destAddress);
};

} //namespace

#endif


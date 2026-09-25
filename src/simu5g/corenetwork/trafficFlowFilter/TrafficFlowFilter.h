//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __TRAFFICFLOWFILTER_H_
#define __TRAFFICFLOWFILTER_H_

#include <memory>
#include <vector>

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/PacketFilter.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/corenetwork/trafficFlowFilter/TftControlInfo_m.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class BearerConfigurator;

/**
 * The objective of the Traffic Flow Filter is to map IP 4-Tuples to TFT identifiers. This commonly means identifying a bearer and
 * associating it with an ID that will be recognized by the first GTP-U entity.
 *
 * This simplified traffic filter queries the Binder to find the destination of the packet.
 * It resides at both the eNodeB and the PGW. At the PGW (and at a UPF or a MEC host's UPF), it finds the destination UE,
 * whose PDU session's downlink tunnel the GTP-U endpoint then sends the packet on. At the eNodeB, the destination endpoint
 * is always the PGW. However, if the fastForwarding flag is enabled and the destination of the packet is within the same
 * cell, the packet is just relayed to the Radio interface.
 */
class TrafficFlowFilter : public cSimpleModule
{
  protected:
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

    // The QFI-assignment rules of this tunnel-entry filter, delivered by the bearer
    // configurator (see its dlQfiRules parameter): the first rule whose filter
    // matches the packet supplies its QFI -- a fixed value, or the packet's DSCP
    // field read as the QFI. Stays empty at a base-station filter, which classifies
    // by the built-in residual instead (see handleMessage()).
    QfiRuleSet qfiRules_;

    // where the rules come from; resolved -- and this filter registered there --
    // at core-network tunnel entries only, see initialize()
    inet::ModuleRefByPar<BearerConfigurator> bearerConfigurator_;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;

    // The TrafficFlowFilter module may receive messages only from the input interface of its compound module
    void handleMessage(cMessage *msg) override;

    CoreNodeType selectOwnerType(const char *type);

    // Where a datagram goes; for TFT_PDU_SESSION, ueNodeId is set to the destination UE
    TftOutcome findTrafficFlow(const inet::L3Address& srcAddress, const inet::L3Address& destAddress, MacNodeId& ueNodeId);

  public:
    // Take delivery of this filter's compiled QFI-assignment rules from the
    // bearer configurator
    virtual void setQfiRules(QfiRuleSet&& rules);
};

} //namespace

#endif

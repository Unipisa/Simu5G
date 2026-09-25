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

#ifndef __GTP_USER_H_
#define __GTP_USER_H_

#include <map>

#include "simu5g/common/LteDefs.h"
#include <inet/common/ModuleAccess.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/gtp/GtpUserMsg_m.h"

namespace simu5g {

using namespace omnetpp;

class BearerConfigurator;

/**
 * GtpUser is used for building data tunnels between GTP peers.
 * GtpUser can receive two kinds of packets:
 * a) IP datagram from a traffic filter. These packets are labeled with a tftId
 * b) GtpUserMsg from Udp-IP layers.
 *
 */
class GtpUser : public cSimpleModule
{
    inet::UdpSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

    // the core network gateway (the "gateway" parameter) of a base station connected to
    // the core network or of a MEC host's UPF, and its IP address; empty otherwise
    std::string gateway_;
    inet::L3Address gwAddress_;

    // the SMF stand-in, which this tunnel endpoint registers with
    inet::ModuleRefByPar<BearerConfigurator> bearerConfigurator_;

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    CoreNodeType ownerType_;

    // if this module is on BS, this variable includes the ID of the BS
    MacNodeId myMacNodeID;

    opp_component_ptr<cModule> networkNode_;

    CoreNodeType selectOwnerType(const char *type);

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // receive an IP Datagram from the traffic filter, encapsulates it in a GTP-U packet then forwards it to the proper next hop
    void handleFromTrafficFlowFilter(inet::Packet *datagram);

    // receive a GTP-U packet from Udp, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(inet::Packet *gtpMsg);

    // receive a reply of the Neighbor Discovery responder (UPF/PGW only), and tunnel it to the UE's base station
    void handleFromNdResponder(inet::Packet *datagram);

    // encapsulate a datagram into GTP-U, and send it through the tunnel to the given base station
    void tunnelToBaseStation(inet::Packet *datagram, MacNodeId bsId, Qfi qfi);
};

} //namespace

#endif

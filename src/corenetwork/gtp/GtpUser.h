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

#ifndef __GTP_USER_H_
#define __GTP_USER_H_

#include <map>

#include <omnetpp.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "common/binder/Binder.h"
#include "corenetwork/gtp/GtpUserMsg_m.h"

namespace simu5g {

using namespace omnetpp;

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

    // IP address of the gateway to the Internet
    inet::L3Address gwAddress_;

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    CoreNodeType ownerType_;

    CoreNodeType selectOwnerType(const char *type);

    // if this module is on BS, this variable includes the ID of the BS
    MacNodeId myMacNodeID;

    opp_component_ptr<inet::NetworkInterface> ie_;

    opp_component_ptr<cModule> networkNode_;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // receive an IP Datagram from the traffic filter, encapsulates it in a GTP-U packet then forwards it to the proper next hop
    void handleFromTrafficFlowFilter(inet::Packet *datagram);

    // receive a GTP-U packet from Udp, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(inet::Packet *gtpMsg);

    // detect outgoing interface name (CellularNic)
    inet::NetworkInterface *detectInterface();
};

} //namespace

#endif


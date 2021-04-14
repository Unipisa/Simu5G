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
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "corenetwork/gtp/GtpUserMsg_m.h"
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/InterfaceEntry.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "common/binder/Binder.h"
#include <inet/linklayer/common/InterfaceTag_m.h>

/**
 * GtpUser is used for building data tunnels between GTP peers.
 * GtpUser can receive two kind of packets:
 * a) IP datagram from a trafficFilter. Those packets are labeled with a tftId
 * b) GtpUserMsg from Udp-IP layers.
 *
 */
class GtpUser : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    Binder* binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

    // IP address of the gateway to the Internet
    inet::L3Address gwAddress_;

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    CoreNodeType ownerType_;

    CoreNodeType selectOwnerType(const char * type);


    //mec
    //only if owner type is ENB
    MacNodeId myMacNodeID;
    // to intercept traffic for ME Host
    std::string meHostVirtualisationInfrastructure;
    inet::L3Address meHostVirtualisationInfrastructureAddress;
    // to tunnel traffic for ME Host
    std::string meHostGtpEndpoint;
    inet::L3Address meHostGtpEndpointAddress;
    //end mec

    inet::InterfaceEntry *ie_;

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    // receive and IP Datagram from the traffic filter, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromTrafficFlowFilter(inet::Packet * datagram);

    // receive a GTP-U packet from Udp, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(inet::Packet * gtpMsg);

    // detect outgoing interface name (LteNic)
    inet::InterfaceEntry *detectInterface();
};

#endif

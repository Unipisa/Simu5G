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
#include "simu5g/corenetwork/gtp/GtpTunnel.h"
#include "simu5g/corenetwork/gtp/GtpUserMsg_m.h"

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

    // the core network gateway (the "gateway" parameter) of a base station connected to
    // the core network or of a MEC host's UPF, and its IP address; empty otherwise
    std::string gateway_;
    inet::L3Address gwAddress_;

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    CoreNodeType ownerType_;

    // if this module is on BS, this variable includes the ID of the BS
    MacNodeId myMacNodeID;

    opp_component_ptr<cModule> networkNode_;

    // The tunnels ending here: the session of each TEID allocated at this endpoint.
    // A released session's tunnels stay known, so that a G-PDU still in flight can be
    // attributed; TEIDs are not reused.
    std::map<Teid, TunnelSession> rxTunnels_;

    // At a base station: the uplink tunnels of the sessions of the UEs it serves or
    // has served, with the session, by each of the UE's node ids
    struct UplinkSession {
        TunnelSession session;
        UplinkTunnels tunnels;
    };
    std::map<MacNodeId, UplinkSession> ulTunnels_;

    // At a UPF/PGW or a MEC host's UPF: the downlink tunnel of each session it serves,
    // by each of the UE's node ids; none while the UE is attached nowhere
    std::map<MacNodeId, FTeid> dlTunnels_;

    // At an anchor UPF: the N6 tunnel of each Unstructured session it anchors, by each
    // of the UE's node ids (see UserPlaneNodeControl)
    std::map<MacNodeId, N6Tunnel> n6Tunnels_;

    // At an anchor UPF: the Identification field of the IPv4 datagrams it originates
    // on the N6 tunnels of Unstructured sessions
    uint16_t n6DatagramId_ = 0;

    CoreNodeType selectOwnerType(const char *type);

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // receive an IP Datagram from the traffic filter, encapsulates it in a GTP-U packet then forwards it to the proper next hop
    void handleFromTrafficFlowFilter(inet::Packet *datagram);

    // receive a GTP-U packet from Udp, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(inet::Packet *gtpMsg);

    // at a UPF/PGW: send a packet to the node's message dispatcher on the relay's lower side,
    // which delivers it to the module that provides the given service
    void sendToDispatcher(inet::Packet *packet, const inet::Protocol& service);

    // encapsulate a datagram into GTP-U, and send it through the given downlink tunnel of
    // a session
    void tunnelDownlink(inet::Packet *datagram, const FTeid& tunnel, Qfi qfi);

    // At an anchor UPF: send the uplink payload of an Unstructured session to the data
    // network over the session's N6 tunnel, in a UDP/IP datagram this node originates
    // (TS 29.561 9.2)
    void tunnelUplinkOverN6(inet::Packet *payload, const SessionRef& session);

    // At a base station: an End Marker arrived on the tunnel with the given TEID
    void handleEndMarker(Teid teid);

    // At a UPF/PGW or a MEC host's UPF: the downlink tunnel of the session of the UE
    // with the given node id; nullptr if its session is not served here, or the UE is
    // attached nowhere
    const FTeid *findDownlinkTunnel(MacNodeId ueNodeId);

    // At a base station: the uplink tunnels of the session of the UE with the given node id
    const UplinkTunnels& getUplinkTunnels(MacNodeId ueNodeId);

    // At a base station: the session of a UE whose uplink enters the core network
    // here, by one of the UE's node ids; throws if it has none
    const TunnelSession& getServedSession(MacNodeId ueNodeId);

    // The session of a tunnel ending here; throws for an unknown TEID
    const TunnelSession& findTunnel(Teid teid);


  public:
    // The tunnels of the sessions, as the bearer configurator (the SMF stand-in)
    // sets them up and moves them.

    // A tunnel ending at this endpoint: G-PDUs arriving with the TEID belong to the
    // session, which is of the given type
    virtual void addTunnel(Teid teid, const SessionRef& session, SessionType type);

    // At a base station: the uplink tunnels of the session, which is of the given type
    virtual void setUplinkTunnels(const SessionRef& session, SessionType type, const UplinkTunnels& tunnels);

    // At a UPF/PGW or a MEC host's UPF: the session's downlink tunnel, which the path
    // switch moves; an unset F-TEID while the UE is attached nowhere
    virtual void setDownlinkTunnel(const SessionRef& session, const FTeid& tunnel);

    // At an anchor UPF/PGW: end a downlink tunnel with an End Marker, as the path switch
    // moves the session's downlink off it (the "send end marker" instruction of the SMF)
    virtual void sendEndMarker(const FTeid& tunnel);

    // At an anchor UPF: the N6 tunnel of an Unstructured session, which its uplink
    // leaves the node on
    virtual void setN6Tunnel(const SessionRef& session, const N6Tunnel& tunnel);

    // The session is released: forget its uplink, downlink and N6 tunnels
    virtual void removeSession(const SessionRef& session);
};

} //namespace

#endif

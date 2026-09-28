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

#ifndef __GTP_USER_X2_H_
#define __GTP_USER_X2_H_

#include <map>
#include <tuple>

#include <inet/common/ModuleRefByPar.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/gtp/GtpTunnel.h"
#include "simu5g/corenetwork/gtp/GtpUserMsg_m.h"
#include "simu5g/x2/packet/LteX2Message.h"

namespace simu5g {

using namespace omnetpp;


/**
 * GtpUserX2 is used for building data tunnels between GTP peers over X2, for handover procedures.
 * GtpUserX2 can receive two kinds of packets:
 * a) LteX2Message from the X2 Manager. These packets encapsulate an IP datagram
 * b) GtpUserX2Msg from UDP-IP layers.
 *
 * Downlink data a handover source forwards travels on the downlink tunnel of the
 * datagram's PDU session at the target: the TEID the target allocated for the session
 * (the one the core network's downlink arrives with, see ~GtpUser), which the base
 * stations of the session are told about (see ConnectionControlEnb). A PDCP PDU of a dual
 * connectivity bearer travels on the bearer's X2-U tunnel for its direction, whose TEID
 * the receiving end allocated; the receiver names the bearer to ~DcMux in an
 * X2DcTunnelInd tag.
 */
class GtpUserX2 : public cSimpleModule
{
    inet::UdpSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

    // The tunnels ending here: the PDU session of each TEID this base station allocated.
    // A released session's tunnels stay known, like at ~GtpUser.
    std::map<Teid, SessionRef> rxTunnels_;

    // The TEID of each PDU session's downlink tunnel at the other base stations it has
    // one at, by each of the UE's node ids, then by base station
    std::map<MacNodeId, std::map<MacNodeId, Teid>> forwardingTeids_;

    // The dual connectivity bearer of each X2-U tunnel ending here (rx), and the TEID of
    // each bearer's tunnel at the peer node for the direction this node sends (tx),
    // by each of the UE's node ids, the DRB and the direction
    struct DcTunnel {
        MacNodeId ueNodeId = NODEID_NONE;   // the UE, by the id this node keys the bearer by
        DrbId drbId;
        Direction direction = DL;
    };
    std::map<Teid, DcTunnel> dcRxTunnels_;
    std::map<std::tuple<MacNodeId, DrbId, Direction>, Teid> dcTxTeids_;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(inet::cMessage *msg) override;

    // receive an X2 Message from the X2 Manager, encapsulates it in a GTP-U packet then forwards it to the proper next hop
    void handleFromStack(inet::Packet *x2Msg);

    // receive a GTP-U packet from UDP, detunnel it and send it to the X2 Manager
    void handleFromUdp(inet::Packet *gtpMsg);

    // The TEID to forward a datagram of the given PDU session to the given base station with
    virtual Teid getForwardingTeid(const SessionTag *session, MacNodeId targetBs);

  public:
    // The tunnels of the PDU sessions, as the base station's control plane
    // (ConnectionControlEnb) sets them up here

    // A tunnel ending at this base station: G-PDUs arriving with the TEID belong to the session
    virtual void addTunnel(Teid teid, const SessionRef& session);

    // The TEID of the session's downlink tunnel at another base station
    virtual void setForwardingTeid(const SessionRef& session, MacNodeId bsId, Teid teid);

    // The session is released: forget the TEIDs to forward it with
    virtual void removeSession(const SessionRef& session);

    // A dual connectivity bearer's X2-U tunnel ending at this node, for the given
    // direction: G-PDUs arriving with the TEID carry PDCP PDUs of the bearer the UE
    // with the given id (this node's key for the bearer) has with the given DRB id
    virtual void addDcTunnel(Teid teid, MacNodeId ueNodeId, DrbId drbId, Direction direction);

    // The TEID of a dual connectivity bearer's X2-U tunnel at the peer node, for the
    // direction this node sends; the UE is named by both of its node ids
    virtual void setDcTunnelTeid(MacNodeId ueLteId, MacNodeId ueNrId, DrbId drbId, Direction direction, Teid teid);
};

} //namespace

#endif


//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _CONNECTIONCONTROLENB_H_
#define _CONNECTIONCONTROLENB_H_

#include <string>

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/gtp/GtpTunnel.h"

namespace simu5g {

class GtpUser;
class GtpUserX2;

/**
 * The control-plane entry point of a base station: the one module of the node the
 * control plane of other nodes talks to. It owns the node's TEID space, and programs
 * the node's GtpUser and GtpUserX2 as the calls say. See ConnectionControlEnb.ned.
 */
class ConnectionControlEnb : public omnetpp::cSimpleModule
{
  protected:
    MacNodeId nodeId_ = NODEID_NONE;

    // The node's core network gateway; empty at a node not connected to a core
    // network (a secondary node), which takes no session
    std::string gateway_;

    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<GtpUser> gtpUser_;
    inet::ModuleRefByPar<GtpUserX2> gtpUserX2_;

    // The node's TEID space: the TEID allocated last (see allocateTeid())
    Teid lastTeid_ = TEID_NONE;

    // The transport address of the node's tunnel endpoints, resolved on first use: the
    // node's addresses are assigned after INITSTAGE_LOCAL (see getAddress())
    inet::L3Address address_;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override { throw omnetpp::cRuntimeError("This module does not process messages"); }

    // Hand out the next TEID of the node's TEID space. TEIDs are allocated in
    // increasing order and never reused within a run, so a G-PDU still in flight on a
    // released tunnel cannot be taken for a later session's.
    virtual Teid allocateTeid();

    // The transport address of the node's tunnel endpoints: that of the network node
    virtual const inet::L3Address& getAddress();

  public:
    MacNodeId getNodeId() const { return nodeId_; }
    const std::string& getGateway() const { return gateway_; }

    // A PDU session's tunnels at this base station: its downlink tunnel, under a TEID
    // allocated here, which also receives the downlink a handover source forwards
    // over X2-U; and its uplink tunnels into the core network, to send the UE's
    // uplink on. Returns the downlink tunnel's F-TEID.
    virtual FTeid sessionTunnelSetup(const SessionRef& session, const UplinkTunnels& uplink);

    // The TEID of the session's downlink tunnel at another base station, to forward
    // the session's downlink to it with over X2-U during a handover
    virtual void setForwardingTeid(const SessionRef& session, MacNodeId bsId, Teid teid);

    // The session is released: its tunnels at this base station are forgotten
    virtual void sessionRelease(const SessionRef& session);

    // A dual connectivity bearer's X2-U tunnel ending at this node, for the given
    // direction, under a TEID allocated here; returns the TEID. The bearer is that of
    // the UE with the given id (this node's key for the bearer) with the given DRB id.
    virtual Teid addDcTunnel(MacNodeId ueNodeId, DrbId drbId, Direction direction);

    // The TEID of a dual connectivity bearer's X2-U tunnel at the peer node, for the
    // direction this node sends; the UE is named by both of its node ids
    virtual void setDcTunnelTeid(MacNodeId ueLteId, MacNodeId ueNrId, DrbId drbId, Direction direction, Teid teid);
};

} //namespace

#endif

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

#ifndef __IP2NIC_H_
#define __IP2NIC_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <set>
#include <unordered_map>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/SessionTag_m.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"

namespace simu5g {


using namespace omnetpp;

class HandoverX2Forwarder;


/**
 *
 */
class Ip2Nic : public cSimpleModule
{
  protected:
    RanNodeType nodeType_;      // UE or NODEB

    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // the node's control-plane entry point, which establishes bearers (a base station)
    // or has the serving base station establish them (a UE)
    inet::ModuleRefByPar<ConnectionControlBase> connectionControl_;

    // LTE MAC node id of this node
    MacNodeId nodeId_ = NODEID_NONE;
    // NR MAC node id of this node (if enabled)
    MacNodeId nrNodeId_ = NODEID_NONE;

    // Enable for dual connectivity
    bool dualConnectivityEnabled_;

    // Dual connectivity only: the anchor cell group's identity at this node -- the id every
    // packet carries until the bearer's splitter picks a leg. At a base station it is the
    // UE-id space of the node's own technology; at a UE it is the id of the stack that
    // faces the master. Computed once at initialization: the master/serving relations it
    // reads are the configured ones, and no handover can be in flight during init.
    bool anchorNr_ = false;
    MacNodeId anchorId_ = NODEID_NONE;   // UEs only: this UE's id on the anchor stack

    // Flag mirroring PDCP's (to be verified with ASSERTs, then used to replace PDCP dependency)
    bool isNr_ = false;
    bool hasSdap_ = false;
    bool establishBearersOnDemand_ = true;

    // Whether a UE is attached with its LTE stack, its NR stack, both (dual connectivity
    // only), or neither -- a packet whose UE is attached with neither is dropped.
    // UE only: this UE itself, as RRC pushed its attachment
    virtual void getOwnStackAvailability(bool& hasLte, bool& hasNr);
    // Base station only: the UE of the given PDU session, which a downlink packet travels to
    virtual void getUeStackAvailability(const SessionTag *session, bool& hasLte, bool& hasNr);

    // UE only: the id this UE's outgoing flows carry as their source -- the anchor
    // stack's id under dual connectivity (which leg carries a PDU is the bearer
    // splitter's per-PDU choice), otherwise the id of the stack the UE is attached with.
    virtual MacNodeId ueSourceNodeId();

    // Fills in an outgoing packet's FlowControlInfo: the flow's endpoints, its direction
    // and, for D2D, its peers. Identity only -- which bearer carries the flow is the
    // separate question assignBearer() answers, because answering it can establish one.
    // Core handles the plain UL/DL path (LTE) and the NR (non-D2D) path; the D2D-aware
    // overrides live in Ip2NicD2D.
    virtual void attachFlowControlInfo(inet::Packet *pkt, const inet::L3Address& srcAddr, const inet::L3Address& destAddr, uint16_t typeOfService);

    // Records in the packet's FlowControlInfo which DRB carries its flow, establishing a
    // bearer for it if none does yet -- so calling this can create entities at both
    // endpoints (see establishBearerOnDemand()). Not called when SDAP is present: it maps
    // the QoS flow onto a DRB itself.
    virtual void assignBearer(inet::Packet *pkt, const inet::L3Address& srcAddr, const inet::L3Address& destAddr, uint16_t typeOfService);

    // Fills in the flow's endpoint ids: this node on the near side, and on the far side
    // the next hop towards the destination (the multicast group's sender, for multicast).
    // At a base station the destination is the UE of the packet's SessionTag.
    virtual void assignEndpointIds(inet::Packet *pkt, FlowControlInfo *lteInfo, const inet::L3Address& destAddr, bool isEnb);

    // Establishes a bearer for a flow that has none, and returns the DRB id it got. This
    // is the data plane asking RRC for a bearer, so it is the packet path's one
    // control-plane action -- it builds entities at BOTH endpoints. The request carries
    // identity only (the flow and its binding key); the bearer's properties are authored
    // by the node's control plane (see ConnectionControlEnb::establishBearer()). Throws instead when the
    // establishBearersOnDemand parameter turned this fallback off.
    virtual DrbId establishBearerOnDemand(const FlowBindingKey& key, FlowControlInfo *lteInfo, inet::Packet *pkt);

    // Which bearer carries which flow: the classifier's half of a bearer, and the only
    // bearer state this module holds. Entirely maintained by RRC, which installs an
    // entry at each endpoint of a bearer it establishes and drops it again when it
    // tears that bearer down -- so a flow is bound exactly while a bearer carries it,
    // and an unbound flow is one that needs a bearer established.
    std::unordered_map<FlowBindingKey, DrbKey, FlowBindingKeyHash> flowBindings_;

    // UEs whose context this node released after a radio link failure (RLF).
    // While a peer is listed here, its DL (gNB) / UL (UE) packets are dropped at
    // Ip2Nic -- modeling RRC UE Context Release: the bearer is gone, so incoming
    // data is discarded rather than pushed at a torn-down entity. RRC (BearerManagement)
    // owns the release/re-establishment lifecycle and toggles this set via releaseUe()/resumeUe().
    std::set<MacNodeId> releasedUes_;

    cGate *stackGateOut_ = nullptr;       // gate connecting Ip2Nic module to cellular stack
    cGate *ipGateOut_ = nullptr;          // gate connecting Ip2Nic module to network layer

    // UE only: mirror of RRC's stack attachment ledger, pushed on every handover event
    // (see setServingNodeIds() and getStackAvailability()); NODEID_NONE = not attached
    MacNodeId lteServingNodeId_ = NODEID_NONE;  // the LTE stack's serving node
    MacNodeId nrServingNodeId_ = NODEID_NONE;   // the NR stack's serving node

    // corresponding entry for our interface
    opp_component_ptr<inet::NetworkInterface> networkIf;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    // Tags a received datagram for the dispatchers above the NIC: to be handled by
    // the given protocol, as having arrived on this interface
    virtual void prepareForIp(inet::Packet *datagram, const inet::Protocol *protocol);
    virtual void toIpUe(inet::Packet *datagram);
    virtual void toIpBs(inet::Packet *datagram);
    virtual void toStackBs(inet::Packet *datagram);
    virtual void toStackUe(inet::Packet *datagram);

    /// classifies the connection of an outgoing packet (called after the
    /// common preamble of attachFlowControlInfo(), before the endpoint ids are
    /// assigned). No-op in the base; D2D-aware subclasses set the multicast
    /// group, peer IDs and the actual flow direction here.
    virtual void classifyConnection(inet::Packet *pkt, FlowControlInfo *lteInfo, const inet::L3Address& destAddr, MacNodeId localNodeId, bool isEnb) {}

    /// direction stored in the flow key. The plain-LTE stack has historically used
    /// a direction-agnostic key; the NR and D2D stacks key by the actual flow
    /// direction.
    virtual Direction bindingDirection(FlowControlInfo *lteInfo) { return isNr_ ? (Direction)lteInfo->getDirection() : Direction(0xFFFF); }
    // UE only: the node an uplink packet from the given source stack goes to
    virtual MacNodeId getNextHopNodeId(const inet::L3Address& destAddr, MacNodeId sourceId);
    // Base station only: the node a downlink packet for the UE of the given PDU session
    // goes to -- the UE itself when this node serves it directly, else the node the
    // packet is relayed through (the UE's serving node, or this node's master)
    virtual MacNodeId getDownlinkNextHopNodeId(const SessionTag *session);

  public:
    // Configuration push: RRC binds a flow to the bearer carrying it, at both endpoints
    // of the bearer it establishes. First binding wins, so a flow already bound keeps
    // its bearer -- which is what makes a dual-connectivity UE keep the master-anchored
    // bearer its packets are routed to. Ip2Nic does not author bindings; together with
    // releaseFlowBindings() this is the only write path.
    virtual void configureFlowBinding(const FlowBindingKey& key, DrbKey bearer);

    // Unbind every flow the given bearer carried: RRC calls this where it tears the
    // bearer down, so the bindings never outlive the entities behind them and the
    // affected flows establish a fresh bearer on their next packet.
    virtual void releaseFlowBindings(DrbKey bearer);

    // Radio link failure handling is data-plane only here: BearerManagement (RRC) drives
    // the release/re-establishment lifecycle and gates this node's packet dropping via
    // the two calls below.

    // Start dropping a peer's future DL/UL packets (UE Context Release). Its bearer
    // re-establishes on demand once resumeUe() is called.
    virtual void releaseUe(MacNodeId ueId);

    // Stop dropping the peer's packets (RRC re-establishment complete); traffic resumes.
    virtual void resumeUe(MacNodeId ueId);

    // RRC's push of the stacks' attachment (see BearerManagement::pushServingNodeIds()):
    // the serving node of this UE's LTE and NR stack, current as of handover start. UE only.
    virtual void setServingNodeIds(MacNodeId servingNodeId, MacNodeId nrServingNodeId);
};

} //namespace

#endif

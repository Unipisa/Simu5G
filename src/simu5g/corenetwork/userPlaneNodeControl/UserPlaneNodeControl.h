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

#ifndef _USERPLANENODECONTROL_H_
#define _USERPLANENODECONTROL_H_

#include <string>

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/gtp/GtpTunnel.h"

namespace simu5g {

class GtpUser;
class TrafficFlowFilter;

/**
 * The control-plane entry point of a user plane node (a UPF, a PGW, or a MEC host's
 * UPF): the one module of the node the core network's control plane talks to. It
 * owns the node's TEID space, and programs the node's GtpUser and TrafficFlowFilter
 * as the CoreControl's calls say. See UserPlaneNodeControl.ned.
 */
class UserPlaneNodeControl : public omnetpp::cSimpleModule
{
  protected:
    CoreNodeType nodeType_ = UPF;
    std::string gateway_;

    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<GtpUser> gtpUser_;
    inet::ModuleRefByPar<TrafficFlowFilter> trafficFlowFilter_;

    // The node's TEID space: the TEID allocated last (see allocateTeid())
    Teid lastTeid_ = TEID_NONE;

    // The transport address of the node's tunnel endpoint, resolved on first use: the
    // node's addresses are assigned after INITSTAGE_LOCAL (see getAddress())
    inet::L3Address address_;

    // The N6 end of the Unstructured sessions this node anchors, as configured (the
    // unstructured* parameters), checked and resolved at the first such session (see
    // getN6Configuration()): the prefix the sessions' N6 addresses are allocated
    // from, the server's address, and the ports
    bool n6Configured_ = false;
    inet::L3Address unstructuredPrefix_;
    int unstructuredPrefixLength_ = -1;
    inet::L3Address unstructuredServer_;
    int unstructuredServerPort_ = -1;
    int unstructuredPort_ = -1;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override { throw omnetpp::cRuntimeError("This module does not process messages"); }

    // Hand out the next TEID of the node's TEID space. TEIDs are allocated in
    // increasing order and never reused within a run, so a G-PDU still in flight on a
    // released tunnel cannot be taken for a later session's.
    virtual Teid allocateTeid();

    // The transport address of the node's tunnel endpoint: that of the network node
    virtual const inet::L3Address& getAddress();

    // Check and resolve the unstructured* parameters; throws if they do not describe
    // the N6 end of Unstructured sessions
    virtual void configureN6();

  public:
    CoreNodeType getNodeType() const { return nodeType_; }
    const std::string& getGateway() const { return gateway_; }

    // The node's downlink QFI classification rules, installed into the traffic flow
    // filter (5GC: the PDRs/QERs PFCP would carry)
    virtual void setDownlinkClassifierRules(QfiRuleSet&& rules);

    // The prefix the N6 addresses of the Unstructured sessions this node anchors are
    // allocated from (the unstructuredPrefix parameter), as (address, length); throws
    // if the node is not configured for Unstructured sessions
    virtual std::pair<inet::L3Address, int> getUnstructuredPrefix();

    // PFCP Session Establishment / GTP-C Create Session: the uplink tunnel of the
    // session, which is of the given type, at this node, under a TEID allocated here;
    // returns the tunnel's F-TEID, for the base stations to send the session's uplink
    // on. An Unstructured session also gets its N6 tunnel here, under the given N6
    // address (the anchor's only; unspecified for the other sessions and nodes).
    virtual FTeid establishUserPlaneSession(const SessionRef& session, SessionType type, const inet::L3Address& n6Address);

    // PFCP Session Modification / GTP-C Modify Bearer: the session's downlink now goes
    // into tunnel dl (unset: nowhere, the UE is attached nowhere). If oldDl is set, the
    // downlink on that path is ended with an End Marker first (TS 23.502 4.9.1.2.2).
    virtual void updateDownlinkTunnel(const SessionRef& session, const FTeid& dl, const FTeid& oldDl);

    // PFCP Session Deletion / GTP-C Delete Session: the session's tunnels at this node
    // are forgotten
    virtual void releaseUserPlaneSession(const SessionRef& session);
};

} //namespace

#endif

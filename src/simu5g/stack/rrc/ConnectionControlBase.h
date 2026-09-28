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

#ifndef _CONNECTIONCONTROLBASE_H_
#define _CONNECTIONCONTROLBASE_H_

#include <inet/common/packet/Packet.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/stack/rrc/DrbDesc.h"

namespace simu5g {

/**
 * What a node's control-plane entry point answers, at a base station
 * (ConnectionControlEnb) and at a UE (HandoverController, one per leg) alike: the
 * calls the node's own data path and RRC make on it, and the bearer installation
 * calls the control plane of another node makes on it. Not a 3GPP reference point:
 * the in-node contract of the entry point. A UE answers the requests by asking its
 * serving base station, which is where bearers are established.
 */
class ConnectionControlBase : public omnetpp::cSimpleModule
{
  public:
    // ---- requests from the node's own data path and RRC ----

    // The data path found no bearer for a flow it classified: establish one whose
    // properties come from the bearer definition the packet matches (Ip2Nic; see
    // ConnectionControlEnb::establishBearer()). Returns the bearer's DRB id.
    virtual DrbId establishBearer(const FlowId& flow, const FlowBindingKey& key, const inet::Packet *pkt) = 0;

    // Establish the bearer of a flow whose DRB the requester already knows (SDAP's
    // mapped-but-not-established case, and the static definitions). Returns the DRB id.
    virtual DrbId establishBearer(const FlowId& flow, const BearerRequest& req) = 0;

    // The DRB an unmapped QFI resolves to at the given UE, materializing an on-demand
    // definition's bearer on first use; DRBID_NONE if nothing covers it (SDAP)
    virtual DrbId resolveDrbForQfi(MacNodeId ueNodeId, Qfi qfi) = 0;

    // A bearer of this node was torn down: its DRB id returns to its pool where this
    // node owns the pool (BearerManagement)
    virtual void bearerReleased(DrbKey bearer) = 0;

    // The node joined a multicast group (Registration; legacy sidelink multicast)
    virtual void multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId) = 0;

    // ---- bearer installation, from the control plane of another node ----
    // The group a 3GPP RRCReconfiguration carries at once; each forwards to the
    // node's BearerManagement. Kept as several calls in this round.

    virtual void configureDrb(const DrbDesc& drb) = 0;
    virtual void createIncomingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) = 0;
    virtual void createOutgoingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) = 0;
    virtual void setUplinkQfiRules(QfiRuleSet&& rules) = 0;
};

} //namespace

#endif

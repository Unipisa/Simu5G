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

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"
#include "simu5g/corenetwork/coreControl/CoreControl.h"
#include "simu5g/corenetwork/gtp/GtpTunnel.h"
#include "simu5g/corenetwork/gtp/GtpUser.h"
#include "simu5g/corenetwork/gtp/GtpUserX2.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderEnb.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"

namespace simu5g {

class HandoverController;

/**
 * A UE leg's MeasurementReport (TS 38.331 5.5.5): what the leg measured of its
 * serving cell and of the best other cell, once the leg's A3-like event fired (the
 * best cell above the serving one by the hysteresis). The base station decides.
 */
struct MeasurementReport
{
    MacNodeId servingCell = NODEID_NONE;
    double servingRssi = 0;
    MacNodeId bestCell = NODEID_NONE;
    double bestRssi = 0;
};

/**
 * The HANDOVER REQUEST of Xn (TS 38.423 8.2.1), as far as the model needs it: the
 * context of the leg the source hands over, for the target to take on.
 */
struct HandoverRequest
{
    MacNodeId legId = NODEID_NONE;
    omnetpp::cModule *ueModule = nullptr;
    HandoverController *ueRrc = nullptr;       // the leg's control-plane entry point
    std::vector<SessionResource> sessions;     // the UE's PDU sessions at the source, with the source's downlink F-TEIDs
};

/**
 * The control-plane entry point of a base station: the one module of the node the
 * control plane of other nodes talks to. It owns the node's TEID space and programs
 * the node's GtpUser and GtpUserX2 as the calls say; and it establishes the bearers
 * of the UEs it serves, from the definitions the BearerConfigurator holds, installing
 * them at itself, at the UE, and at a dual connectivity secondary, through their
 * control-plane entry points. See ConnectionControlEnb.ned.
 */
class ConnectionControlEnb : public ConnectionControlBase
{
  protected:
    MacNodeId nodeId_ = NODEID_NONE;

    // The node's core network gateway; empty at a node not connected to a core
    // network (a secondary node), which takes no session
    std::string gateway_;

    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<BearerConfigurator> bearerConfigurator_;   // the bearer definitions, read only (its const API)
    inet::ModuleRefByPar<CoreControl> coreControl_;                 // the core network's control plane, the far end of N2
    inet::ModuleRefByPar<BearerManagement> bearerManagement_;       // the node's installer
    inet::ModuleRefByPar<LteMacEnb> mac_;                           // the node's MAC, for the AMC's user attachment and the per-UE queues
    inet::ModuleRefByPar<HandoverPacketHolderEnb> handoverPacketHolder_;   // the node's downlink holder/forwarder
    inet::ModuleRefByPar<GtpUser> gtpUser_;
    inet::ModuleRefByPar<GtpUserX2> gtpUserX2_;

    // What the control plane keeps of a UE leg this base station serves, or is the
    // handover target of: created by connectionSetupRequest() and handoverRequest(), dropped
    // by ueContextRelease(), connectionLost() and handoverCancel()
    struct UeContext {
        omnetpp::cModule *ueModule = nullptr;
        HandoverController *ueRrc = nullptr;   // the leg's control-plane entry point
        enum State { CONNECTED, HO_SOURCE_PREPARING, HO_SOURCE_EXECUTING, HO_TARGET_PREPARED } state = CONNECTED;
        MacNodeId hoPeer = NODEID_NONE;        // the other base station of the leg's handover in progress
        std::vector<SessionResource> sessions; // target role: the session resources the preparation set up for the leg, reported in the PATH SWITCH REQUEST
    };
    std::map<MacNodeId, UeContext> ues_;       // by the leg's node id

    // The PDU sessions with tunnels at this base station (see setUpSessionTunnels())
    std::vector<SessionResource> sessions_;

    // The node's TEID space: the TEID allocated last (see allocateTeid())
    Teid lastTeid_ = TEID_NONE;

    // The transport address of the node's tunnel endpoints, resolved on first use: the
    // node's addresses are assigned after INITSTAGE_LOCAL (see getAddress())
    inet::L3Address address_;

    // The DRB ids in use within each node pair this base station is a party to (see
    // assignDrbId()); a pair between two UEs, or a UE and a multicast group, is the
    // D2D subclass's (see foreignPairPool())
    std::map<std::pair<MacNodeId, MacNodeId>, std::set<DrbId>> drbIdsInUse_;

    // The on-demand definitions materialized within each node pair, and the id each
    // got: like every DRB id, an on-demand bearer's identity is pair-scoped, assigned
    // at the definition's first match within the pair and returned to the pair's pool
    // with the bearer (see forgetOnDemandDrbId()), so a UE that moves to another
    // serving node materializes the definition afresh there
    std::map<std::pair<const BearerConfigurator::AuthoredBearer *, std::pair<MacNodeId, MacNodeId>>, DrbId> onDemandIds_;

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

    // The control-plane entry point of another node, through the Binder's node directory
    virtual ConnectionControlBase *controlOf(MacNodeId nodeId);
    virtual ConnectionControlEnb *baseStationControl(MacNodeId bsId);
    virtual HandoverController *ueControl(MacNodeId legId);

    // The handover decision: the cell the leg is handed over to on its report, or
    // NODEID_NONE for none. The default policy takes the reported best cell -- the
    // decision the UE made itself before, whose hysteresis it still applies before
    // reporting -- so the instant and the target are unchanged.
    virtual MacNodeId selectHandoverTarget(MacNodeId legId, const MeasurementReport& report);

    // The other leg of the UE the leg belongs to, or NODEID_NONE
    virtual MacNodeId otherLegOf(MacNodeId legId);

    // The context of a leg this base station has one of; throws otherwise
    virtual UeContext& ueContext(MacNodeId legId);

    // This base station's resources of the session, or nullptr
    virtual SessionResource *findSession(const SessionRef& session);

    // This base station's resources of the PDU sessions of the UE the leg belongs to
    virtual std::vector<SessionResource> sessionsOf(MacNodeId legId);

    // Release this base station's state for a leg that left it: its MAC queues and RLC
    // entities here, its PDCP entities here and at this node's master if this node is a
    // secondary; and, if the UE's other leg is attached nowhere, whatever this node's
    // secondary holds for that leg, which the master's bearer establishment provisions
    // there regardless of the leg's attachment and nothing else would release (see
    // HandoverController::deleteOwnBuffers()). The AMC detach is the caller's.
    virtual void releaseLeg(MacNodeId legId);

    // The UE's uplink and downlink at this node's AMC
    virtual void attachAtAmc(MacNodeId legId);
    virtual void detachAtAmc(MacNodeId legId);

    // The body of sessionTunnelSetup(), for the calls that come from inside
    virtual FTeid setUpSessionTunnels(const SessionRef& session, const UplinkTunnels& uplink);

    // Whether the static bearers of the UE are established on this leg: the
    // technology-neutral LTE leg when the serving nodes form a DC setup (so that the
    // establishment splits the bearer into legs), else the NR leg when attached, else
    // the LTE leg -- the stack packet-triggered establishment would pick (see
    // Ip2Nic::assignBearer)
    virtual bool carriesStaticDrbs(omnetpp::cModule *ueModule, MacNodeId legId);

    // Install the static bearers of the UE from this leg: each definition's descriptor
    // delivered to the RRCs involved (pushDrbToRrcs()), then each bearer established
    // toward the leg's serving node, exactly like packet-triggered establishment, so
    // traffic finds the configured bearers in place
    virtual void establishStaticDrbs(omnetpp::cModule *ueModule, MacNodeId legId);

    // ---- DRB identities ----

    // Allocate the lowest free DRB ID within the (unordered) node pair {a, b}, so the
    // two endpoints of a link can never mint colliding IDs for the same peer.
    // For multicast flows, pass the multicast group ID as the second node.
    virtual DrbId assignDrbId(MacNodeId a, MacNodeId b);

    // Return a DRB ID to its pair's pool when the bearer is torn down. DRB identities are
    // a finite per-UE resource (TS 38.331: DRB-Identity is 1..32) and are reused once
    // released -- without this, a UE handing over repeatedly would exhaust the space.
    // Releasing an ID that is not in use is a no-op.
    virtual void releaseDrbId(MacNodeId a, MacNodeId b, DrbId drbId);

    // The pool of a node pair this base station is no party to: a pair between two
    // UEs, or a UE and a multicast group (legacy sidelink). None here; the D2D
    // subclass keeps them network-wide in the D2dBinder.
    virtual std::set<DrbId>& foreignPairPool(const std::pair<MacNodeId, MacNodeId>& pair);

    // Forget an on-demand definition's materialization in the given node pair when its
    // bearer is torn down: the id has returned to the pair's pool (releaseDrbId()), and
    // the next matching flow assigns afresh. Forgetting an id that is not recorded is a
    // no-op.
    virtual void forgetOnDemandDrbId(omnetpp::cModule *ueModule, MacNodeId a, MacNodeId b, DrbId drbId);

    // ---- bearer establishment ----

    // Establish the flow on the bearer a definition describes: a static entry's flow
    // joins the configured bearer under its pinned id; an on-demand entry is assigned
    // its pair-scoped id, and delivered to the RRCs, when it first matches within the
    // node pair.
    virtual DrbId establishFromDefinition(const BearerConfigurator::AuthoredBearer& ab, const FlowId& flow, const FlowBindingKey& key);

    // The DRB id a definition resolves to at the given UE, for the QFI path (see
    // resolveDrbForQfi()): a static definition's bearer already exists, so its pinned
    // id is returned as-is; an on-demand definition's bearer is materialized on first
    // use within the node pair -- the id assigned and the descriptor delivered to the
    // RRCs -- and that id returned. DRBID_NONE if the UE is not attached, so an
    // on-demand bearer has nowhere to be established.
    virtual DrbId drbOfDefinition(const BearerConfigurator::AuthoredBearer& ab, MacNodeId ueNodeId);

    // The definition a flow's bearer was authored from, or nullptr if none covers it
    virtual const DrbDesc *findBearerDefinition(const FlowId& flow);

    // The body of establishBearer(flow, req), for the calls that come from inside
    virtual DrbId establishDataConnection(const FlowId& flow, const BearerRequest& req);

    // Deliver one bearer's definition to the RRCs involved: the UE's (keyed by
    // NODEID_NONE, "my serving node") and, for each attached stack, the serving
    // node's (keyed by that stack's UE id), reserving the configured id per pair.
    virtual void pushDrbToRrcs(omnetpp::cModule *ueModule, const DrbDesc& drb);

    // A D2D or multicast flow's bearer: outside the definition system (definitions
    // describe infrastructure bearers), with a fixed transitional configuration.
    // None at a base station without D2D; the D2D subclass establishes it.
    virtual DrbId establishD2dBearer(const FlowId& flow, const FlowBindingKey& key);

    virtual bool isDualConnectivityRequired(const FlowId& flow);
    virtual void createConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp);
    // A multicast bearer's connections: TX at the sender, RX at the group members. None
    // at a base station without D2D; the D2D subclass creates them.
    virtual void createMulticastConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp);
    virtual void createIncomingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp);
    virtual void createOutgoingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp);

    // Set up the X2-U tunnels of a dual connectivity bearer, one per direction, each at
    // its receiving end: the secondary for the downlink the master relays, the master
    // for the uplink the secondary relays back (see GtpUserX2, DcMux)
    virtual void setUpX2DcTunnels(MacNodeId masterId, MacNodeId secondaryId, MacNodeId ueLteId, MacNodeId ueNrId,
            MacNodeId ueMcgId, MacNodeId ueScgId, DrbId drbId);

  public:
    MacNodeId getNodeId() const { return nodeId_; }
    const std::string& getGateway() const { return gateway_; }

    // ---- attach ----

    // RRCSetupRequest, and the registration that follows: a leg of a UE connects at
    // this base station (at initialization, the leg's configured serving cell). The
    // base station registers the UE with the core network, which sets up the
    // session's resources here (sessionResourceSetup()).
    virtual void connectionSetupRequest(omnetpp::cModule *ueModule, MacNodeId legId, ConnectionControlBase *ueRrc);

    // PDU SESSION RESOURCE SETUP REQUEST (N2), from the core network, for a leg that
    // registered here: the session's tunnels at this base station
    // (sessionTunnelSetup()), the UE's uplink QoS rules (the NAS container), and the
    // static data radio bearers of the leg that carries them, installed at this node
    // and at the UE. Returns the downlink tunnel's F-TEID.
    virtual FTeid sessionResourceSetup(MacNodeId legId, const SessionRef& session, const UplinkTunnels& uplink, QfiRuleSet&& ulQfiRules);

    // RRCRelease's counterpart the UE side sends, a simulation shortcut for the
    // network's own radio link monitoring: the leg lost the cell, detached, or is
    // being deleted. This base station releases its state for the leg and tells the
    // core network (UE CONTEXT RELEASE REQUEST).
    virtual void connectionLost(MacNodeId legId);

    // ---- handover ----

    // MeasurementReport, from a leg this base station serves: the base station
    // decides (selectHandoverTarget()) and, as the source, prepares the leg's handover
    // with the target over Xn (handoverRequest())
    virtual void measurementReport(MacNodeId legId, const MeasurementReport& report);

    // HANDOVER REQUEST (Xn), from the source of a leg's handover: this base station,
    // the target, takes the leg's context, sets the UE's session tunnels up at the
    // base station the downlink will enter the RAN at (this node's master under dual
    // connectivity), which also learns the source's downlink TEIDs, starts holding
    // the leg's downlink, and acknowledges (handoverRequestAck())
    virtual void handoverRequest(const HandoverRequest& request, ConnectionControlEnb *source);

    // HANDOVER REQUEST ACKNOWLEDGE (Xn), from the target, with the session resources
    // it set up: this base station, the source, commands the leg (the
    // RRCReconfiguration with sync, HandoverController::handoverCommand()), then
    // forwards the leg's downlink to the target's tunnels over X2-U (TS 38.300
    // 9.2.3.2.1)
    virtual void handoverRequestAck(MacNodeId legId, const std::vector<SessionResource>& admitted);

    // HANDOVER CANCEL (Xn), from the source of a leg's handover that lost the leg
    // meanwhile: this base station, the target, drops the leg's context (its
    // downlink holder keeps whatever it holds for the leg)
    virtual void handoverCancel(MacNodeId legId);

    // RRCReconfigurationComplete, from a leg that arrived here by handover: the target
    // takes the leg on at its AMC, has the core network switch the session's downlink
    // path here (PATH SWITCH REQUEST, with the tunnels the preparation set up),
    // releases the leg at the source (UE CONTEXT RELEASE over Xn), and sends down the
    // downlink held for the leg
    virtual void reconfigurationComplete(MacNodeId legId);

    // UE CONTEXT RELEASE (Xn), from the target of a leg's handover once the path is
    // switched: this base station, the source, stops forwarding and releases its
    // state for the leg
    virtual void ueContextRelease(MacNodeId legId);

    // PATH SWITCH REQUEST ACKNOWLEDGE, as far as the model needs it: the downlink of
    // the PDU session of the UE with the given node ids now enters the RAN here, and
    // entered it at fromBaseStation before (NODEID_NONE: nowhere), where the anchor
    // ends it with an End Marker if it is another base station. Told to the base
    // station the downlink enters at, which may be the master of the one that
    // requested the switch.
    virtual void downlinkPathSwitched(MacNodeId ueLteId, MacNodeId ueNrId, MacNodeId fromBaseStation);

    // A secondary's leg left it: the master releases the leg's PDCP entities, which
    // it anchors (SN release, as far as the model needs it)
    virtual void releasePdcpEntities(MacNodeId legId);

    // ---- the node's tunnels, for the core network's control plane and the other base stations ----

    // A PDU session's tunnels at this base station: its downlink tunnel, under a TEID
    // allocated here, which also receives the downlink a handover source forwards
    // over X2-U; and its uplink tunnels into the core network, to send the UE's
    // uplink on. Set up once per session, whichever side asks first; a later call
    // finds them in place. Returns the downlink tunnel's F-TEID.
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

    // ---- bearers ----

    // Establish a bearer for a flow the requester identifies but does not describe:
    // the requester supplies the flow, its classifier key and the triggering packet,
    // and the bearer's properties come from the "epc" definition whose packet filter
    // matches (staticDrbs first, then onDemandDrbs, in table order; the default entry
    // catches what no filter matched). A flow no definition covers throws: the
    // onDemandDrbs default value carries catch-all definitions, so only a
    // configuration that replaced them with a non-covering set can get here. D2D and
    // multicast bearers are outside the definition system (see establishD2dBearer()).
    // Returns the established bearer's DRB id.
    DrbId establishBearer(const FlowId& flow, const FlowBindingKey& key, const inet::Packet *pkt) override;

    // Establish a duplex data radio bearer for the flow: entities for BOTH directions
    // are created at both endpoints at once (DRBs are bidirectional per TS 38.331;
    // RLC-AM in particular needs the reverse path for its STATUS PDUs). Multicast
    // flows remain unidirectional (TX at the sender, RX at the group members).
    //
    // The bearer's DRB id is the flow's own when it has one, and a freshly assigned
    // one (see assignDrbId()) when flow.drbId is DRBID_NONE, i.e. when the requester
    // is establishing a bearer for a flow it has not seen before. Either way the id
    // of the established bearer is returned.
    DrbId establishBearer(const FlowId& flow, const BearerRequest& req) override;

    // Resolve the DRB an unmapped QFI should use at the given UE, when SDAP's QFI-to-DRB
    // table missed. The "5gc" definition that maps this QFI specifically wins; failing
    // that, the UE's default bearer catches it (it carries the QFIs no other bearer
    // maps). One walk in table order, static definitions before on-demand ones, so an
    // authored default outranks the onDemandDrbs catch-all -- the precedence
    // establishBearer() gives packet filters. Returns the DRB id (materializing an
    // on-demand definition's bearer on first use, so it also reaches SDAP's table via
    // the RRC push), or DRBID_NONE when nothing covers the QFI or the UE is not
    // attached. Repeated calls return the same bearer. This is SDAP's sole
    // bearer-selection authority: SDAP holds no default-DRB fallback of its own.
    DrbId resolveDrbForQfi(MacNodeId ueNodeId, Qfi qfi) override;

    // A bearer of this node was torn down: its DRB id returns to the pair's pool,
    // unless a static definition owns it for the whole run, and an on-demand
    // definition's materialization is forgotten
    void bearerReleased(DrbKey bearer) override;

    // A node has joined a multicast group (its RRC registration tells us through the
    // node's entry point). Nothing to do at a base station without D2D, where no
    // sidelink multicast bearer exists; see the D2D subclass.
    void multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId) override;

    // Deliver a static definition's bearer to the RRCs involved (see pushDrbToRrcs());
    // the base station serving the UE's first attached stack does it for all of them.
    // Called by the BearerConfigurator at initialization.
    virtual void installStaticDrb(omnetpp::cModule *ueModule, const DrbDesc& drb);

    // Mark an externally chosen DRB ID as in use within the pair {a, b}, so
    // assignDrbId() cannot hand out the same one later (SDAP and the static definitions
    // name their bearers themselves). The pair's serving base station does it for a
    // peer that installs a bearer at a UE the peer does not serve.
    virtual void reserveDrbId(MacNodeId a, MacNodeId b, DrbId drbId);

    // ---- bearer installation at this node, from a peer's control plane ----
    // The base station's half of a dual connectivity bearer's secondary cell group
    // (the SN Addition / Modification of TS 36.423 8.7.1, TS 38.423 8.3.1, as the
    // master runs it), and the RX leg of a multicast bearer at a member's serving
    // node; each forwards to the node's BearerManagement

    void configureDrb(const DrbDesc& drb) override;
    void createIncomingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) override;
    void createOutgoingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) override;
    void setUplinkQfiRules(QfiRuleSet&& rules) override;
};

} //namespace

#endif

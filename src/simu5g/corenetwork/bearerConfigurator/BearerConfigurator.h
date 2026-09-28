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

#ifndef _BEARERCONFIGURATOR_H_
#define _BEARERCONFIGURATOR_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/PacketFilter.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/QfiRuleSet.h"
#include "simu5g/stack/rrc/DrbDesc.h"

namespace simu5g {

using namespace omnetpp;

class ConnectionControlEnb;

/**
 * The network's central bearer configurator: a network-wide simulation service
 * combining bearer decisions that a real system distributes across the core
 * network and the RAN. It has one instance per cellular network, and it is what
 * decides which data radio bearers exist and when they are set up. See the NED
 * file for details.
 */
class BearerConfigurator : public cSimpleModule, public cListener
{
  public:
    ~BearerConfigurator() override;

  protected:
    inet::ModuleRefByPar<Binder> binder_;

    // QoS-derived RAN defaults for definitions that state no rlcMode/lcg themselves:
    // the amPerThreshold and lcgPriorityBounds parameters (see the NED documentation)
    double amPerThreshold_ = 0;
    std::vector<long> lcgPriorityBounds_;

    // Header compression policy for definitions that do not state "rohc": the profile
    // names whose bearers get ROHC (the rohcForDrbProfiles parameter)
    std::set<std::string> rohcForDrbProfiles_;

    // The DRB IDs currently in use within each node pair (see assignDrbId())
    std::map<std::pair<MacNodeId, MacNodeId>, std::set<DrbId>> drbIdsInUse_;

    // A bearer definition retained for establishment-time authoring: the UE it belongs
    // to, the descriptor delivered to the RRCs, and its compiled packet filters. One
    // record per (entry x matched UE); records keep table order, staticDrbs before
    // onDemandDrbs, which is the match order of establishOnDemandBearer(). An
    // onDemandDrbs record carries no DRB id of its own (desc.key stays DRBID_NONE):
    // like every DRB id, an on-demand bearer's identity is pair-scoped, assigned at the
    // definition's first match within each node pair (pairIds) and returned to the
    // pair's pool with the bearer (see forgetOnDemandDrbId()) -- so a UE that moves to
    // another serving node materializes the definition afresh there.
    struct AuthoredBearer {
        cModule *ueModule = nullptr;
        DrbDesc desc;                  // key = (NODEID_NONE, drbId); DRBID_NONE for onDemand
        bool onDemand = false;         // true = onDemandDrbs entry (ids assigned at first match, per pair)
        std::vector<std::unique_ptr<inet::PacketFilter>> filters;   // compiled desc.filters
        std::map<std::pair<MacNodeId, MacNodeId>, DrbId> pairIds;   // onDemand only: id per materialized node pair
    };
    std::vector<AuthoredBearer> authoredBearers_;

    /*
     * The flow that established each multicast bearer, kept so that a node joining the group
     * later can still be given its RX leg (see multicastGroupJoined()). createConnection()
     * provisions the members that exist when the sender starts, which is all of them only if
     * the node population is static; with nodes created during the simulation the joiner
     * would otherwise receive PDUs for a connection its stack knows nothing about.
     *
     * Keyed by (multicast group id, sender id): the RX side of a bearer is keyed by its
     * sender (see FlowId::rxDrbKey, and MacCid(senderId, lcid) in
     * BearerManagement::createIncomingConnection), so a group served by several senders --
     * at once, or one after another as vehicles come and go -- needs one remembered flow per
     * sender. Each entry is dropped when its sender leaves, see receiveSignal().
     */
    struct MulticastFlow {
        FlowId flow;
        BearerRequest req;
        bool withPdcp = false;
    };
    std::map<std::pair<MacNodeId, MacNodeId>, MulticastFlow> multicastFlows_;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override { throw cRuntimeError("This module does not process messages"); }

    // Configure the data radio bearers described by the staticDrbs parameter (see NED
    // documentation) by telling the RRC of each node involved in a bearer what to set
    // up: BearerManagement::configureDrb() at the UE and at its serving node. The
    // configuration is pushed from here, and no module ever reads it back out.
    virtual void configureDrbs();

    // Establish the bearers the staticDrbs entries describe, in the last
    // initialization stage: each retained static record goes through
    // establishDataConnection(), exactly like packet-triggered establishment, so
    // traffic finds the configured bearers in place. A dual-stack UE's bearer is
    // established on the stack packet-triggered establishment would pick; a UE
    // attached on no stack is a configuration error.
    virtual void establishStaticDrbs();

    // Establish the flow on the bearer a definition describes: a static entry's flow
    // joins the configured bearer under its pinned id; an on-demand entry is assigned
    // its pair-scoped id, and delivered to the RRCs, when it first matches within the
    // node pair.
    virtual DrbId establishFromDefinition(AuthoredBearer& ab, const FlowId& flow, const FlowBindingKey& key);

    // The DRB id a definition resolves to at the given UE, for the QFI path (see
    // resolveDrbForQfi()): a static definition's bearer already exists, so its pinned
    // id is returned as-is; an on-demand definition's bearer is materialized on first
    // use within the node pair -- the id assigned and the descriptor delivered to the
    // RRCs -- and that id returned. DRBID_NONE if the UE is not attached, so an
    // on-demand bearer has nowhere to be established.
    virtual DrbId drbOfDefinition(AuthoredBearer& ab, MacNodeId ueNodeId);

    // The definition a flow's bearer was authored from, or nullptr if none covers it
    virtual const DrbDesc *findBearerDefinition(const FlowId& flow);

    // The standardized QoS characteristics rows as built-in drbProfiles entries
    // ("qci-1".."qci-9", "5qi-1".."5qi-9"); built lazily, owned by this module
    omnetpp::cValueMap *predefinedDrbProfiles_ = nullptr;
    virtual const omnetpp::cValueMap *getPredefinedDrbProfiles();


    // Settle the SDAP header decision of one bearer record (TS 38.331 sdap-HeaderDL/UL,
    // stored in DrbDesc::useSdapHeader): a header is needed exactly when the bearer
    // alone cannot name the arriving QFI on the RX side -- it is the default bearer or
    // several QFIs map to it -- unless the configuration suppresses it, asserting that
    // a single QoS flow rides the bearer (which SDAP verifies per packet). Called once
    // the UE's default bearer is settled, since the decision hangs on isDefault.
    virtual void computeUseSdapHeader(DrbDesc& drb);

    // Deliver one bearer's definition to the RRCs involved: the UE's (keyed by
    // NODEID_NONE, "my serving node") and, for each attached stack, the serving
    // node's (keyed by that stack's UE id), reserving the configured id per pair.
    virtual void pushDrbToRrcs(cModule *ueModule, const DrbDesc& drb);

    // Parse one bearer-definition table (staticDrbs or onDemandDrbs) into
    // authoredBearers_, and, for staticDrbs, into drbsOfUe for the init-time pushes.
    virtual void parseDrbDefinitions(const char *paramName, bool onDemand,
            const std::map<cModule *, std::vector<MacNodeId>>& ueNodeIds, const std::string& networkPrefix,
            std::map<cModule *, std::map<DrbId, DrbDesc>>& drbsOfUe);

    // Validate the two QFI rule tables at initialization: every entry's field vocabulary
    // and grammar, and that every scoped entry matches a site -- a "node" pattern a
    // user plane node (an N4 endpoint registered with the Binder), a "ue" pattern a
    // registered UE with SDAP -- since a pattern that matches nothing is a typo the
    // per-site compilation (getDownlinkQfiRules() etc.) would never surface.
    virtual void validateQfiRules();

    // A module's path relative to the network, which the tables' patterns are matched against
    virtual std::string relativeToNetwork(const cModule *module) const;

    // The control-plane entry point of a base station, through the Binder's node directory
    virtual ConnectionControlEnb *baseStationControl(MacNodeId bsId);

    /**
     * Binder::nodeUnregisteredSignal_: forget the DRB identity pools of a node that has
     * left the simulation. drbIdsInUse_ is keyed by node pair, so a departed id survives
     * inside every pair it took part in and would keep those identities reserved.
     */
    void receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details) override;

    // Set up the X2-U tunnels of a dual connectivity bearer, one per direction, each at
    // its receiving end: the secondary for the downlink the master relays, the master
    // for the uplink the secondary relays back (see GtpUserX2, DcMux)
    virtual void setUpX2DcTunnels(MacNodeId masterId, MacNodeId secondaryId, MacNodeId ueLteId, MacNodeId ueNrId,
            MacNodeId ueMcgId, MacNodeId ueScgId, DrbId drbId);

    virtual bool isDualConnectivityRequired(const FlowId& flow);
    virtual void createConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp);
    virtual void createIncomingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp);
    virtual void createOutgoingConnectionOnNode(MacNodeId nodeId, const FlowId& flow, const BearerRequest& req, bool withPdcp);

  public:
    // Whether the UE's stack contains SDAP. Structure, not configuration: the sdap
    // submodule exists iff the NIC's hasSdap is set, the same resolvability test
    // BearerManagement::configureDrb() applies on its own side.
    static bool ueStackHasSdap(const cModule *ueModule);

    // The downlink QFI classification rules of a user plane node: the dlQfiRules
    // entries whose "node" pattern matches the node's path relative to the network (an
    // entry without one matches every node), in table order. Read by CoreControl, which
    // installs them into the node over N4; this module delivers nothing itself.
    virtual QfiRuleSet getDownlinkQfiRules(const cModule *node) const;

    // The uplink QFI classification rules of a UE: the ulQfiRules entries scoped to
    // it, likewise. Read by CoreControl, which delivers them to the UE's classifier as
    // the QoS rules of PDU session establishment.
    virtual QfiRuleSet getUplinkQfiRules(const cModule *ue) const;

    // A node has joined a multicast group (RRC registration tells us). If a sender has
    // already established that group's bearer, the node missed the RX-leg provisioning
    // createConnection() did over the membership as it stood then; give it one now, or
    // its MAC will receive PDUs for a connection it has no descriptor for and assert in
    // macPduUnmake(). Nodes that join before the bearer exists are covered by
    // createConnection() itself; createIncomingConnection() de-duplicates, so a node
    // reached by both paths is harmless.
    virtual void multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId);

    // Establish a duplex data radio bearer for the flow described by lteInfo:
    // entities for BOTH directions are created at both endpoints at once (DRBs are
    // bidirectional per TS 38.331; RLC-AM in particular needs the reverse path for
    // its STATUS PDUs). Multicast flows remain unidirectional (TX at the sender,
    // RX at the group members).
    //
    // The bearer's DRB id is the flow's own when it has one, and a freshly assigned
    // one (see assignDrbId()) when flow.drbId is DRBID_NONE, i.e. when the requester
    // is establishing a bearer for a flow it has not seen before. Either way the id
    // of the established bearer is returned.
    virtual DrbId establishDataConnection(const FlowId& flow, const BearerRequest& req);

    // Establish a bearer for a flow the requester identifies but does not describe:
    // Ip2Nic supplies the flow, its classifier key and the triggering packet, and the
    // bearer's properties come from the "epc" definition whose packet filter matches
    // (staticDrbs first, then onDemandDrbs, in table order; the default entry catches
    // what no filter matched). A flow no definition covers throws: the onDemandDrbs
    // default value carries catch-all definitions, so only a configuration that
    // replaced them with a non-covering set can get here. D2D and multicast bearers
    // are outside the definition system and get a fixed transitional configuration
    // (RLC UM, LCG 3). Returns the established bearer's DRB id.
    virtual DrbId establishOnDemandBearer(const FlowId& flow, const FlowBindingKey& key, const inet::Packet *pkt);

    // Resolve the DRB an unmapped QFI should use at the given UE, when SDAP's QFI-to-DRB
    // table missed. The "5gc" definition that maps this QFI specifically wins; failing
    // that, the UE's default bearer catches it (it carries the QFIs no other bearer
    // maps). One walk in table order, static definitions before on-demand ones, so an
    // authored default outranks the onDemandDrbs catch-all -- the precedence
    // establishOnDemandBearer() gives packet filters. Returns the DRB id (materializing
    // an on-demand definition's bearer on first use, so it also reaches SDAP's table via
    // the RRC push), or DRBID_NONE when nothing covers the QFI or the UE is not attached.
    // Called by SDAP on a lookup miss; repeated calls return the same bearer. This is
    // SDAP's sole bearer-selection authority: SDAP holds no default-DRB fallback of its
    // own.
    virtual DrbId resolveDrbForQfi(MacNodeId ueNodeId, Qfi qfi);

    // Allocate the lowest free DRB ID within the (unordered) node pair {a, b}, so the
    // two endpoints of a link can never mint colliding IDs for the same peer.
    // For multicast flows, pass the multicast group ID as the second node.
    virtual DrbId assignDrbId(MacNodeId a, MacNodeId b);

    // Mark an externally chosen DRB ID as in use, so assignDrbId() cannot hand out the
    // same one later (SDAP and the static definitions name their bearers themselves).
    virtual void reserveDrbId(MacNodeId a, MacNodeId b, DrbId drbId);

    // Return a DRB ID to its pair's pool when the bearer is torn down. DRB identities are
    // a finite per-UE resource (TS 38.331: DRB-Identity is 1..32) and are reused once
    // released -- without this, a UE handing over repeatedly would exhaust the space.
    // Releasing an ID that is not in use is a no-op, so both endpoints may call it.
    virtual void releaseDrbId(MacNodeId a, MacNodeId b, DrbId drbId);

    // True if a staticDrbs entry of the UE names this DRB id. A static definition owns
    // its id for the whole run: releasing it would let assignDrbId() hand the id to an
    // unrelated bearer while the definition still names it, so teardown must skip it.
    virtual bool ownsStaticDrbId(cModule *ueModule, DrbId drbId);

    // Forget an on-demand definition's materialization in the given node pair when its
    // bearer is torn down: the id has returned to the pair's pool (releaseDrbId()), and
    // the next matching flow assigns afresh. Forgetting an id that is not recorded is a
    // no-op, so both endpoints may call it.
    virtual void forgetOnDemandDrbId(cModule *ueModule, MacNodeId a, MacNodeId b, DrbId drbId);
};

} //namespace

#endif

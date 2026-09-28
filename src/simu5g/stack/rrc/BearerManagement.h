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

#ifndef _BEARER_MANAGEMENT_H_
#define _BEARER_MANAGEMENT_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/rrc/DrbTable.h"
#include <inet/common/ModuleRefByPar.h>
#include <set>
#include <utility>

using namespace omnetpp;

namespace simu5g {

class LteMacBase;
class RlcMux;
class RlcTxEntityBase;
class RlcRxEntityBase;
class RlcUmTxEntityBase;
class PdcpMux;
class DcMux;
class PdcpTxEntityBase;
class PdcpRxEntityBase;
class Registration;
class Binder;
class ConnectionControlBase;
class NrSdap;
class QosFlowClassifier;
class QfiRuleSet;
class Ip2Nic;
class HandoverPacketHolderUe;

/**
 * @brief RRC Bearer Management — creates and tears down PDCP, RLC and MAC
 *        entities for data radio bearers.
 */
class BearerManagement : public cSimpleModule
{
  protected:
    Registration *registration_ = nullptr;

    // PDCP entity types (resolved from NED params): the full PDCP entity is a compound
    // (LtePdcpEntity/NrPdcpEntity, TX+RX); at a DC secondary an PdcpRelayEntity stands in for it.
    cModuleType *pdcpEntityModuleType_ = nullptr;
    cModuleType *pdcpRelayEntityModuleType_ = nullptr;
    cModule *nicModule_ = nullptr;  // containing NIC module (parent of all submodules and entities)

    // RLC entity types (compound modules; resolved from NED params):
    // lteRlc* = LTE-FI bearers (TS 36.322), nrRlc* = NR bearers (SI/SO, TS 38.322)
    cModuleType *rlcTmEntityModuleType_ = nullptr;
    cModuleType *lteRlcUmEntityModuleType_ = nullptr;
    cModuleType *lteRlcAmEntityModuleType_ = nullptr;
    cModuleType *nrRlcUmEntityModuleType_ = nullptr;
    cModuleType *nrRlcAmEntityModuleType_ = nullptr;

    inet::ModuleRefByPar<RlcMux> rlcMuxModule;
    inet::ModuleRefByPar<RlcMux> nrRlcMuxModule;
    inet::ModuleRefByPar<LteMacBase> macModule;
    inet::ModuleRefByPar<LteMacBase> nrMacModule;
    inet::ModuleRefByPar<Binder> binderModule;   // DC master/secondary topology lookups; peer BearerManagement on RLF
    inet::ModuleRefByPar<ConnectionControlBase> connectionControlModule;      // the node's control-plane entry point, which a torn-down bearer's DRB identity goes back through
    inet::ModuleRefByPar<DrbTable> drbTableModule;   // the bearer configuration this module authors
    inet::ModuleRefByPar<NrSdap> sdapModule;     // configuration push target; null when the NIC has no SDAP
    inet::ModuleRefByPar<QosFlowClassifier> qosFlowClassifierModule;   // uplink QFI rule push target; null unless a UE stack with SDAP
    Ip2Nic *ip2nicModule_ = nullptr;             // configuration/notification push target

    // Tell the user-plane modules that a bearer came up or went away, so they never have
    // to inspect another layer to find out. Called exactly where this module mutates
    // PdcpMux's TX-entity registry, so their view of which bearers exist is identical
    // to that registry by construction.
    virtual void notifyBearerEstablished(DrbKey key);
    virtual void notifyBearerReleased(DrbKey key);

    // Give a torn-down bearer's DRB identity back to the pool it came from, through
    // the node's control-plane entry point (see ConnectionControlBase::bearerReleased()).
    virtual void releaseDrbIdOf(DrbKey bearer);

    // Entity registries (CP owns the lifecycle of all entities)
    // One PDCP entity module (compound: TX+RX, see PdcpEntityBase) per bearer, keyed by (peer
    // node, DRB id); the tx/rx submodule pointers below index into it for per-side lookups.
    std::map<DrbKey, cModule *> pdcpEntities_;
    std::map<DrbKey, PdcpTxEntityBase *> pdcpTxEntities_;
    std::map<DrbKey, PdcpRxEntityBase *> pdcpRxEntities_;
    // One PdcpRelayEntity compound per DC-secondary bearer (both directions), keyed by (UE, DRB id)
    std::map<DrbKey, cModule *> pdcpRelayEntities_;
    // One RLC entity module (compound: TX+RX sides, see RlcTm/Um/AmEntity) per
    // bearer, keyed by (peer node, DRB id); one map per leg (LTE / NR)
    std::map<DrbKey, cModule *> rlcEntities_;
    std::map<DrbKey, cModule *> nrRlcEntities_;

    // Radio Link Failure: teardown is deferred to a safe execution context via a
    // self-message, so we never delete entity modules from inside RLC/PDCP processing.
    cMessage *rlfTrigger_ = nullptr;
    std::set<std::pair<MacNodeId, bool>> pendingRlf_;  // (peer nodeId, nrStack)
    virtual void handleRadioLinkFailure(MacNodeId nodeId, bool nrStack);

    // RRC re-establishment (TS 38.331 5.3.7), modeled by its timers. On RLF the peer's link
    // is released (Ip2Nic drops its traffic); T311 (cell selection) then T301 (request ->
    // complete) run, after which the peer is un-released (Ip2Nic::resumeUe) and its bearer
    // re-establishes on demand. t311_ = 0 => release-to-IDLE (no reconnect attempt).
    simtime_t t311_;
    simtime_t t301_;
    std::map<cMessage *, MacNodeId> t311Timers_;   // pending cell-selection timers -> peer
    std::map<cMessage *, MacNodeId> t301Timers_;   // pending request->complete timers -> peer

    // NR dual connectivity on this NIC: infrastructure bearers get two legs (see
    // findOrCreatePdcpEntity)
    bool dualConnectivityEnabled_ = false;

    /*
     * The serving node of each of this UE's stacks, as RRC knows
     * it: updated the instant a handover, attachment or detachment BEGINS (see
     * HandoverController::triggerHandover()), which is ahead of the Binder -- that
     * learns only when the handover executes. Within that window this is the answer
     * that matches what the stack is actually doing; the Binder's stale one would make
     * the UE abandon a stack that is attached, or about to be. Seeded from the Binder
     * at initialization, which is still too early to see the result of dynamic cell
     * association. UE only: a base station learns of its UEs' handovers by signaling
     * and has no earlier local knowledge (its bearer legs consult the Binder).
     *
     * Delivery is push only: every change is fanned out to the modules that steer by
     * it (see pushServingNodeIds()), which keep mirrors and never call back into
     * RRC on the data path.
     */
    MacNodeId servingNodeId_ = NODEID_NONE;     // the LTE stack's serving node
    MacNodeId nrServingNodeId_ = NODEID_NONE;   // the NR stack's serving node

    // push target of the attachment ledger (UE only; see pushServingNodeIds())
    HandoverPacketHolderUe *handoverPacketHolderModule_ = nullptr;

    // The configuration entry for the bearer of an infrastructure unicast flow, as
    // delivered by configureDrb(), or nullptr. peerId is the flow's remote end; on the
    // UE side entries are keyed by NODEID_NONE instead.
    virtual const DrbDesc *lookupConfiguredDrb(const FlowId& flow, MacNodeId peerId);

    // Checks req.rlcMode before any use of it (entity type selection, materializeDrb),
    // called once at the top of each establishment entry point. The delivered configuration
    // wins (a conflicting explicit request throws). The bearer configurator resolves
    // every request before it reaches RRC (from the bearer's definition entry, or the fixed D2D/multicast
    // configuration), so an rlcMode still UNKNOWN_RLC_MODE here is a requester bug and
    // throws.
    virtual BearerRequest resolveBearerRequest(const BearerRequest& req, const FlowId& flow, MacNodeId peerId);

    virtual void setRlcEntityParams(cModule *entity, bool isNr);
    virtual void setEntityDisplayPosition(cModule *entity, bool isPdcpEntity, cModule *rlcMux, int bearerIndex);
    virtual cModule *lookupRlcEntityModule(DrbKey id, bool isNr);
    virtual cModule *findOrCreateRlcEntity(DrbKey id, RlcMode rlcMode, const FlowId& flow, RlcMux *rlcMux, bool isNr);
    virtual RlcTxEntityBase *installRlcTxSide(DrbKey id, const FlowId& flow, const BearerRequest& req, RlcMux *rlcMux, bool isNr);
    virtual RlcRxEntityBase *installRlcRxSide(DrbKey id, const FlowId& flow, const BearerRequest& req, RlcMux *rlcMux, bool isNr);
    virtual cModule *findOrCreatePdcpEntity(DrbKey id, const FlowId& flow, RlcMode rlcMode, RlcMux *rlcMux);
    virtual void installPdcpTxSide(DrbKey id, const FlowId& flow, RlcMode rlcMode, RlcMux *rlcMux, bool isNr);
    virtual void installPdcpRxSide(DrbKey id, const FlowId& flow, RlcMode rlcMode, RlcMux *rlcMux, bool isNr);
    virtual cModule *findOrCreatePdcpRelayEntity(DrbKey id, RlcMux *rlcMux);

    // The layout of a bearer over the node's stack legs. getNumLegs() gives the number of legs
    // the bearer's PDCP entity is built with, legCellGroups() the cell group of each leg
    // (authored, or derived: two legs = an MCG+SCG split bearer, one = MCG), and
    // selectPdcpLeg() the leg an establishment call attaches to (and, for a leg of a
    // bearer anchored elsewhere, the anchor bearer's key).
    virtual int getNumLegs(DrbKey id, const FlowId& flow);
    virtual std::vector<CellGroup> legCellGroups(DrbKey id, const FlowId& flow, int numLegs);
    virtual int selectPdcpLeg(MacNodeId peerId, const FlowId& flow, DrbKey& compoundId /*inout*/);

    // True when this node terminates the bearer's PDCP without carrying an RLC leg of it:
    // a DC master whose bearer states legs without an MCG leg (an SCG bearer, TS 37.340).
    // The core network still delivers to the master, so its PDCP legs are wired to the
    // X2 path and no local RLC/MAC state exists. peerId keys the configuration lookup.
    virtual bool isPdcpAnchorOnly(const FlowId& flow, MacNodeId peerId);

    // This node's own MacNodeId. A base station holds it under the technology it is, so a
    // gNB has no LTE id to ask for; a dual-stack UE has both, and its LTE id is its
    // identity (see Registration).
    virtual MacNodeId getOwnNodeId() const;

    // Is this one of the node's own ids? Spelling the question out as "the LTE id or the
    // NR id" is what breaks at a node whose stacks are not that pair.
    virtual bool isLocalNodeId(MacNodeId nodeId) const;

    // Whether this base station serves only a non-anchor leg of the given UE's bearers:
    // it is a secondary node and the UE is also attached to its master, where the
    // bearers' PDCP (and any SDAP) terminates. NODEB only; see configureDrb().
    virtual bool servesNonAnchorLegOnly(MacNodeId ueId);

    // Records the configuration of the bearer this establishment call sets up, from
    // exactly the derivations the entities are built from. Runs after the RLC entity
    // exists: the wire format and the SN field length are properties of the entity that
    // implements the bearer, and are read off it. Returns the descriptor so callers can
    // push it into MAC without a re-lookup.
    virtual const DrbDesc& materializeDrb(const FlowId& flow, const BearerRequest& req, MacNodeId peerId, DrbKey rlcId, bool isNr);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:
    ~BearerManagement() override;
    // Take delivery of one bearer's configuration from the serving base station's
    // control plane (see ConnectionControlEnb::installStaticDrb()), through this
    // node's entry point. RRC never fetches this itself.
    virtual void configureDrb(const DrbDesc& drb);

    // The bearer configuration delivered for the given peer (at a base station: a UE
    // leg), as configureDrb() recorded it
    virtual std::vector<DrbDesc> getConfiguredDrbs(MacNodeId peerId);
    // Take delivery of this UE's uplink QFI-assignment rules from the core network
    // (see CoreControl::deliverQfiRules()) and push them into the
    // QoS-flow classifier, which never authors rules of its own. UE with SDAP only.
    virtual void setUplinkQfiRules(QfiRuleSet&& rules);
    // Record that one of this UE's stacks is changing its serving node -- called by the
    // handover controller the instant a handover, attachment or detachment begins,
    // ahead of its execution. NODEID_NONE = the stack is detaching. Each updates the
    // ledger and pushes it to every consumer (see servingNodeId_). UE only.
    virtual void setServingNodeId(MacNodeId servingNodeId);
    virtual void setNrServingNodeId(MacNodeId servingNodeId);
    // Deliver the attachment ledger to everything that steers by it -- ~Ip2Nic, the
    // ~HandoverPacketHolderUe, and each bearer's leg splitter. Consumers keep pushed
    // mirrors; nothing pulls. A splitter created later gets its initial push at
    // creation (see findOrCreatePdcpEntity()).
    virtual void pushServingNodeIds();
    // Schedule an RLC-detected radio link failure teardown for a peer node, deferred
    // to a safe execution context. nrStack selects the failing leg (LTE vs NR).
    virtual void scheduleRadioLinkFailure(MacNodeId nodeId, bool nrStack);
    // Release + tear down the link to a peer (both legs, MAC/RLC/PDCP + Ip2Nic drop). Public
    // so the failing node can drive the symmetric teardown on the peer via the binder.
    virtual void releaseLink(MacNodeId peerId);
    virtual void createIncomingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp=true);
    virtual void createOutgoingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp=true);
    virtual RlcTxEntityBase *createRlcTxBuffer(DrbKey id, const FlowId& flow, const BearerRequest& req);
    virtual RlcRxEntityBase *createRlcRxBuffer(DrbKey id, const FlowId& flow, const BearerRequest& req);
    virtual PdcpTxEntityBase *lookupPdcpTxEntity(DrbKey id);
    virtual cModule *lookupPdcpEntityModule(DrbKey id);    // the per-bearer PdcpEntityBase compound (DcMux leg dispatch)
    virtual cModule *lookupPdcpRelayEntityModule(DrbKey id); // the per-bearer PdcpRelayEntity compound (DcMux DL dispatch)
    virtual void deleteLocalPdcpEntities(MacNodeId nodeId);
    virtual void deleteLocalRlcQueues(MacNodeId nodeId, bool nrStack=false);
    virtual void pdcpActiveUeUL(std::set<MacNodeId> *ueSet);
};

} // namespace simu5g

#endif

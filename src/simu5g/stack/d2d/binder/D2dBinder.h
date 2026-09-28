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

#ifndef _D2DBINDER_H_
#define _D2DBINDER_H_

#include <map>
#include <set>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class PhyBase;

/**
 * Global holder of the D2D-specific network state, split out of the core Binder.
 *
 * It keeps the D2D peering map (which pairs of D2D-capable UEs communicate in
 * Direct Mode or Infrastructure Mode) and the set of D2D one-to-many
 * transmitters, and answers the D2D capability/mode queries derived from them.
 *
 * There is a single instance per network. It is created lazily and dynamically
 * under the network module the first time any D2D code needs it (see
 * getInstance()), so that non-D2D networks pay nothing and existing network NED
 * files need no explicit 'd2dBinder' submodule.
 *
 * This module has no events, no emitted signals and no statistics: it is a
 * passive state container. That keeps it inert with respect to simulation
 * fingerprints even though it is created on the fly. It does listen to the
 * Binder's node-unregistered notification, to drop the state of nodes that
 * leave mid-simulation.
 */
class D2dBinder : public cSimpleModule, public cListener
{
  private:
    // reference to the core Binder, used for node and serving-cell lookups
    inet::ModuleRefByPar<Binder> binder_;

    // determines if two D2D-capable UEs are communicating in D2D mode or Infrastructure Mode
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>> d2dPeeringMap_;

    // set of D2D one-to-many (multicast) transmitters
    std::set<MacNodeId> multicastTransmitterSet_;

    // The DRB ids in use within each sidelink node pair -- a UE and a peer UE, or a UE
    // and a multicast group -- pooled network-wide, since a sidelink bearer's ends are
    // UEs whose serving base stations may differ and change (a base station pools the
    // pairs it is itself a party to, see ConnectionControlEnb::assignDrbId())
    std::map<std::pair<MacNodeId, MacNodeId>, std::set<DrbId>> sidelinkDrbIds_;

  public:
    /*
     * The flow that established each multicast bearer, kept so that a node joining the group
     * later can still be given its RX leg (see ConnectionControlEnbD2D::multicastGroupJoined()).
     * The sender's base station provisions the members that exist when the sender starts,
     * which is all of them only if the node population is static; with nodes created
     * during the simulation the joiner would otherwise receive PDUs for a connection its
     * stack knows nothing about.
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

  private:
    std::map<std::pair<MacNodeId, MacNodeId>, MulticastFlow> multicastFlows_;

    // D2D-capable UE PHYs by node id, registered at init (see registerD2dPhy)
    std::map<MacNodeId, opp_component_ptr<PhyBase>> d2dPhys_;

  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override {}

    // compute the initial D2D mode for the given transmitter/receiver UE pair
    virtual LteD2DMode computeD2DCapability(MacNodeId src, MacNodeId dst);

    /**
     * Binder::nodeUnregisteredSignal_: forget the node that has just left the simulation.
     * Both maps are keyed by node id, and the peering map holds the id inside every other
     * UE's map as well; a departed id left behind is later handed to getD2DCapability(),
     * which asserts that both of its arguments are registered UEs.
     */
    void receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details) override;

  public:
    /**
     * Returns the single D2dBinder instance for the network the given context
     * module belongs to, creating it dynamically under the network module on
     * first use (find-or-create).
     */
    static D2dBinder *getInstance(cModule *contextModule);

    /**
     * Registers a D2D-capable UE PHY under its node id. Called by the PHY at
     * INITSTAGE_SIMU5G_BINDER_ACCESS, so that the one-to-many transmit path can
     * resolve a peer PHY by id instead of walking submodule names.
     */
    virtual void registerD2dPhy(MacNodeId nodeId, PhyBase *phy);

    /**
     * Returns the registered D2D UE PHY of the given node, or nullptr if that
     * node has no D2D-capable PHY.
     */
    virtual PhyBase *getD2dPhy(MacNodeId nodeId);

    /**
     * If the src->dst peering is not yet known, computes and records it; returns
     * whether src may transmit to dst using D2D (either DM or IM).
     */
    virtual bool checkD2DCapability(MacNodeId src, MacNodeId dst);

    /**
     * Returns whether a (non-NONE) D2D peering has already been recorded for
     * src->dst.
     */
    virtual bool getD2DCapability(MacNodeId src, MacNodeId dst);

    /**
     * Returns the current D2D mode (DM or IM) of the src->dst peering; throws if
     * no peering is recorded.
     */
    virtual LteD2DMode getD2DMode(MacNodeId src, MacNodeId dst);

    /**
     * Read-only access to the whole peering map, for the mode-selection and
     * conflict-graph modules to iterate over.
     */
    const std::map<MacNodeId, std::map<MacNodeId, LteD2DMode>>& getD2DPeeringModeMap() const { return d2dPeeringMap_; }

    /**
     * Sets the D2D mode of the src->dst peering (used by the mode-selection
     * modules to apply a computed switch).
     */
    virtual void setD2DMode(MacNodeId src, MacNodeId dst, LteD2DMode mode);

    // add one D2D one-to-many (multicast) transmitter
    virtual void addD2DMulticastTransmitter(MacNodeId nodeId);
    // get the set of D2D one-to-many (multicast) transmitters
    virtual std::set<MacNodeId>& getD2DMulticastTransmitters();

    // The DRB id pool of a sidelink node pair (see sidelinkDrbIds_); the pair is ordered
    virtual std::set<DrbId>& sidelinkDrbIdPool(const std::pair<MacNodeId, MacNodeId>& pair);
    // Remember the flow that established a multicast bearer, unless the sender's flow
    // for the group is remembered already (see multicastFlows_)
    virtual void rememberMulticastFlow(MacNodeId groupId, MacNodeId senderId, const FlowId& flow, const BearerRequest& req, bool withPdcp);
    virtual const std::map<std::pair<MacNodeId, MacNodeId>, MulticastFlow>& getMulticastFlows() const { return multicastFlows_; }
};

} //namespace

#endif

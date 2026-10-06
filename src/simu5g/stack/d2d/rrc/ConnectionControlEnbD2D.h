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

#ifndef _SIMU5G_CONNECTIONCONTROLENBD2D_H_
#define _SIMU5G_CONNECTIONCONTROLENBD2D_H_

#include <map>

#include "simu5g/stack/rrc/ConnectionControlEnb.h"

namespace simu5g {

class D2dBinder;
class D2dModeSelectionBase;

/**
 * D2D-capable variant of the ConnectionControlEnb: the sidelink (D2D unicast and
 * multicast) bearers of the UEs the base station serves. See ConnectionControlEnbD2D.ned.
 */
class ConnectionControlEnbD2D : public ConnectionControlEnb
{
  protected:
    // the node's D2D mode selection, if it has one
    inet::ModuleRefByPar<D2dModeSelectionBase> d2dModeSelection_;

    // the network-wide sidelink state
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

    // the post-handover D2D mode re-selections pending, by the leg they are for
    std::map<omnetpp::cMessage *, MacNodeId> modeSwitchTimers_;

  protected:
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;

    // Ask the node's D2D mode selection to switch the leg's D2D flows: to
    // infrastructure mode before a handover, or (back) to direct mode after one
    virtual void requestModeSwitch(MacNodeId legId, bool handoverCompleted);

    // A D2D-capable leg: its D2D flows go back to infrastructure mode before the
    // source commands its handover; its D2D direction is attached at this node's AMC
    // when it arrives, followed by a D2D mode re-selection at the end of the instant;
    // and detached when it leaves
    void beforeHandoverCommand(MacNodeId legId, const UeContext& ctx) override;
    void legArrived(MacNodeId legId, const UeContext& ctx) override;
    void legLeft(MacNodeId legId, const UeContext& ctx) override;

    std::set<DrbId>& foreignPairPool(const std::pair<MacNodeId, MacNodeId>& pair) override;
    DrbId establishD2dBearer(const FlowId& flow, const FlowBindingKey& key) override;
    void createMulticastConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) override;

  public:
    ~ConnectionControlEnbD2D() override;

    // A node has joined a multicast group. If a sender has already established that
    // group's bearer, the node missed the RX-leg provisioning createMulticastConnection()
    // did over the membership as it stood then; give it one now, or its MAC will
    // receive PDUs for a connection it has no descriptor for and assert in
    // macPduUnmake(). Nodes that join before the bearer exists are covered by
    // createMulticastConnection() itself; createIncomingConnection() de-duplicates,
    // so a node reached by both paths is harmless.
    void multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId) override;
};

} //namespace

#endif

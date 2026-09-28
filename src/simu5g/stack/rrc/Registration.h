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

#ifndef _REGISTRATION_H_
#define _REGISTRATION_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"
#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>

using namespace omnetpp;

namespace simu5g {

/**
 * @brief RRC Registration — registers the node with the Binder, sets up
 *        the network interface, and joins multicast groups.
 */
class Registration : public cSimpleModule, public cListener
{
  private:
    MacNodeId lteNodeId = NODEID_NONE;
    MacNodeId nrNodeId = NODEID_NONE;
    RanNodeType nodeType = UNKNOWN_NODE_TYPE;

    // corresponding entry for our interface
    opp_component_ptr<inet::NetworkInterface> networkIf;

    inet::ModuleRefByPar<Binder> binder;
    inet::ModuleRefByPar<ConnectionControlBase> connectionControl;   // the node's control-plane entry point, told about multicast group joins

    // UE only: the cellular interface's addresses the Binder maps to this UE
    std::set<inet::L3Address> registeredAddresses;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
    void finish() override;

    virtual void registerInterface();
    virtual void registerMulticastGroups();

    // UE only: makes the Binder map exactly the cellular interface's current unicast
    // addresses, of both families, to this UE's LTE and NR node ids. Tentative IPv6
    // addresses (duplicate address detection still running) are left out.
    virtual void registerAddresses();

    // Interface configuration changes: the UE's addresses can change during the run
    // (an IPv6 link-local address, for one, appears only when Neighbor Discovery has
    // started up), and the Binder must follow them
    void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

  public:
    RanNodeType getNodeType() const { return nodeType; }
    MacNodeId getLteNodeId() const { return lteNodeId; }
    MacNodeId getNrNodeId() const { return nrNodeId; }
    bool isDualTechnology() const { return lteNodeId != NODEID_NONE && nrNodeId != NODEID_NONE; }
};

} // namespace simu5g

#endif

//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __HANDOVERPACKETFILTER_H_
#define __HANDOVERPACKETFILTER_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/handoverManager/LteHandoverManager.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/ip2nic/SplitBearersTable.h"

namespace simu5g {

using namespace omnetpp;

class LteHandoverManager;

//TODO split UPWARD part, split UE and NODEB halves --> HandoverPacketBufferUe, HandoverPacketBufferEnb

/**
 *
 */
class HandoverPacketFilter : public cSimpleModule
{
  public: //protected:
    RanNodeType nodeType_;      // UE or NODEB

    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // LTE MAC node id of this node
    MacNodeId nodeId_ = NODEID_NONE;
    // NR MAC node id of this node (if enabled)
    MacNodeId nrNodeId_ = NODEID_NONE;

    // LTE MAC node id of this node's master
    MacNodeId servingNodeId_ = NODEID_NONE;
    // NR MAC node id of this node's master (if enabled)
    MacNodeId nrServingNodeId_ = NODEID_NONE;

    /*
     * Handover support
     */

    // manager for the handover
    inet::ModuleRefByPar<LteHandoverManager> hoManager_;
    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<inet::Packet *> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

    bool ueHold_ = false;
    IpDatagramQueue ueHoldFromIp_;

    cGate *stackGateOut_ = nullptr;

    // corresponding entry for our interface
    opp_component_ptr<inet::NetworkInterface> networkIf;

  public: //  protected:
    /**
     * Handle packets from transport layer and forward them to the stack
     */
    void fromIpUe(inet::Packet *datagram);

    /**
     * Manage packets received from the stack
     * and forward them to transport layer.
     */
    virtual void prepareForIpv4(inet::Packet *datagram, const inet::Protocol *protocol = & inet::Protocol::ipv4);
    virtual void fromIpBs(inet::Packet *datagram);
    virtual void toStackBs(inet::Packet *datagram);
    virtual void toStackUe(inet::Packet *datagram);

    /**
     * utility: set nodeType_ field
     *
     * @param s string containing the node type ("enodeb", "gnodeb", "ue")
     */
    void setNodeType(std::string s);

    /**
     * utility: print LteStackControlInfo fields
     *
     * @param ci LteStackControlInfo object
     */
    void printControlInfo(inet::Packet *pkt);

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:

    /*
     * Handover management at the BS side
     */
    void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    void sendTunneledPacketOnHandover(inet::Packet *datagram, MacNodeId targetEnb);
    void receiveTunneledPacketOnHandover(inet::Packet *datagram);
    void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    void signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb);

    /*
     * Handover management at the UE side
     */
    void triggerHandoverUe(MacNodeId newMasterId, bool isNr = false);
    void signalHandoverCompleteUe(bool isNr = false);

    ~HandoverPacketFilter() override;
};

} //namespace

#endif

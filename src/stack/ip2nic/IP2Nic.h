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

#ifndef __IP2NIC_H_
#define __IP2NIC_H_

#include <omnetpp.h>
#include <inet/networklayer/common/InterfaceEntry.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/handoverManager/LteHandoverManager.h"
#include "common/binder/Binder.h"
#include "stack/ip2nic/SplitBearersTable.h"


class LteHandoverManager;

// a sort of five-tuple with only two elements (a two-tuple...), src and dst addresses
typedef std::pair<inet::Ipv4Address, inet::Ipv4Address> AddressPair;

/**
 *
 */
class IP2Nic : public omnetpp::cSimpleModule
{
    omnetpp::cGate *stackGateOut_;       // gate connecting IP2Nic module to cellular stack
    omnetpp::cGate *ipGateOut_;          // gate connecting IP2Nic module to network layer

    RanNodeType nodeType_;      // node type: can be ENODEB, GNODEB, UE

    // reference to the binder
    Binder* binder_;

    // LTE MAC node id of this node
    MacNodeId nodeId_;
    // NR MAC node id of this node (if enabled)
    MacNodeId nrNodeId_;

    // LTE MAC node id of this node's master
    MacNodeId masterId_;
    // NR MAC node id of this node's master (if enabled)
    MacNodeId nrMasterId_;

    // corresponding entry for our interface
    inet::InterfaceEntry* interfaceEntry;

    /*
     * Handover support
     */

    // manager for the handover
    LteHandoverManager* hoManager_;
    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<inet::Packet*> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

    bool ueHold_;
    IpDatagramQueue ueHoldFromIp_;

    /*
     * NR Dual Connectivity support
     */
    // enabler for dual connectivity
    bool dualConnectivityEnabled_;

    // for each connection exploiting Split Bearer,
    // keep trace of the number of packets sent down to the PDCP
    SplitBearersTable* sbTable_;

  protected:
    /**
     * Handle packets from transport layer and forward them to the stack
     */
    void fromIpUe(inet::Packet * datagram);

    /**
     * Manage packets received from the stack
     * and forward them to transport layer.
     */
    virtual void prepareForIpv4(inet::Packet *datagram, const inet::Protocol *protocol = &inet::Protocol::ipv4);
    virtual void toIpUe(inet::Packet *datagram);
    virtual void fromIpBs(inet::Packet * datagram);
    virtual void toIpBs(inet::Packet * datagram);
    virtual void toStackBs(inet::Packet* datagram);
    virtual void toStackUe(inet::Packet* datagram);

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
    void printControlInfo(inet::Packet* pkt);
    void registerInterface();
    void registerMulticastGroups();

    // mark packet for using LTE, NR or split bearer
    //
    // In the current version, the Ip2Nic module of the master eNB (the UE) selects which path
    // to follow based on the Type of Service (TOS) field:
    // - use master eNB if tos < 10
    // - use secondary gNB if 10 <= tos < 20
    // - use split bearer if tos >= 20
    //
    // To change the policy, change the implementation of the Ip2Nic::markPacket() function
    //
    // TODO use a better policy
    bool markPacket(FlowControlInfo* ci);

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::INITSTAGE_LAST; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;
  public:

    /*
     * Handover management at the BS side
     */
    void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    void sendTunneledPacketOnHandover(inet::Packet* datagram, MacNodeId targetEnb);
    void receiveTunneledPacketOnHandover(inet::Packet* datagram, MacNodeId sourceEnb);
    void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    void signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb);

    /*
     * Handover management at the UE side
     */
    void triggerHandoverUe(MacNodeId newMasterId, bool isNr=false);
    void signalHandoverCompleteUe(bool isNr=false);

    virtual ~IP2Nic();
};

#endif

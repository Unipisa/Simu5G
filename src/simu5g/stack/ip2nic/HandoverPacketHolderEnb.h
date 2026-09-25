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

#ifndef __HANDOVERPACKETHOLDERENB_H_
#define __HANDOVERPACKETHOLDERENB_H_

#include <inet/common/ModuleRefByPar.h>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/SessionTag_m.h"

namespace simu5g {

using namespace omnetpp;

class HandoverX2Forwarder;

/**
 *
 */
//TODO write docu
class HandoverPacketHolderEnb : public cSimpleModule
{
  protected:
    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // MAC node id of this node
    bool amNr_ = false;    // this node's technology, from the Binder; decides which of a UE's ids this node handles
    MacNodeId nodeId_ = NODEID_NONE;

    inet::ModuleRefByPar<HandoverX2Forwarder> hoManager_;
    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<inet::Packet *> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

     cGate *stackGateOut_ = nullptr;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    // The UE of the datagram's PDU session, by the id of this node's own cell group
    virtual MacNodeId resolveUeNodeId(const SessionTag *session);
    void handleMessage(cMessage *msg) override;

    virtual void fromIpBs(inet::Packet *datagram);
    virtual void toStackBs(inet::Packet *datagram);

  public:
    ~HandoverPacketHolderEnb() override;
    virtual void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    virtual void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    virtual void sendTunneledPacketOnHandover(inet::Packet *datagram, MacNodeId targetEnb);
    virtual void receiveTunneledPacketOnHandover(inet::Packet *datagram);
    virtual void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    virtual void signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb);
};

} //namespace

#endif

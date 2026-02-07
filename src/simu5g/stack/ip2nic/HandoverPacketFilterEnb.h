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

#ifndef __HANDOVERPACKETFILTERENB_H_
#define __HANDOVERPACKETFILTERENB_H_

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

/**
 *
 */
class HandoverPacketFilterEnb : public cSimpleModule
{
  protected:
    // reference to the binder
    inet::ModuleRefByPar<Binder> binder_;

    // MAC node id of this node
    MacNodeId nodeId_ = NODEID_NONE;

    inet::ModuleRefByPar<LteHandoverManager> hoManager_;
    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<inet::Packet *> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

     cGate *stackGateOut_ = nullptr;

  protected:
    virtual void fromIpBs(inet::Packet *datagram);
    virtual void toStackBs(inet::Packet *datagram);

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:
    ~HandoverPacketFilterEnb() override;
    void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    void sendTunneledPacketOnHandover(inet::Packet *datagram, MacNodeId targetEnb);
    void receiveTunneledPacketOnHandover(inet::Packet *datagram);
    void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    void signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb);
};

} //namespace

#endif

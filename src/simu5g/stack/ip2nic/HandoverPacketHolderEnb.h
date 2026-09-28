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

    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<inet::Packet *> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

    // At a handover target: the UEs whose downlink path the handover has switched here,
    // with the End Marker of the old path still to come (see GtpEndMarkerInd); their
    // downlink from the new path is held back until it arrives
    std::set<MacNodeId> awaitingEndMarker_;

     cGate *stackGateOut_ = nullptr;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    // The UE of the datagram's session, by the id of this node's own cell group
    virtual MacNodeId resolveUeNodeId(const SessionTag *session);
    virtual MacNodeId resolveUeNodeId(MacNodeId lteNodeId, MacNodeId nrNodeId);
    void handleMessage(cMessage *msg) override;

    virtual void fromIpBs(inet::Packet *datagram);
    virtual void toStackBs(inet::Packet *datagram);

    // At the old base station: relay the End Marker of a session's downlink to the
    // base station the UE went to
    virtual void relayEndMarker(inet::Packet *endMarker);
    // At the new base station: the End Marker relayed by the old one arrived
    virtual void receiveEndMarker(inet::Packet *endMarker);
    // Send down the downlink held back from the new path for the UE
    virtual void releaseHeldDownlink(MacNodeId ueId);

  public:
    ~HandoverPacketHolderEnb() override;
    virtual void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    virtual void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    virtual void sendTunneledPacketOnHandover(inet::Packet *datagram, MacNodeId targetEnb);
    virtual void receiveTunneledPacketOnHandover(inet::Packet *datagram);
    virtual void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    virtual void signalHandoverCompleteTarget(MacNodeId ueId);

    // Called by the node's control plane (ConnectionControlEnb::downlinkPathSwitched())
    // at the path switch: the downlink of the session of the UE with the given
    // node ids now enters the RAN here, and entered it at fromBaseStation before
    // (NODEID_NONE: nowhere), where the anchor ends it with an End Marker if it is
    // another base station
    virtual void switchDownlinkPath(MacNodeId ueLteId, MacNodeId ueNrId, MacNodeId fromBaseStation);
};

} //namespace

#endif

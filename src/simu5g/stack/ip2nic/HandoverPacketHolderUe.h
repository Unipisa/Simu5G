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

#ifndef __HANDOVERPACKETHOLDERUE_H_
#define __HANDOVERPACKETHOLDERUE_H_

#include <omnetpp.h>
#include <inet/common/packet/Packet.h>
#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;


/**
 *
 */
class HandoverPacketHolderUe : public cSimpleModule
{
  protected:

    // mirror of RRC's stack attachment ledger, pushed on every handover event (see
    // setServingNodeIds()); NODEID_NONE = not attached
    MacNodeId servingNodeId_ = NODEID_NONE;     // the LTE stack's serving node
    MacNodeId nrServingNodeId_ = NODEID_NONE;   // the NR stack's serving node

    SessionType sessionType_ = IP_V4;   // the type of the UE's session (the sessionType parameter)

    bool ueHold_ = false;
    typedef std::list<inet::Packet *> IpDatagramQueue;
    IpDatagramQueue ueHoldFromIp_;

    cGate *stackGateOut_ = nullptr;

  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override;

    virtual void fromIpUe(inet::Packet *datagram);
    virtual void toStackUe(inet::Packet *datagram);

  public:
    ~HandoverPacketHolderUe() override;
    virtual void triggerHandoverUe(MacNodeId newMasterId);
    virtual void signalHandoverCompleteUe(bool isNr = false);

    // RRC's push of the stacks' attachment (see BearerManagement::pushServingNodeIds()):
    // the serving node of this UE's LTE and NR stack, current as of handover start.
    virtual void setServingNodeIds(MacNodeId servingNodeId, MacNodeId nrServingNodeId);
};

} //namespace

#endif

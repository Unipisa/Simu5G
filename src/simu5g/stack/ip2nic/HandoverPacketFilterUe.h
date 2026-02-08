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

#ifndef __HANDOVERPACKETFILTERUE_H_
#define __HANDOVERPACKETFILTERUE_H_

#include <inet/common/ModuleRefByPar.h>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

/**
 *
 */
class HandoverPacketFilterUe : public cSimpleModule
{
  protected:

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

    bool ueHold_ = false;
    typedef std::list<inet::Packet *> IpDatagramQueue;
    IpDatagramQueue ueHoldFromIp_;

    cGate *stackGateOut_ = nullptr;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    void fromIpUe(inet::Packet *datagram);
    virtual void toStackUe(inet::Packet *datagram);

  public:
    ~HandoverPacketFilterUe() override;
    void triggerHandoverUe(MacNodeId newMasterId, bool isNr = false);
    void signalHandoverCompleteUe(bool isNr = false);

    MacNodeId getServingNodeId() const { return servingNodeId_; }
    MacNodeId getNrServingNodeId() const { return nrServingNodeId_; }

};

} //namespace

#endif

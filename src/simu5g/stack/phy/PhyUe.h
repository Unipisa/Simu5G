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

#ifndef _PHYUE_H_
#define _PHYUE_H_

#include <inet/mobility/contract/IMobility.h>

#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/rrc/ConnectionControlUe.h"

namespace simu5g {

using namespace omnetpp;

class PhyUe : public PhyBase
{
  protected:
    /** Serving node MacNodeId */
    MacNodeId servingNodeId_ = NODEID_NONE;

    /** Reference to master node's mobility module */
    opp_component_ptr<IMobility> servingNodeMobility_;

    /** Statistic for distance from serving cell */
    static simsignal_t distanceSignal_;

    inet::ModuleRefByPar<ConnectionControlUe> connectionControl_;

    simtime_t lastFeedback_ = 0;

    // Support to print average CQI at the end of the simulation
    std::vector<short int> cqiDlSamples_;
    std::vector<short int> cqiUlSamples_;
    unsigned int cqiDlSum_ = 0;
    unsigned int cqiUlSum_ = 0;
    unsigned int cqiDlCount_ = 0;
    unsigned int cqiUlCount_ = 0;

    void initialize(int stage) override;
    void handleSelfMessage(cMessage *msg) override;
    void handleAirFrame(cMessage *msg) override;

    /// stale-source test on the receive path
    /// (default: any frame not sent by the serving cell)
    virtual bool isStaleFrame(const UserControlInfo *lteInfo) { return lteInfo->getSourceId() != servingNodeId_; }

    /// called once an incoming frame has passed the acceptance checks (default: nothing)
    virtual void frameAccepted(UserControlInfo *lteInfo) {}

    /// frame types handed to handleControlMsg() on the receive path
    virtual bool isControlFrameType(LtePhyFrameType type) { return type == HARQPKT || type == GRANTPKT || type == RACPKT; }

    /// gives subclasses a chance to consume an incoming data frame before decoding (default: no)
    virtual bool interceptIncomingFrame(LteAirFrame *frame, UserControlInfo *lteInfo) { return false; }
    void finish() override;
    void finish(cComponent *component, simsignal_t signalID) override { cIListener::finish(component, signalID); }

    void handleUpperMessage(cMessage *msg) override;

    /// checks an outgoing upper-layer packet before transmission
    /// (default: it must target the serving cell)
    virtual void validateOutgoingFrame(const UserControlInfo *info);

    /// CQI accounting for outgoing data packets in directions other than UL
    virtual void recordExtraTxCqi(double cqi, const UserControlInfo *info) {}

    void emitMobilityStats() override;

  public:
    ~PhyUe() override;
    /**
     * Send feedback, called by feedback generator in DL
     */
    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req);

    virtual double computeReceivedBeaconPacketRssi(LteAirFrame *frame, UserControlInfo *lteInfo);

    virtual void findCandidateEnb(MacNodeId& outCandidateMasterId, double& outCandidateMasterRssi);

    // called on handover
    virtual void changeServingNode(MacNodeId masterId);

    virtual void recordCqi(unsigned int sample, Direction dir);
    virtual double getAverageCqi(Direction dir);
    virtual double getVarianceCqi(Direction dir);
};

} //namespace

#endif /* _PHYUE_H_ */

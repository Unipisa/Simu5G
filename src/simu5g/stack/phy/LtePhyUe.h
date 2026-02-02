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

#ifndef _LTE_AIRPHYUE_H_
#define _LTE_AIRPHYUE_H_

#include <inet/mobility/contract/IMobility.h>

#include "simu5g/stack/phy/LtePhyBase.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/rrc/HandoverController.h"

namespace simu5g {

using namespace omnetpp;

class LtePhyUe : public LtePhyBase
{
  protected:
    /** Serving node MacNodeId */
    MacNodeId servingNodeId_ = NODEID_NONE;

    /** Reference to master node's mobility module */
    opp_component_ptr<IMobility> servingNodeMobility_;

    /** Statistic for distance from serving cell */
    static simsignal_t distanceSignal_;

    opp_component_ptr<LteMacUe> mac_;
    inet::ModuleRefByPar<HandoverController> handoverController_;

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
    void finish() override;
    void finish(cComponent *component, simsignal_t signalID) override { cIListener::finish(component, signalID); }

    void handleUpperMessage(cMessage *msg) override;

    void emitMobilityStats() override;

  public:
    ~LtePhyUe() override;
    /**
     * Send feedback, called by feedback generator in DL
     */
    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req);

    virtual double computeReceivedBeaconPacketRssi(LteAirFrame *frame, UserControlInfo *lteInfo);

    virtual void findCandidateEnb(MacNodeId& outCandidateMasterId, double& outCandidateMasterRssi);

    // called on handover
    void changeServingNode(MacNodeId masterId);

    void recordCqi(unsigned int sample, Direction dir);
    double getAverageCqi(Direction dir);
    double getVarianceCqi(Direction dir);
};

} //namespace

#endif /* _LTE_AIRPHYUE_H_ */

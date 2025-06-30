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

#ifndef _LTE_AIRPHYUE_H_
#define _LTE_AIRPHYUE_H_

#include <inet/mobility/contract/IMobility.h>

#include "stack/ip2nic/IP2Nic.h"
#include "stack/phy/LtePhyBase.h"
#include "stack/phy/das/DasFilter.h"
#include "stack/mac/LteMacUe.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"
#include "stack/phy/feedback/LteDlFeedbackGenerator.h"

namespace simu5g {

using namespace omnetpp;

class DasFilter;
class LteDlFeedbackGenerator;

class LtePhyUe : public LtePhyBase
{
  protected:
    /** Master MacNodeId */
    MacNodeId masterId_ = NODEID_NONE;

    /** Reference to master node's mobility module */
    opp_component_ptr<IMobility> masterMobility_;

    /** Statistic for distance from serving cell */
    static simsignal_t distanceSignal_;

    /** Statistic for serving cell */
    static simsignal_t servingCellSignal_;

    /** Self message to trigger handover procedure evaluation */
    cMessage *handoverStarter_ = nullptr;

    /** Self message to start the handover procedure */
    cMessage *handoverTrigger_ = nullptr;

    /** RSSI received from the current serving node */
    double currentMasterRssi_;

    /** ID of the not-master node from which the highest RSSI was received */
    MacNodeId candidateMasterId_;

    /** Highest RSSI received from not-master node */
    double candidateMasterRssi_;

    /**
     * Hysteresis threshold to evaluate handover: it introduces a small polarization to
     * avoid multiple subsequent handovers
     */
    double hysteresisTh_;

    /**
     * Value used to divide currentMasterRssi_ and create a hysteresisTh_
     * Use zero to have hysteresisTh_ == 0.
     */
    // TODO: bring it to ned par!
    double hysteresisFactor_;

    /**
     * Time interval elapsing from the reception of the first handover broadcast message
     * to the beginning of the handover procedure.
     * It must be a small number greater than 0 to ensure that all broadcast messages
     * are received before evaluating handover.
     * Note that broadcast messages for handover are always received at the very same time
     * (at bdcUpdateInterval_ seconds intervals).
     */
    // TODO: bring it to ned par!
    double handoverDelta_;

    // Time for completion of the handover procedure
    double handoverLatency_;
    double handoverDetachment_;
    double handoverAttachment_;

    // Lower threshold of RSSI for detachment
    double minRssi_;

    /**
     * Handover switch
     */
    bool enableHandover_;

    /**
     * Pointer to the DAS Filter: used to call DAS functions
     * when receiving broadcasts and to retrieve physical
     * antenna properties on packet reception
     */
    DasFilter *das_ = nullptr;

    /// Threshold for antenna association
    // TODO: bring it to ned par!
    double dasRssiThreshold_;

    opp_component_ptr<LteMacUe> mac_;
    inet::ModuleRefByPar<LteRlcUm> rlcUm_;
    inet::ModuleRefByPar<LtePdcpRrcBase> pdcp_;
    inet::ModuleRefByPar<IP2Nic> ip2nic_;
    inet::ModuleRefByPar<LteDlFeedbackGenerator> fbGen_;

    simtime_t lastFeedback_;

    // Support to print average CQI at the end of the simulation
    std::vector<short int> cqiDlSamples_;
    std::vector<short int> cqiUlSamples_;
    unsigned int cqiDlSum_ = 0;
    unsigned int cqiUlSum_ = 0;
    unsigned int cqiDlCount_ = 0;
    unsigned int cqiUlCount_ = 0;

    bool hasCollector = false;

    void initialize(int stage) override;
    void handleSelfMessage(cMessage *msg) override;
    void handleAirFrame(cMessage *msg) override;
    void finish() override;
    void finish(cComponent *component, simsignal_t signalID) override { cIListener::finish(component, signalID); }

    void handleUpperMessage(cMessage *msg) override;

    void emitMobilityStats() override;

    /**
     * Utility function to update the hysteresis threshold using hysteresisFactor_.
     */
    double updateHysteresisTh(double v);

    void handoverHandler(LteAirFrame *frame, UserControlInfo *lteInfo);

    void deleteOldBuffers(MacNodeId masterId);

    virtual void triggerHandover();
    virtual void doHandover();

  public:
    ~LtePhyUe() override;
    DasFilter *getDasFilter();
    /**
     * Send feedback, called by feedback generator in DL
     */
    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req);
    MacNodeId getMasterId() const
    {
        return masterId_;
    }

    simtime_t coherenceTime(double speed)
    {
        double fd = (speed / SPEED_OF_LIGHT) * carrierFrequency_;
        return 0.1 / fd;
    }

    void recordCqi(unsigned int sample, Direction dir);
    double getAverageCqi(Direction dir);
    double getVarianceCqi(Direction dir);
};

} //namespace

#endif /* _LTE_AIRPHYUE_H_ */


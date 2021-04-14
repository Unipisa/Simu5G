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

#include "stack/phy/layer/LtePhyBase.h"
#include "stack/phy/das/DasFilter.h"
#include "stack/mac/layer/LteMacUe.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

class DasFilter;

class LtePhyUe : public LtePhyBase
{
  protected:
    /** Master MacNodeId */
    MacNodeId masterId_;

    /** Statistic for serving cell */
    omnetpp::simsignal_t servingCell_;

    /** Self message to trigger handover procedure evaluation */
    omnetpp::cMessage *handoverStarter_;

    /** Self message to start the handover procedure */
    omnetpp::cMessage *handoverTrigger_;

    /** RSSI received from the current serving node */
    double currentMasterRssi_;

    /** ID of not-master node from wich highest RSSI was received */
    MacNodeId candidateMasterId_;

    /** Highest RSSI received from not-master node */
    double candidateMasterRssi_;

    /**
     * Hysteresis threshold to evaluate handover: it introduces a small polarization to
     * avoid multiple subsequent handovers
     */
    double hysteresisTh_;

    /**
     * Value used to divide currentMasterRssi_ and create an hysteresisTh_
     * Use zero to have hysteresisTh_ == 0.
     */
    // TODO: bring it to ned par!
    double hysteresisFactor_;

    /**
     * Time interval elapsing from the reception of first handover broadcast message
     * to the beginning of handover procedure.
     * It must be a small number greater than 0 to ensure that all broadcast messages
     * are received before evaluating handover.
     * Note that broadcast messages for handover are always received at the very same time
     * (at bdcUpdateInterval_ seconds intervals).
     */
    // TODO: bring it to ned par!
    double handoverDelta_;

    // time for completion of the handover procedure
    double handoverLatency_;
    double handoverDetachment_;
    double handoverAttachment_;

    // lower threshold of RSSI for detachment
    double minRssi_;

    /**
     * Handover switch
     */
    bool enableHandover_;

    /**
     * Pointer to the DAS Filter: used to call das function
     * when receiving broadcasts and to retrieve physical
     * antenna properties on packet reception
     */
    DasFilter* das_;

    /// Threshold for antenna association
    // TODO: bring it to ned par!
    double dasRssiThreshold_;

    /** set to false if a battery is not present in module or must have infinite capacity */
    bool useBattery_;
    double txAmount_;    // drawn current amount for tx operations (mA)
    double rxAmount_;    // drawn current amount for rx operations (mA)

    LteMacUe *mac_;
    LteRlcUm *rlcUm_;
    LtePdcpRrcBase *pdcp_;

    omnetpp::simtime_t lastFeedback_;

    virtual void initialize(int stage) override;
    virtual void handleSelfMessage(omnetpp::cMessage *msg) override;
    virtual void handleAirFrame(omnetpp::cMessage* msg) override;
    virtual void finish() override;
    virtual void finish(cComponent *component, omnetpp::simsignal_t signalID) override {cIListener::finish(component, signalID);}

    virtual void handleUpperMessage(omnetpp::cMessage* msg) override;


    /**
     * Utility function to update the hysteresis threshold using hysteresisFactor_.
     */
    double updateHysteresisTh(double v);

    void handoverHandler(LteAirFrame* frame, UserControlInfo* lteInfo);

    void deleteOldBuffers(MacNodeId masterId);

    virtual void triggerHandover();
    virtual void doHandover();

  public:
    LtePhyUe();
    virtual ~LtePhyUe();
    DasFilter *getDasFilter();
    /**
     * Send Feedback, called by feedback generator in DL
     */
    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req);
    MacNodeId getMasterId() const
    {
        return masterId_;
    }
    omnetpp::simtime_t coherenceTime(double speed)
    {
        double fd = (speed / SPEED_OF_LIGHT) * carrierFrequency_;
        return 0.1 / fd;
    }
};

#endif  /* _LTE_AIRPHYUE_H_ */

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

#ifndef _LTE_LTEDLFBGENERATOR_H_
#define _LTE_LTEDLFBGENERATOR_H_

#include <omnetpp.h>

#include "common/cellInfo/CellInfo.h"
#include "common/LteCommon.h"
#include "stack/phy/das/DasFilter.h"
#include "stack/phy/feedback/LteFeedback.h"
#include "common/timer/TTimer.h"
#include "common/timer/TTimerMsg_m.h"
#include "stack/phy/feedback/LteFeedbackComputation.h"

class DasFilter;
/**
 * @class LteDlFeedbackGenerator
 * @brief Lte Downlink Feedback Generator
 *
 */
class LteDlFeedbackGenerator : public omnetpp::cSimpleModule
{
    enum FbTimerType
    {
        PERIODIC_SENSING = 0, PERIODIC_TX, APERIODIC_TX
    };

  private:

    FeedbackType fbType_;               /// feedback type (ALLBANDS, PREFERRED, WIDEBAND)
    RbAllocationType rbAllocationType_; /// resource allocation type
    // LteFeedbackComputation* lteFeedbackComputation_; // Object used to compute the feedback
    FeedbackGeneratorType generatorType_;
    /**
     * NOTE: fbPeriod_ MUST be greater than fbDelay_,
     * otherwise we have overlapping transmissions
     * for periodic feedback (when calling start() on busy
     * transmission timer we have no operation)
     */
    omnetpp::simtime_t fbPeriod_;    /// period for Periodic feedback in TTI
    omnetpp::simtime_t fbDelay_;     /// time interval between sensing and transmission in TTI

    bool usePeriodic_;      /// true if we want to use also periodic feedback
    TxMode currentTxMode_;  /// transmission mode to use in feedback generation

    DasFilter *dasFilter_;  /// reference to das filter
    CellInfo *cellInfo_; /// reference to cellInfo

    // cellInfo parameters
    std::map<Remote, int> antennaCws_; /// number of antenna per remote
    int numPreferredBands_;           /// number of preferred bands to use (meaningful only in PREFERRED mode)
    int numBands_;                      /// number of cell bands

    // Timers
    TTimer *tPeriodicSensing_;
    TTimer *tPeriodicTx_;
    TTimer *tAperiodicTx_;

    // Feedback Maps
    //typedef std::map<Remote,LteFeedback> FeedbackMap_;
    LteFeedbackDoubleVector periodicFeedback;
    LteFeedbackDoubleVector aperiodicFeedback;

    //MacNodeID
    MacNodeId masterId_;
    MacNodeId nodeId_;

    bool feedbackComputationPisa_;
    private:

    // initialize cell information
    void initCellInfo();

    /**
     * DUMMY: should be provided by PHY
     */
    void sendFeedback(LteFeedbackDoubleVector fb, FbPeriodicity per);


    LteFeedbackComputation* getFeedbackComputationFromName(std::string name, ParameterMap& params);


  protected:

    /**
     * Initialization function.
     */
    virtual void initialize(int stage) override;

    /**
     * Manage self messages for sensing and transmission.
     * @param msg self message for sensing or transmission
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * Channel sensing
     */
    void sensing(FbPeriodicity per);
    virtual int numInitStages() const override { return inet::INITSTAGE_LINK_LAYER + 1; }

  public:

    /**
     * Constructor
     */
    LteDlFeedbackGenerator();

    /**
     * Destructor
     */
    ~LteDlFeedbackGenerator();

    /**
     * Function used to register an aperiodic feedback request
     * to the Downlink Feedback Generator.
     * Called by PHY.
     */
    void aperiodicRequest();

    /**
     * Function used to set the current Tx Mode.
     * Called by PHY.
     * @param newTxMode new transmission mode
     */
    void setTxMode(TxMode newTxMode);

    /*
     * Perform handover-related operations
     * Update cell id and the reference to the cellInfo
     */
    void handleHandover(MacCellId newEnbId);
};

#endif

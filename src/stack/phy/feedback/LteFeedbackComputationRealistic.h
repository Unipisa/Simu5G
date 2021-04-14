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

#ifndef _LTE_LTEFEEDBACKCOMPUTATIONREALISTIC_H_
#define _LTE_LTEFEEDBACKCOMPUTATIONREALISTIC_H_

#include "stack/phy/feedback/LteFeedbackComputation.h"

class PhyPisaData;
class LteFeedbackComputationRealistic : public LteFeedbackComputation
{
    // Channel matrix struct
    std::map<MacNodeId, Lambda>* lambda_;
    //Target Bler
    double targetBler_;
    //Number of logical bands
    unsigned int numBands_;
    //Lambda threshold
    double lambdaMinTh_;
    double lambdaMaxTh_;
    double lambdaRatioTh_;
    //pointer to pisadata
    PhyPisaData* phyPisaData_;

    std::vector<double> baseMin_;

  protected:
    // Rank computation
    unsigned int computeRank(MacNodeId id);
    // Generate base feedback for all types of feedback(allbands, preferred, wideband)
    void generateBaseFeedback(int numBands, int numPreferredBabds, LteFeedback& fb, FeedbackType fbType, int cw,
        RbAllocationType rbAllocationType, TxMode txmode, std::vector<double> snr);
    // Get cqi from BLer Curves
    Cqi getCqi(TxMode txmode, double snr);
    double meanSnr(std::vector<double> snr);
    public:
    LteFeedbackComputationRealistic(double targetBler, std::map<MacNodeId, Lambda>* lambda, double lambdaMinTh,
        double lambdaMaxTh, double lambdaRatioTh, unsigned int numBands);
    virtual ~LteFeedbackComputationRealistic();

    virtual LteFeedbackDoubleVector computeFeedback(FeedbackType fbType, RbAllocationType rbAllocationType,
        TxMode currentTxMode,
        std::map<Remote, int> antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype,
        int numRus, std::vector<double> snr, MacNodeId id = 0);

    virtual LteFeedbackVector computeFeedback(const Remote remote, FeedbackType fbType,
        RbAllocationType rbAllocationType, TxMode currentTxMode,
        int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id = 0);

    virtual LteFeedback computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
        RbAllocationType rbAllocationType,
        int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id = 0);
};

#endif

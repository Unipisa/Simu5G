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

#ifndef _LTE_LTEFEEDBACKCOMPUTATIONREALISTIC_H_
#define _LTE_LTEFEEDBACKCOMPUTATIONREALISTIC_H_

#include "simu5g/stack/phy/feedback/LteFeedbackComputation.h"

namespace simu5g {

class PhyPisaData;
class LteFeedbackComputationRealistic : public LteFeedbackComputation
{
    // Target BLER
    double targetBler_;
    // Number of logical bands
    unsigned int numBands_;
    // Pointer to Pisa data
    PhyPisaData *phyPisaData_ = nullptr;

    std::vector<double> baseMin_;

  protected:
    // Rank computation
    unsigned int computeRank(MacNodeId id);
    // Generate base feedback for all types of feedback (all bands, preferred, wideband)
    void generateBaseFeedback(int numBands, int numPreferredBands, LteFeedback& fb, FeedbackType fbType, int cw,
            RbAllocationType rbAllocationType, TxMode txmode, std::vector<double> snr);
    // Get CQI from BLER Curves
    Cqi getCqi(TxMode txmode, double snr);
    double meanSnr(std::vector<double> snr);

  public:
    LteFeedbackComputationRealistic(Binder *binder, double targetBler, unsigned int numBands);

    LteFeedbackDoubleVector computeFeedback(FeedbackType fbType, RbAllocationType rbAllocationType,
            TxMode currentTxMode,
            std::map<Remote, int> antennaCws, int numPreferredBands,
            int numRus, std::vector<double> snr, MacNodeId id = NODEID_NONE) override;

    LteFeedbackVector computeFeedback(const Remote remote, FeedbackType fbType,
            RbAllocationType rbAllocationType, TxMode currentTxMode,
            int antennaCws, int numPreferredBands, int numRus,
            std::vector<double> snr, MacNodeId id = NODEID_NONE) override;

    LteFeedback computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
            RbAllocationType rbAllocationType,
            int antennaCws, int numPreferredBands, int numRus,
            std::vector<double> snr, MacNodeId id = NODEID_NONE) override;
};

} //namespace

#endif


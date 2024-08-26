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

#ifndef _LTE_LTEFEEDBACKCOMPUTATION_H_
#define _LTE_LTEFEEDBACKCOMPUTATION_H_

#include "stack/phy/feedback/LteFeedback.h"

namespace simu5g {

class LteFeedbackComputation
{
  public:
    virtual ~LteFeedbackComputation();
    /**
     * Interface for Feedback computation
     *
     * @param Feedback type (Wideband, all bands, etc.)
     * @param RbAllocationType Allocation type (distributed or localized)
     * @param Txmode current tx mode
     * @param number of codewords for each antenna
     * @param Number of preferred bands
     * @return Vector of Vector of LteFeedback indexes: Ru and Txmode
     */
    virtual LteFeedbackDoubleVector computeFeedback(FeedbackType fbType, RbAllocationType rbAllocationType,
            TxMode currentTxMode,
            std::map<Remote, int> antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype,
            int numRus, std::vector<double> snr, MacNodeId id = NODEID_NONE) = 0;
    /**
     * Interface for Feedback computation
     *
     * @param Remote antenna remote
     * @param Feedback type (Wideband, all bands, etc.)
     * @param RbAllocationType Allocation type (distributed or localized)
     * @param Txmode current tx mode
     * @param number of codewords for the selected antenna
     * @param Number of preferred bands
     * @return Vector of LteFeedback indexes: Txmode
     */
    virtual LteFeedbackVector computeFeedback(const Remote remote, FeedbackType fbType,
            RbAllocationType rbAllocationType, TxMode currentTxMode,
            int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
            std::vector<double> snr, MacNodeId id = NODEID_NONE) = 0;
    /**
     * Interface for Feedback computation
     *
     * @param Remote antenna remote
     * @param Selected Txmode tx mode
     * @param Remote antenna remote
     * @param Feedback type (Wideband, all bands, etc.)
     * @param RbAllocationType Allocation type (distributed or localized)
     * @param number of codewords for the selected antenna
     * @param Number of preferred bands
     * @return  LteFeedback
     */
    virtual LteFeedback computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
            RbAllocationType rbAllocationType,
            int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
            std::vector<double> snr, MacNodeId id = NODEID_NONE) = 0;
};

} //namespace

#endif


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

#ifndef STACK_PHY_FEEDBACK_LTESUMMARYFEEDBACK_H_
#define STACK_PHY_FEEDBACK_LTESUMMARYFEEDBACK_H_

#include "stack/phy/feedback/LteSummaryFeedback.h"

namespace simu5g {

using namespace omnetpp;
class LteSummaryFeedback
{
    //! confidence function lower bound
    simtime_t confidenceLowerBound_;
    //! confidence function upper bound
    simtime_t confidenceUpperBound_;

  protected:

    //! Maximum number of codewords.
    unsigned char totCodewords_;
    //! Number of logical bands.
    unsigned int logicalBandsTot_;

    //! Rank Indication.
    Rank ri_;
    //! Channel Quality Indicator (per-codeword).
    std::vector<CqiVector> cqi_;
    //! Precoding Matrix Index.
    PmiVector pmi_;

    //! time elapsed from last refresh of RI.
    simtime_t tRi_;
    //! time elapsed from last refresh of CQI.
    std::vector<std::vector<simtime_t>> tCqi_;
    //! time elapsed from last refresh of PMI.
    std::vector<simtime_t> tPmi_;
    // valid flag
    bool valid_;

    /** Calculate the confidence factor.
     *  @param n the feedback age in TTI
     */
    double confidence(simtime_t creationTime) const;

  public:

    //! Create an empty feedback message.
    LteSummaryFeedback(unsigned char cw, unsigned int b, simtime_t lb, simtime_t ub) : confidenceLowerBound_(lb), confidenceUpperBound_(ub), totCodewords_(cw), logicalBandsTot_(b)
    {
        reset();
    }

    //! Reset this summary feedback.
    void reset();

    /*************
    *  Setters
    *************/

    //! Set the RI.
    void setRi(Rank ri)
    {
        ri_ = ri;
        tRi_ = simTime();
        // set CW
        totCodewords_ = ri > 1 ? 2 : 1;
    }

    //! Set single-codeword/single-band CQI.
    void setCqi(Cqi cqi, Codeword cw, Band band)
    {
        // note: it is impossible to receive CQI == 0!
        cqi_[cw][band] = cqi;
        tCqi_[cw][band] = simTime();
        valid_ = true;
    }

    //! Set single-band PMI.
    void setPmi(Pmi pmi, Band band)
    {
        pmi_[band] = pmi;
        tPmi_[band] = simTime();
    }

    /*************
    *  Getters
    *************/

    //! Get the number of codewords.
    unsigned char getTotCodewords() const
    {
        return totCodewords_;
    }

    //! Get the number of logical bands.
    unsigned char getTotLogicalBands() const
    {
        return logicalBandsTot_;
    }

    //! Get the RI.
    Rank getRi() const
    {
        return ri_;
    }

    //! Get the RI confidence value.
    double getRiConfidence() const
    {
        return confidence(tRi_);
    }

    //! Get single-codeword/single-band CQI.
    Cqi getCqi(Codeword cw, Band band) const
    {
        return cqi_.at(cw).at(band);
    }

    //! Get single-codeword CQI vector.
    const CqiVector& getCqi(Codeword cw) const
    {
        return cqi_.at(cw);
    }

    //! Get single-codeword/single-band CQI confidence value.
    double getCqiConfidence(Codeword cw, Band band) const
    {
        return confidence(tCqi_.at(cw).at(band));
    }

    //! Get single-band PMI.
    Pmi getPmi(Band band) const
    {
        return pmi_.at(band);
    }

    //! Get the PMI vector.
    const PmiVector& getPmi() const
    {
        return pmi_;
    }

    //! Get single-band PMI confidence value.
    double getPmiConfidence(Band band) const
    {
        return confidence(tPmi_.at(band));
    }

    bool isValid()
    {
        return valid_;
    }

    /** Print debug information about the LteSummaryFeedback instance.
     *  @param cellId The cell ID.
     *  @param nodeId The node ID.
     *  @param dir The link direction.
     *  @param txm The transmission mode.
     *  @param s The name of the function that requested the debug.
     */
    void print(MacCellId cellId, MacNodeId nodeId, const Direction dir, TxMode txm, const char *s) const;
};

} //namespace

#endif /* STACK_PHY_FEEDBACK_LTESUMMARYFEEDBACK_H_ */


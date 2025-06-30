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

#ifndef STACK_PHY_FEEDBACK_LTESUMMARYBUFFER_H_
#define STACK_PHY_FEEDBACK_LTESUMMARYBUFFER_H_

#include <list>
#include "stack/phy/feedback/LteSummaryBuffer.h"
#include "stack/phy/feedback/LteFeedback.h"

namespace simu5g {

using namespace omnetpp;

class LteSummaryBuffer
{
  protected:
    //! Buffer size
    unsigned char bufferSize_;
    //! The buffer
    std::list<LteFeedback> buffer_;
    //! Number of codewords.
    double totCodewords_;
    //! Number of bands.
    double totBands_;
    //! Cumulative summary feedback.
    LteSummaryFeedback cumulativeSummary_;
    void createSummary(LteFeedback fb);

  public:

    LteSummaryBuffer(unsigned char dim, unsigned char cw, unsigned int b, simtime_t lb, simtime_t ub) :
        bufferSize_(dim), totCodewords_(cw), totBands_(b), cumulativeSummary_(cw, b, lb, ub)
    {}

    //! Put a feedback into the buffer and update current summary feedback
    void put(LteFeedback fb)
    {
        if (bufferSize_ > 0) {
            buffer_.push_back(fb);
        }
        if (buffer_.size() > bufferSize_) {
            buffer_.pop_front();
        }
        createSummary(fb);
    }

    //! Get the current summary feedback
    const LteSummaryFeedback& get() const
    {
        return cumulativeSummary_;
    }

};

} //namespace

#endif /* STACK_PHY_FEEDBACK_LTESUMMARYBUFFER_H_ */


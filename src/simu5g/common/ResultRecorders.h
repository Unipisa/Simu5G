//
//                  Simu5G
//
// Authors: Andras Varga
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMU5G_RESULTRECORDERS_H_
#define __SIMU5G_RESULTRECORDERS_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace simu5g {

// NOTE: This class is a copy of the similarly named result recorder in OMNeT++.
// It was added here because it will only appear in OMNeT++ 6.4, which is not
// yet released at the time of writing (November 2025).

/**
 * @brief Listener for recording the time-weighted average of signal values,
 * where each value represents the rate/throughput that was active during the
 * interval *preceding* its emission time. NaN values in the input denote
 * intervals to be ignored.
 *
 * This recorder uses backward-sample-hold interpolation, meaning when a value V
 * is emitted at time T, it represents the rate that was constant during the
 * interval from the previous emission time to T.
 *
 * Typical use cases include recording average throughput, bitrate, or packet
 * rate from measurements taken at the end of transmission intervals.
 *
 * @see TimeAverageRecorder
 */
class RateAverageRecorder : public cNumericResultRecorder
{
    protected:
        simtime_t lastTime = SIMTIME_ZERO;
        double weightedSum = 0;
        simtime_t totalTime = SIMTIME_ZERO;
    protected:
        virtual void collect(simtime_t_cref t, double value, cObject *details) override;
        virtual void finish(cResultFilter *prev) override;
    public:
        RateAverageRecorder() {}
        double getRateAverage() const;
        virtual std::string str() const override;
};

} // namespace simu5g

#endif

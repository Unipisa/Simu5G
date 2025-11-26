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

#include "simu5g/common/ResultFilters.h"
#include <sstream>

namespace simu5g {

Register_ResultFilter2("simu5g_rateavg", RateAverageFilter,
        "Produces the time-weighted average of signal values, where each emitted value "
        "represents the rate that was active during the interval preceding its emission (backward-sample-hold). "
        "NaN values in the input denote intervals to be ignored. "
);

bool RateAverageFilter::process(simtime_t& t, double& value, cObject *details)
{
    if (!std::isnan(value)) {
        totalTime += t - lastTime;
        weightedSum += value * SIMTIME_DBL(t - lastTime);
    }
    lastTime = t;
    value = weightedSum / totalTime;
    return !std::isnan(value);
}

double RateAverageFilter::getRateAverage() const
{
    // Note: The current interval (lastTime,now) cannot be taken into account,
    // because we don't know the value yet (it will be emitted at the end of the interval).
    return weightedSum / totalTime;
}

std::string RateAverageFilter::str() const
{
    std::stringstream os;
    os << "rateAverage = " << getRateAverage();
    return os.str();
}

} // namespace simu5g

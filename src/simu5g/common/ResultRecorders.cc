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

#include "simu5g/common/ResultRecorders.h"
#include <sstream>

namespace simu5g {

Register_ResultRecorder2("simu5g_rateavg", RateAverageRecorder,
        "Records the time-weighted average of signal values, where each emitted value "
        "represents the rate that was active during the interval preceding its emission (backward-sample-hold). "
        "NaN values in the input denote intervals to be ignored. "
);

void RateAverageRecorder::collect(simtime_t_cref t, double value, cObject *details)
{
    if (!std::isnan(value)) {
        totalTime += t - lastTime;
        weightedSum += value * SIMTIME_DBL(t - lastTime);
    }
    lastTime = t;
}

double RateAverageRecorder::getRateAverage() const
{
    // Note: The current interval (lastTime,now) cannot be taken into account,
    // because we don't know the value yet (it will be emitted at the end of the interval).
    return weightedSum / totalTime;
}

void RateAverageRecorder::finish(cResultFilter *prev)
{
    opp_string_map attributes = getStatisticAttributes();
    getEnvir()->recordScalar(getComponent(), getResultName().c_str(), getRateAverage(), &attributes);
}

std::string RateAverageRecorder::str() const
{
    std::stringstream os;
    os << getResultName() << " = " << getRateAverage();
    return os.str();
}

} // namespace simu5g

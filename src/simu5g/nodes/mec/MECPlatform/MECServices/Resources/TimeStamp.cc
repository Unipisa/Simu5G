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
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

#include "time.h"

namespace simu5g {

using namespace omnetpp;

TimeStamp::TimeStamp() : nanoSeconds_(0), valid_(true)
{
    seconds_ = simTime().dbl();
}

TimeStamp::TimeStamp(bool valid) : nanoSeconds_(0), valid_(valid)
{
    seconds_ = simTime().dbl();
}


bool TimeStamp::isValid() const
{
    return valid_;
}

nlohmann::ordered_json TimeStamp::toJson() const
{
    nlohmann::ordered_json val;

    val["seconds"] = seconds_;
    val["nanoSeconds"] = nanoSeconds_;

    return val;
}

int32_t TimeStamp::getSeconds() const
{
    return seconds_;
}

void TimeStamp::setSeconds(int32_t value)
{
    seconds_ = value;
}

void TimeStamp::setSeconds() // TODO rename
{
    seconds_ = simTime().dbl();
}

int32_t TimeStamp::getNanoSeconds() const
{
    return nanoSeconds_;
}

void TimeStamp::setNanoSeconds(int32_t value)
{
    nanoSeconds_ = value;
}

void TimeStamp::setValid(bool valid)
{
    valid_ = valid;
}

} //namespace


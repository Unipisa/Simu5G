/**

*/


#include "nodes/mec/MEPlatform/MeServices/Resources/TimeStamp.h"

#include "time.h"

TimeStamp::TimeStamp()
{
    seconds_ = omnetpp::simTime().dbl();
    nanoSeconds_ = 0;
    valid_ = true;
}

TimeStamp::TimeStamp(bool valid)
{
    seconds_ = seconds_ = omnetpp::simTime().dbl();
    nanoSeconds_ = 0;
    valid_ = valid;
}

TimeStamp::~TimeStamp(){}

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
void TimeStamp::setSeconds()
{
    seconds_ =  seconds_ = omnetpp::simTime().dbl();
    
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



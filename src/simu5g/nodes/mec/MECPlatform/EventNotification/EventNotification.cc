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

#include "nodes/mec/MECPlatform/EventNotification/EventNotification.h"

namespace simu5g {

EventNotification::EventNotification()
{
}

EventNotification::EventNotification(const std::string& type, int subId) : type_(type), subId_(subId)
{
}

int EventNotification::getSubId() const
{
    return subId_;
}

std::string EventNotification::getType() const
{
    return type_;
}

// getters
void EventNotification::setSubId(int subId)
{
    subId_ = subId;
}

void EventNotification::setType(const std::string& type)
{
    type_ = type;
}


} //namespace


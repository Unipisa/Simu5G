/*
 * EventNotification.cc
 *
 *  Created on: Mar 23, 2021
 *      Author: linofex
 */

#include "nodes/mec/MECPlatform/EventNotification/EventNotification.h"

EventNotification::EventNotification() {
    // TODO Auto-generated constructor stub

}

EventNotification::EventNotification(const std::string& type, const int& subId)
{
    type_ = type;
    subId_ = subId;
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




EventNotification::~EventNotification() {
    // TODO Auto-generated destructor stub
}


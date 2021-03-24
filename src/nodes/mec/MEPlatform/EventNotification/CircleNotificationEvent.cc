/*
 * CircleNotificationEvent.cc
 *
 *  Created on: Mar 23, 2021
 *      Author: linofex
 */

#include "CircleNotificationEvent.h"

CircleNotificationEvent::CircleNotificationEvent() {
    // TODO Auto-generated constructor stub

}

CircleNotificationEvent::CircleNotificationEvent(const std::string& type, const int& subId, const std::vector<TerminalLocation>& terminalLocations)
        : EventNotification(type, subId), terminalLocations_(terminalLocations){}

const std::vector<TerminalLocation>& CircleNotificationEvent::getTerminalLocations() const
{
    return terminalLocations_;
}


CircleNotificationEvent::~CircleNotificationEvent() {
    // TODO Auto-generated destructor stub
}


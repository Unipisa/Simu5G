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

#include "CircleNotificationEvent.h"

namespace simu5g {

CircleNotificationEvent::CircleNotificationEvent()
{
}

CircleNotificationEvent::CircleNotificationEvent(const std::string& type, const int& subId, const std::vector<TerminalLocation>& terminalLocations)
    : EventNotification(type, subId), terminalLocations_(terminalLocations)
{
}

const std::vector<TerminalLocation>& CircleNotificationEvent::getTerminalLocations() const
{
    return terminalLocations_;
}


} //namespace


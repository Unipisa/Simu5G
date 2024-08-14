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
#ifndef NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_CIRCLENOTIFICATIONEVENT_H_
#define NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_CIRCLENOTIFICATIONEVENT_H_

#include "EventNotification.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/TerminalLocation.h"

namespace simu5g {

class CircleNotificationEvent : public EventNotification
{
  public:
    CircleNotificationEvent();
    CircleNotificationEvent(const std::string& type, const int& subId, const std::vector<TerminalLocation>& terminalLocations);

    const std::vector<TerminalLocation>& getTerminalLocations() const;


  private:
    std::vector<TerminalLocation> terminalLocations_;

};

} //namespace

#endif /* NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_CIRCLENOTIFICATIONEVENT_H_ */


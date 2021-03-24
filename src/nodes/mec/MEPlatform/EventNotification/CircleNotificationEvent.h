/*
 * CircleNotificationEvent.h
 *
 *  Created on: Mar 23, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_CIRCLENOTIFICATIONEVENT_H_
#define NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_CIRCLENOTIFICATIONEVENT_H_

#include "EventNotification.h"
#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/TerminalLocation.h"


class CircleNotificationEvent: public EventNotification {
    public:
        CircleNotificationEvent();
        CircleNotificationEvent(const std::string& type, const int& subId, const std::vector<TerminalLocation>& terminalLocations);

        const std::vector<TerminalLocation>& getTerminalLocations() const;


        virtual ~CircleNotificationEvent();

    private:
        std::vector<TerminalLocation> terminalLocations_;

};

#endif /* NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_CIRCLENOTIFICATIONEVENT_H_ */

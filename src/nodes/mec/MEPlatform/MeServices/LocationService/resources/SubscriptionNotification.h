/*
 * SubscriptionNotification.h
 *
 *  Created on: Dec 28, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_SUBSCRIPTIONNOTIFICATION_H_
#define APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_SUBSCRIPTIONNOTIFICATION_H_

#include "corenetwork/nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "corenetwork/nodes/mec/MEPlatform/MeServices/LocationService/resources/LocationApiDefs.h"

/*
 *  From RESTful Network APIforTerminal Location
 *  section 5.2.2.3
 *
 * attributes           optional
 * callbackData           yes
 * terminalLocation       no
 * enterLeavingCriteria   yes
 * distanceCriteria       yes
 * isFinalNotification    yes
 * link                   yes
 *
 */
class SubscriptioNotification : public AttributeBase
{
    protected:
        std::string callbackData;
        bool isValidCallbackData;

        EnteringLeavingCriteria elCriteria;
        bool isValidElCriteria;

        DistanceCriteria dCriteria;
        bool isValidDCriteria;

        bool isFinalNotification;
        bool isValidIsFinalNotification;

        std::vector<std::string> links;
        //check done by looking at empty() method


        std::vector<TerminalLocation> ues;
        //check done by looking at empty() method



};



#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_SUBSCRIPTIONNOTIFICATION_H_ */

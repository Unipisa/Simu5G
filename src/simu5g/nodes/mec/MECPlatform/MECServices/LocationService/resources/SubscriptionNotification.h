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

#ifndef APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_SUBSCRIPTIONNOTIFICATION_H_
#define APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_SUBSCRIPTIONNOTIFICATION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationApiDefs.h"

namespace simu5g {

/*
 *  From RESTful Network API for Terminal Location
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
class SubscriptionNotification : public AttributeBase
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

    std::vector<std::string> links;  // Check done by looking at empty() method

    std::vector<TerminalLocation> ues;  // Check done by looking at empty() method

};

} //namespace

#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_SUBSCRIPTIONNOTIFICATION_H_ */


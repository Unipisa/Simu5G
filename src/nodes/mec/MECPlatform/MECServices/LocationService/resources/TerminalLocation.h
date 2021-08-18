/*
 * TerminalLocation.h
 *
 *  Created on: Dec 28, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MECSERVICES_LOCATIONSERVICE_RESOURCES_TERMINALLOCATION_H_
#define APPS_MEC_MECSERVICES_LOCATIONSERVICE_RESOURCES_TERMINALLOCATION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/CurrentLocation.h"


/*
 *  From RESTful Network APIforTerminal Location
 *  section 5.2.2.1
 *
 * attributes           optional
 * address                 no
 * locationRetreivalStatus no
 * currentLocation         yes
 * errorInformation        yes
 *
 */


class TerminalLocation : public AttributeBase
{
    protected:
        std::string address;
        std::string locationRetreivalStatus; // Retrieved or error
        CurrentLocation currentLocation;
        std::string errorInformation;

    public:
        TerminalLocation();
        TerminalLocation(const std::string& address, const std::string& locationRetreivalStatus, const CurrentLocation& currentLocation);
        ~TerminalLocation();
        nlohmann::ordered_json toJson() const;
};


#endif /* APPS_MEC_MECSERVICES_LOCATIONSERVICE_RESOURCES_TERMINALLOCATION_H_ */

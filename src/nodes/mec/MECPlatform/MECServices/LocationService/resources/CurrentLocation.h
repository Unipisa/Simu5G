/*
 * CurrentLocation.h
 *
 *  Created on: Dec 28, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CURRENTLOCATION_H_
#define APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CURRENTLOCATION_H_



#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

class CurrentLocation : public AttributeBase
{
    protected:
        double accuracy;
        inet::Coord coords;
        TimeStamp timeStamp;

    public:
        CurrentLocation();
        CurrentLocation(double accuracy, const inet::Coord& coords, const TimeStamp& ts);
        CurrentLocation(double accuracy, const inet::Coord& coords);
        ~CurrentLocation();

        nlohmann::ordered_json toJson() const;
};



#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CURRENTLOCATION_H_ */

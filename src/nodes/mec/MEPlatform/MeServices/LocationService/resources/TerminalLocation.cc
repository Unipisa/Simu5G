/*
 * TerminalLocation.cc
 *
 *  Created on: Dec 28, 2020
 *      Author: linofex
 */


#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/TerminalLocation.h"


TerminalLocation::TerminalLocation(){};
TerminalLocation::TerminalLocation(const std::string& address, const std::string& locationRetreivalStatus, const CurrentLocation& currentLocation):
        currentLocation(currentLocation)
{
    this->address = address;
    this->locationRetreivalStatus = locationRetreivalStatus;
    }
TerminalLocation::~TerminalLocation(){}
nlohmann::ordered_json TerminalLocation::toJson() const
{
    nlohmann::ordered_json val ;

    val["address"] = address;
    val["currentLocation"] = currentLocation.toJson();
    val["locationRetreivalStatus"] = locationRetreivalStatus;
    return val;
}




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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/TerminalLocation.h"

namespace simu5g {

TerminalLocation::TerminalLocation() {};
TerminalLocation::TerminalLocation(const std::string& address, const std::string& locationRetrievalStatus, const CurrentLocation& currentLocation):
    address(address), locationRetrievalStatus(locationRetrievalStatus), currentLocation(currentLocation)
{
}

nlohmann::ordered_json TerminalLocation::toJson() const
{
    nlohmann::ordered_json val;

    val["address"] = address;
    val["currentLocation"] = currentLocation.toJson();
    val["locationRetrievalStatus"] = locationRetrievalStatus;
    return val;
}

} //namespace


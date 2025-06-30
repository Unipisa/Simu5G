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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/CurrentLocation.h"

namespace simu5g {

CurrentLocation::CurrentLocation() {};
CurrentLocation::CurrentLocation(double accuracy, const inet::Coord& coords, const TimeStamp& ts): accuracy(accuracy), coords(coords), timeStamp(ts)
{
}

CurrentLocation::CurrentLocation(double accuracy, const inet::Coord& coords): accuracy(accuracy), coords(coords)
{
    timeStamp.setSeconds();
}


nlohmann::ordered_json CurrentLocation::toJson() const
{
    nlohmann::ordered_json val;

    val["accuracy"] = accuracy;
    val["x"] = coords.x;
    val["y"] = coords.y;
    val["z"] = coords.z;
    val["timeStamp"] = timeStamp.toJson();

    return val;
}

} //namespace


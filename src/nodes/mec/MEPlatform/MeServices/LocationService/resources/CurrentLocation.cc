/*
 * CurrentLocation.cc
 *
 *  Created on: Dec 28, 2020
 *      Author: linofex
 */

#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/CurrentLocation.h"


CurrentLocation::CurrentLocation(){};
CurrentLocation::CurrentLocation(double accuracy, const inet::Coord& coords, const TimeStamp& ts): coords(coords), timeStamp(ts)
{
    this->accuracy = accuracy;
}

CurrentLocation::CurrentLocation(double accuracy, const inet::Coord& coords): coords(coords)
{
    this->accuracy = accuracy;
    timeStamp.setSeconds();
}
CurrentLocation::~CurrentLocation(){}

nlohmann::ordered_json CurrentLocation::toJson() const
{
    nlohmann::ordered_json val ;

    val["accuracy"] = accuracy;
    val["x"] = coords.x;
    val["y"] = coords.y;
    val["z"] = coords.z;
    val["timeStamp"] = timeStamp.toJson();

    return val;

}





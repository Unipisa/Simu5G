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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationInfo.h"

namespace simu5g {

LocationInfo::LocationInfo() : coordinates_(inet::Coord::NIL), speed_(inet::Coord::NIL)
{
}

LocationInfo::LocationInfo(const inet::Coord& coordinates, const inet::Coord& speed) : coordinates_(coordinates), speed_(speed)
{
}

LocationInfo::LocationInfo(const inet::Coord& coordinates) : coordinates_(coordinates), speed_(inet::Coord::NIL)
{
}


nlohmann::ordered_json LocationInfo::toJson() const
{

    nlohmann::ordered_json val;

    /*
     * According to the Location API standard (ETSI GS MEC 013 V2.1.1), the position
     * is expressed with latitude and longitude angles and altitude (WGS84 compliant)
     *
     * For simplicity, in OMNeT++ the position is expressed in Cartesian Coordinates
     */
    val["x"] = coordinates_.x;
    val["y"] = coordinates_.y;
    val["z"] = coordinates_.z;
    val["shape"] = 2;
    if (!speed_.isUnspecified()) {
        inet::Coord North(0, 1, 0);
        inet::Coord temp(speed_); // this method is const and angle() does not.
        double angle = temp.angle(North);

        val["velocity"]["velocityType"] = 1;
        val["velocity"]["bearing"] = (speed_.x >= 0 ? angle : 360 - angle);
        val["velocity"]["horizontalSpeed"] = speed_.length();
    }
    return val;
}

} //namespace


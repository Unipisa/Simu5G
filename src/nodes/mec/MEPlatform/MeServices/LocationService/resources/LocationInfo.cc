/*
 * LocationInfo.cc
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */





#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/LocationInfo.h"

LocationInfo::LocationInfo()
{
    coordinates_ = inet::Coord(0.,0.,0.);
    speed_ = inet::Coord(0.,0.,0.);

}
LocationInfo::LocationInfo(const inet::Coord& coordinates, const inet::Coord& speed)
{
    coordinates_ = coordinates;
    speed_ = speed;
}
LocationInfo::~LocationInfo(){}

nlohmann::ordered_json LocationInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["x"] = coordinates_.x;
    val["y"] = coordinates_.y;
    val["z"] = coordinates_.z;
    val["speed"]["x"] = speed_.x;
    val["speed"]["y"] = speed_.y;
    val["speed"]["z"] = speed_.z;

 return val;
}

/*
 * UserInfo.cc
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */



#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/UserInfo.h"

UserInfo::UserInfo():timestamp_(), locationInfo_(){}
UserInfo::UserInfo(const LocationInfo& location, const inet::Ipv4Address& address, MacCellId accessPointId, const std::string& resourceUrl, int zoneId):
        locationInfo_(location), timestamp_()
{
    address_ = address;
    accessPointId_ = accessPointId;
    resourceUrl_ = resourceUrl;
    zoneId_ = zoneId;
    timestamp_.setSeconds();
}
UserInfo::UserInfo(const inet::Coord& location, const inet::Coord& speed,  const inet::Ipv4Address& address, MacCellId accessPointId, const std::string& resourceUrl, int zoneId):
        locationInfo_(location, speed), timestamp_()
{
    address_ = address;
    accessPointId_ = accessPointId;
    resourceUrl_ = resourceUrl;
    zoneId_ = zoneId;
    timestamp_.setSeconds();
}


nlohmann::ordered_json UserInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["address"] = "acr:"+address_.str();
    val["accessPointId"] = accessPointId_;
    val["zoneId"] = zoneId_;
    val["resourceURL"] = resourceUrl_;
    val["timeStamp"] = timestamp_.toJson();
    //val["timeStamp"] = timestamp_.toJson();
    val["locationInfo"] = locationInfo_.toJson();
    return val;
}

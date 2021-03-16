/*
 * UserInfo.cc
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */



#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/UserInfo.h"

UserInfo::UserInfo():timestamp_(), locationInfo_(){}
UserInfo::UserInfo(const LocationInfo& location, const inet::Ipv4Address& address, MacCellId accessPointId, const std::string& resourceUrl, int zoneId):
        locationInfo_(location)
{
    address_ = address;
    accessPointId_ = accessPointId;
    resourceUrl_ = resourceUrl;
    zoneId_ = zoneId;
}
UserInfo::UserInfo(const inet::Coord& location, const inet::Coord& speed,  const inet::Ipv4Address& address, MacCellId accessPointId, const std::string& resourceUrl, int zoneId):
        locationInfo_(location, speed)
{
    address_ = address;
    accessPointId_ = accessPointId;
    resourceUrl_ = resourceUrl;
    zoneId_ = zoneId;
}


nlohmann::ordered_json UserInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["address"] = "acr:"+address_.str();
    val["accessPointId"] = accessPointId_;
    val["zoneId"] = zoneId_;
    val["resourceURL"] = resourceUrl_;
    val["timeStamp"] = omnetpp::simTime().dbl();
    //val["timeStamp"] = timestamp_.toJson();
    val["locationInfo"] = locationInfo_.toJson();
    return val;
}

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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/UserInfo.h"

namespace simu5g {

UserInfo::UserInfo():timestamp_(), locationInfo_() {}
UserInfo::UserInfo(const LocationInfo& location, const inet::Ipv4Address& address, MacCellId accessPointId, const std::string& resourceUrl, int zoneId):
    timestamp_(), address_(address), accessPointId_(accessPointId), zoneId_(zoneId), resourceUrl_(resourceUrl), locationInfo_(location)
{
    timestamp_.setSeconds();
}

UserInfo::UserInfo(const inet::Coord& location, const inet::Coord& speed, const inet::Ipv4Address& address, MacCellId accessPointId, const std::string& resourceUrl, int zoneId):
    timestamp_(), address_(address), accessPointId_(accessPointId), zoneId_(zoneId), resourceUrl_(resourceUrl), locationInfo_(location, speed)
{
    timestamp_.setSeconds();
}

nlohmann::ordered_json UserInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["address"] = "acr:" + address_.str();
    val["accessPointId"] = accessPointId_;
    val["zoneId"] = zoneId_;
    val["resourceURL"] = resourceUrl_;
    val["timeStamp"] = timestamp_.toJson();
    //val["timeStamp"] = timestamp_.toJson();
    val["locationInfo"] = locationInfo_.toJson();
    return val;
}

} //namespace


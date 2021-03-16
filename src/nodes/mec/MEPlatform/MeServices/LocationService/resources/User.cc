/*
 * User.cc
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */



#include "nodes/mec/MEPlatform/MeServices/LocationService/resources/User.h"

User::User():timestamp_(){}
User::User(const inet::Ipv4Address& address, const MacCellId accessPointId, const std::string& resourceUrl, int zoneId):timestamp_()
{
    address_ = address;
    accessPointId_ = accessPointId;
    resourceUrl_ = resourceUrl;
    zoneId_ = zoneId;
}



nlohmann::ordered_json User::toJson() const
{
    nlohmann::ordered_json val;
    val["address"] = "acr:"+address_.str();
    val["accessPointId"] = accessPointId_;
    val["zoneId"] = zoneId_;
    val["resourceURL"] = resourceUrl_;
    val["timeStamp"] = omnetpp::simTime().dbl();

    return val;
}

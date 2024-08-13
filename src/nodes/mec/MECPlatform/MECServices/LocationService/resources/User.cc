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

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/User.h"

namespace simu5g {

using namespace omnetpp;

User::User():timestamp_() {}
User::User(const inet::Ipv4Address& address, const MacCellId accessPointId, const std::string& resourceUrl, int zoneId):timestamp_(), address_(address), zoneId_(zoneId), accessPointId_(accessPointId), resourceUrl_(resourceUrl)
{
}

nlohmann::ordered_json User::toJson() const
{
    nlohmann::ordered_json val;
    val["address"] = "acr:" + address_.str();
    val["accessPointId"] = accessPointId_;
    val["zoneId"] = zoneId_;
    val["resourceURL"] = resourceUrl_;
    val["timeStamp"] = simTime().dbl();

    return val;
}

} //namespace


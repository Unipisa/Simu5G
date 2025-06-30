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

#include "nodes/mec/MECPlatform/ServiceRegistry/resources/TransportInfo.h"

namespace simu5g {

TransportInfo::TransportInfo(const std::string& id, const std::string& name, const std::string& type, const std::string& protocol, const EndPointInfo& endPoint):
    id_(id), name_(name), type_(type), protocol_(protocol), endPoint_(endPoint)
{
}

nlohmann::ordered_json TransportInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["id"] = id_;
    val["name"] = name_;
    val["protocol"] = protocol_;
    val["endPoint"] = endPoint_.toJson();

    return val;
}

} //namespace


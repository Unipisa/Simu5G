/*
 * CategoryRef.cc
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */


#include "nodes/mec/MEPlatform/ServiceRegistry/resources/TransportInfo.h"


TransportInfo::TransportInfo(const std::string& id, const std::string& name, const std::string& type, const std::string& protocol, const EndPointInfo& endPoint):
                        endPoint_(endPoint)
{
    id_ = id;
    name_ = name;
    type_ = type;
    protocol_ = protocol;
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




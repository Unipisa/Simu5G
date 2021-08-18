//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//



#include "nodes/mec/MECPlatform/ServiceRegistry/resources/TransportInfo.h"


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




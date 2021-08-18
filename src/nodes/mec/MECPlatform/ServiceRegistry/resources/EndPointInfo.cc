//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//



#include "nodes/mec/MECPlatform/ServiceRegistry/resources/EndPointInfo.h"

EndPointInfo::EndPointInfo(const std::string& host, int port)
{
    host_ = host;
    port_ = port;
}

nlohmann::ordered_json EndPointInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["addresses"]["host"] = host_;
    val["addresses"]["port"] = port_;
    return val;
}




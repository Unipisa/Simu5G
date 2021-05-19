/*
 * CategoryRef.cc
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */


#include "nodes/mec/MEPlatform/ServiceRegistry/resources/EndPointInfo.h"

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




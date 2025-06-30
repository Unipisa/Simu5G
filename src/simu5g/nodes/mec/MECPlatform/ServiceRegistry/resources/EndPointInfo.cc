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

#include "nodes/mec/MECPlatform/ServiceRegistry/resources/EndPointInfo.h"

namespace simu5g {

EndPointInfo::EndPointInfo(const std::string& host, int port) : host_(host), port_(port)
{
}

nlohmann::ordered_json EndPointInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["addresses"]["host"] = host_;
    val["addresses"]["port"] = port_;
    return val;
}

} //namespace


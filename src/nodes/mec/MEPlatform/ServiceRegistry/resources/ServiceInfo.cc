/*
 * ServiceInfo.cc
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */


#include "nodes/mec/MEPlatform/ServiceRegistry/resources/ServiceInfo.h"


ServiceInfo::ServiceInfo(const std::string& serInstanceId, const std::string& serName, const CategoryRef& serCat, const std::string& version,
                const std::string& state, const TransportInfo& tInfo, const std::string& serializer, const std::string& mecHost, const std::string& sol, bool clo, bool local):
                serCategory_(serCat), transportInfo_(tInfo)
{
    serInstanceId_ = serInstanceId;
    serName_ = serName;
    serCategory_ = serCat;
    version_ = version;
    state_ = state;
    serializer_ = serializer;
    scopeOfLocality_ = sol;
    consumedLocalOnly_ = clo;
    isLocal_ = local;
    mecHost_ = mecHost;
}

nlohmann::ordered_json ServiceInfo::toJson() const
{
    nlohmann::ordered_json val;
    val["serInstanceId"] = serInstanceId_;
    val["serName"] = serName_;
    val["serCategory"] = serCategory_.toJson();
    val["version"] = version_;
    val["state"] = state_;
    val["transportInfo"] = transportInfo_.toJson();
    val["serializer"] = serializer_;
    val["scopeOfLocality"] = scopeOfLocality_ ;
    val["consumedLocalOnly"] = (consumedLocalOnly_ == true)? "TRUE":"FALSE";
    val["isLocal"] = (isLocal_== true)? "TRUE":"FALSE";

    return val;
}



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

#include "nodes/mec/MECPlatform/ServiceRegistry/resources/ServiceInfo.h"

namespace simu5g {

ServiceInfo::ServiceInfo(const std::string& serInstanceId, const std::string& serName, const CategoryRef& serCat, const std::string& version,
        const std::string& state, const TransportInfo& tInfo, const std::string& serializer, const std::string& mecHost, const std::string& sol, bool clo, bool local):
    serInstanceId_(serInstanceId), serName_(serName), mecHost_(mecHost), serCategory_(serCat), version_(version), state_(state), transportInfo_(tInfo), serializer_(serializer), scopeOfLocality_(sol), consumedLocalOnly_(clo), isLocal_(local)
{
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
    val["scopeOfLocality"] = scopeOfLocality_;
    val["consumedLocalOnly"] = consumedLocalOnly_ ? "TRUE" : "FALSE";
    val["isLocal"] = isLocal_ ? "TRUE" : "FALSE";

    return val;
}

} //namespace


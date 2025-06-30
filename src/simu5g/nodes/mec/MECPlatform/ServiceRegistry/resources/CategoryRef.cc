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

#include "nodes/mec/MECPlatform/ServiceRegistry/resources/CategoryRef.h"

namespace simu5g {

CategoryRef::CategoryRef(const std::string& href, const std::string& id, const std::string& name, const std::string& version) : href_(href), id_(id), name_(name), version_(version)
{
}

nlohmann::ordered_json CategoryRef::toJson() const
{
    nlohmann::ordered_json val;
    val["href"] = href_;
    val["id"] = id_;
    val["name"] = name_;
    val["version"] = version_;

    return val;
}

} //namespace


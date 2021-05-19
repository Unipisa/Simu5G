/*
 * CategoryRef.cc
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */


#include "nodes/mec/MEPlatform/ServiceRegistry/resources/CategoryRef.h"

CategoryRef::CategoryRef(const std::string& href, const std::string& id, const std::string& name, const std::string& version)
{
    href_  = href;
    id_   = id;
    name_ = name;
    version_  = version;
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




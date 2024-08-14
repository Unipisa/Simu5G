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

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

namespace simu5g {



std::string AttributeBase::toJson(const std::string& value)
{
    return value;
}

std::string AttributeBase::toJson(const std::time_t& value)
{
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&value));
    return buf;
}

int32_t AttributeBase::toJson(int32_t value)
{
    return value;
}

int64_t AttributeBase::toJson(int64_t value)
{
    return value;
}

double AttributeBase::toJson(double value)
{
    return value;
}

bool AttributeBase::toJson(bool value)
{
    return value;
}

nlohmann::ordered_json AttributeBase::toJson(AttributeBase& content)
{
    return content.toJson();
}

} //namespace


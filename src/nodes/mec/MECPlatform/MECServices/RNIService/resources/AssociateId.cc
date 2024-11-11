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

#include "AssociateId.h"

#include <inet/networklayer/contract/ipv4/Ipv4Address.h>

#include "common/binder/Binder.h"

namespace simu5g {

AssociateId::AssociateId() : type_(""), value_("")
{
}

AssociateId::AssociateId(const std::string& type, const std::string& value)
{
    setType(type);
    setValue(value);
}

AssociateId::AssociateId(mec::AssociateId& associateId)
{
    setType(associateId.type);
    setValue(associateId.value);
}


nlohmann::ordered_json AssociateId::toJson() const
{
    nlohmann::ordered_json val;

    val["type"] = type_;
    val["value"] = value_;
    return val;
}

void AssociateId::setAssociateId(const mec::AssociateId& associateId)
{
    type_ = associateId.type;
    value_ = associateId.value;
}

std::string AssociateId::getType() const
{
    return type_;
}

void AssociateId::setType(std::string value)
{
    type_ = value;
}

std::string AssociateId::getValue() const
{
    return value_;
}

void AssociateId::setValue(std::string value)
{
    value_ = value;
}

} //namespace


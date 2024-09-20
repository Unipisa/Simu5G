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

#ifndef NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_CATEGORYREF_H_
#define NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_CATEGORYREF_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

namespace simu5g {

class CategoryRef : public AttributeBase
{
  protected:
    std::string href_;
    std::string id_;
    std::string name_;
    std::string version_;

  public:
    CategoryRef() {}
    CategoryRef(const std::string& href, const std::string& id, const std::string& name, const std::string& version);
    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif /* NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_CATEGORYREF_H_ */


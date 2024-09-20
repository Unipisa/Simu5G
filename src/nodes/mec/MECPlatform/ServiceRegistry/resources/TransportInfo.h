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

#ifndef NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_TRANSINFO_H_
#define NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_TRANSINFO_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/resources/EndPointInfo.h"

namespace simu5g {

class TransportInfo : public AttributeBase
{
  protected:
    std::string id_;
    std::string name_;
    std::string type_;
    std::string protocol_;
    EndPointInfo endPoint_;

  public:
    TransportInfo() {}
    TransportInfo(const std::string& id, const std::string& name, const std::string& type, const std::string& protocol, const EndPointInfo& endPoint);
    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif /* NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_TRANSINFO_H_ */


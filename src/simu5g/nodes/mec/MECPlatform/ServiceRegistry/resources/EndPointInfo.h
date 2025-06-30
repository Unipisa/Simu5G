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

#ifndef NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_
#define NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

namespace simu5g {

class EndPointInfo : public AttributeBase
{
  protected:
    std::string host_;
    int port_;

  public:
    EndPointInfo() {};
    EndPointInfo(const std::string& host, int port);
    nlohmann::ordered_json toJson() const override;
};

} //namespace

#endif /* NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_ */


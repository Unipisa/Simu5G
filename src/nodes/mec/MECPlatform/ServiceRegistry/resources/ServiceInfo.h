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

#ifndef NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_SERVICEINFO_H_
#define NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_SERVICEINFO_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/resources/CategoryRef.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/resources/TransportInfo.h"

#include <omnetpp.h>
#include <string>

namespace simu5g {

/*
 * According to ETSI GS MEC 011 V2.1.1
 */

class ServiceInfo : public AttributeBase
{
  public:
    ServiceInfo() {}
    ServiceInfo(const std::string& serInstanceId, const std::string& serName, const CategoryRef& serCat, const std::string& version,
            const std::string& state, const TransportInfo& tInfo, const std::string& serializer, const std::string& mecHost, const std::string& sol, bool clo, bool local);

    nlohmann::ordered_json toJson() const override;

    const std::string& getName() const { return serName_; }
    const std::string& getInstanceId() const { return serInstanceId_; }
    const std::string& getMecHost() const { return mecHost_; }

    // getters

  private:
    std::string serInstanceId_; // Identifier of the service instance assigned by the MEPM/MEC platform.
                                // For uniqueness, UUID format is recommended.
    std::string serName_;
    std::string mecHost_;
    CategoryRef serCategory_;
    std::string version_;
    std::string state_;
    TransportInfo transportInfo_;
    std::string serializer_;
    std::string scopeOfLocality_;
    bool consumedLocalOnly_;
    bool isLocal_;
};

} //namespace

#endif /* NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_SERVICEINFO_H_ */


/*
 * ServiceInfo.h
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_SERVICEINFO_H_
#define NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_SERVICEINFO_H_

#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "nodes/mec/MEPlatform/ServiceRegistry/resources/CategoryRef.h"
#include "nodes/mec/MEPlatform/ServiceRegistry/resources/TransportInfo.h"


#include <omnetpp.h>
#include <string>

/*
 * According to ETSI GS MEC 011 V2.1.1
 */

class ServiceInfo : public AttributeBase
{
    public:
    ServiceInfo(){}
    ServiceInfo(const std::string& serInstanceId, const std::string& serName, const CategoryRef& serCat, const std::string& version,
                const std::string& state, const TransportInfo& tInfo, const std::string& serializer, const std::string& sol, bool clo, bool local);

    nlohmann::ordered_json toJson() const;

    // getters

    private:
        std::string serInstanceId_; // Identifier of the service instance assigned by the MEPM/MEC platform.
                                   // For the uniqueness UUID format is recommended.
        std::string serName_;
        CategoryRef serCategory_;
        std::string version_;
        std::string state_;
        TransportInfo transportInfo_;
        std::string serializer_;
        std::string scopeOfLocality_;
        bool consumedLocalOnly_;
        bool isLocal_;
};


#endif /* NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_SERVICEINFO_H_ */

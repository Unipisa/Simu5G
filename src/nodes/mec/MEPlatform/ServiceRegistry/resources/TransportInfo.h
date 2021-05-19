/*
 * TransportInfo.h
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_TRANSINFO_H_
#define NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_TRANSINFO_H_


#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "nodes/mec/MEPlatform/ServiceRegistry/resources/EndPointInfo.h"



class TransportInfo : public AttributeBase
{
    protected:
        std::string id_;
        std::string name_;
        std::string type_;
        std::string protocol_;
        EndPointInfo endPoint_;

    public:
        TransportInfo(){};
        TransportInfo(const std::string& id, const std::string& name, const std::string& type, const std::string& protocol, const EndPointInfo& endPoint);
        ~TransportInfo(){};
        nlohmann::ordered_json toJson() const;
};




#endif /* NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_TRANSINFO_H_ */

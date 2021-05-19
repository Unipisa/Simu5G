/*
 * EndPointInfo.h
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_
#define NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_


#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"


class EndPointInfo : public AttributeBase
{
    protected:
        std::string host_;
        int port_;


    public:
        EndPointInfo(){};
        EndPointInfo(const std::string& host, int port);
        ~EndPointInfo(){};
        nlohmann::ordered_json toJson() const;
};




#endif /* NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_ */

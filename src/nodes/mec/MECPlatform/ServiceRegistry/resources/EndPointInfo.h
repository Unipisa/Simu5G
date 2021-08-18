//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


#ifndef NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_
#define NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_


#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"


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




#endif /* NODES_MEC_MECPLATFORM_SERVICEREGISTRY_RESOURCES_ENDPOINTINFO_H_ */

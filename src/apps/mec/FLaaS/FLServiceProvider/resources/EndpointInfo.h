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


#ifndef APPS_MEC_FLAAS_FLSERVICEPROVIDER_RESOURCES_ENDPOINTINFO_H_
#define APPS_MEC_FLAAS_FLSERVICEPROVIDER_RESOURCES_ENDPOINTINFO_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "apps/mec/FLaaS/FLServiceProvider/FLService.h"

class EndpointInfo : public AttributeBase
{
    public:
        EndpointInfo();
        EndpointInfo(FLService* flService);

        virtual ~EndpointInfo();
        nlohmann::ordered_json toJson() const override;

    private:
        FLService* flService_;
};

#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_RESOURCES_ENDPOINTINFO_H_ */

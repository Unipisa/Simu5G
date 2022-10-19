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

#ifndef APPS_MEC_FLAAS_FLSERVICEPROVIDER_RESOURCES_FLSERVICEINFO_H_
#define APPS_MEC_FLAAS_FLSERVICEPROVIDER_RESOURCES_FLSERVICEINFO_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "apps/mec/FLaaS/FLServiceProvider/FLService.h"


class FLServiceInfo : public AttributeBase
{
    public:
        FLServiceInfo();
        FLServiceInfo(std::map<std::string, FLService>* flServices);
        virtual ~FLServiceInfo(){};
        nlohmann::ordered_json toJson() const override;
        nlohmann::ordered_json toJson(std::set<std::string>& serviceIds) const;
        nlohmann::ordered_json toJson(FLTrainingMode mode) const;
        nlohmann::ordered_json toJson(std::string& category) const;
    private:
        std::map<std::string, FLService>* flServices_;
};


#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_RESOURCES_FLSERVICEINFO_H_ */

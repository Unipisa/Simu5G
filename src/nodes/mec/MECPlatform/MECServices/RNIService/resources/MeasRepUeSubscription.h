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

#ifndef APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_
#define APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Ecgi.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "nodes/mec/utils/MecCommon.h"

class MeasRepUeSubscription : public SubscriptionBase
{

    typedef struct
    {
        std::string appIstanceId;
        AssociateId associteId_;
        Ecgi ecgi;
    }FilterCriteriaAssocTri;

    public:
        MeasRepUeSubscription();
        MeasRepUeSubscription(unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs);
        virtual ~MeasRepUeSubscription();
        virtual bool fromJson(const nlohmann::ordered_json& json) override;
        virtual void sendSubscriptionResponse() override;
        virtual void sendNotification(EventNotification *event) override;
        virtual EventNotification* handleSubscription() override {return nullptr;};
    protected:
        FilterCriteriaAssocTri filterCriteria_;

};




#endif /* APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_ */

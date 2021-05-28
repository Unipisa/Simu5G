/*
 * L2MeasSubscription.h
 *
 *  Created on: Dec 19, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_
#define APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_

#include "nodes/mec/MEPlatform/MeServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MEPlatform/MeServices/RNIService/resources/Ecgi.h"
#include "nodes/mec/MEPlatform/MeServices/RNIService/resources/AssociateId.h"
#include "nodes/mec/MecCommon.h"
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
        virtual bool fromJson(const nlohmann::ordered_json& json);
        virtual void sendSubscriptionResponse();
        virtual void sendNotification();
        virtual EventNotification* handleSubscription(){return nullptr;};
    protected:
        FilterCriteriaAssocTri filterCriteria_;

};




#endif /* APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_ */

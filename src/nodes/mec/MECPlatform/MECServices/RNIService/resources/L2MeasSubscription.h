//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_L2MEASSUBSCRIPTION_H_
#define APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_L2MEASSUBSCRIPTION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Ecgi.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "nodes/mec/utils/MecCommon.h"

using namespace omnetpp;

class L2MeasSubscription : public SubscriptionBase
{

    typedef struct
    {
        std::string appIstanceId;
        AssociateId associteId_;
        Ecgi ecgi;
    }FilterCriteriaL2Meas;

    public:
        L2MeasSubscription();
        L2MeasSubscription(unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation,  std::set<cModule*>& eNodeBs);
        virtual ~L2MeasSubscription();
        virtual bool fromJson(const nlohmann::ordered_json& json);
        virtual void sendSubscriptionResponse();
        virtual void sendNotification();
        virtual EventNotification* handleSubscription(){return nullptr;};

    protected:
        FilterCriteriaL2Meas filterCriteria_;

};




#endif /* APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_L2MEASSUBSCRIPTION_H_ */

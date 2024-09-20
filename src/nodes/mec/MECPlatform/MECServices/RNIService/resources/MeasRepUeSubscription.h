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

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Ecgi.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "nodes/mec/utils/MecCommon.h"

namespace simu5g {

using namespace omnetpp;

class MeasRepUeSubscription : public SubscriptionBase
{

    struct FilterCriteriaAssocTri {
        std::string appInstanceId; // Fixed spelling from "appIstanceId" to "appInstanceId"
        AssociateId associateId_; // Fixed spelling from "associteId_" to "associateId_"
        Ecgi ecgi;
    };

  public:
    MeasRepUeSubscription();
    MeasRepUeSubscription(unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation, std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs);
    bool fromJson(const nlohmann::ordered_json& json) override;
    void sendSubscriptionResponse() override;
    void sendNotification(EventNotification *event) override;
    EventNotification *handleSubscription() override { return nullptr; };

  protected:
    FilterCriteriaAssocTri filterCriteria_;

};

} //namespace

#endif /* APPS_MEC_MESERVICES_RNISERVICE_RESOURCES_MEASREPUESUBSCRIPTION_H_ */


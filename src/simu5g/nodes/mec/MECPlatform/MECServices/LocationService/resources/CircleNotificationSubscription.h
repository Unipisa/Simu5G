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

#ifndef APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CIRCLENOTIFICATIONSUBSCRIPTION_H_
#define APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CIRCLENOTIFICATIONSUBSCRIPTION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"

#include <set>

#include <inet/common/INETDefs.h>
#include <inet/common/geometry/common/Coord.h>

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationApiDefs.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/TerminalLocation.h"

namespace simu5g {

using namespace omnetpp;

class LteBinder;

class CircleNotificationSubscription : public SubscriptionBase
{
  public:
    CircleNotificationSubscription(Binder *binder_);
    CircleNotificationSubscription(Binder *binder_, unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation, std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs);
    CircleNotificationSubscription(Binder *binder_, unsigned int subId, inet::TcpSocket *socket, const std::string& baseResLocation, std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs, bool firstNotSent, simtime_t lastNot);

    bool fromJson(const nlohmann::ordered_json& json) override;
    void sendSubscriptionResponse() override;
    void sendNotification(EventNotification *event) override;
    EventNotification *handleSubscription() override;

    virtual bool getCheckImmediate() const { return checkImmediate; }

    bool getFirstNotification() const { return firstNotificationSent; }
    simtime_t getLastNotification() const { return lastNotification; }

    std::string getResourceUrl() const { return resourceURL; }

    bool findUe(MacNodeId nodeId);

  protected:

    opp_component_ptr<Binder> binder; // used to retrieve NodeId - Ipv4Address mapping
    simtime_t lastNotification;
    bool firstNotificationSent;

    std::map<MacNodeId, bool> users; // optional: NO the bool is the last position with respect to the area

    std::vector<TerminalLocation> terminalLocations; // it stores the user that entered or exited the area

    // callbackReference
    std::string callbackData; // optional: YES
    std::string notifyURL; // optional: NO

    std::string resourceURL;
    bool checkImmediate; // optional: NO
    std::string clientCorrelator; // optional: YES

    double frequency; // optional: NO

    inet::Coord center; // optional: NO, used for simulation

    double latitude; // optional: NO, used for simulation
    double longitude;// optional: NO, used for simulation

    double radius; // optional: NO

    int trackingAccuracy = 0; // optional: NO
    LocationUtils::EnteringLeavingCriteria actionCriteria; // optional: NO

};

} //namespace

#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CIRCLENOTIFICATIONSUBSCRIPTION_H_ */


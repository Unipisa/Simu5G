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
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/INETDefs.h"
#include <set>
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationApiDefs.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/TerminalLocation.h"

class LteBinder;
class CircleNotificationSubscription : public SubscriptionBase
{
    public:
        CircleNotificationSubscription();
        CircleNotificationSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs);
        CircleNotificationSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs, bool firstNotSent,  omnetpp::simtime_t lastNot);
        virtual ~CircleNotificationSubscription();

//        nlohmann::ordered_json toJson() const override;
//        nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const;
//        nlohmann::ordered_json toJsonUe(std::vector<MacNodeId>& uesID) const;
//        nlohmann::ordered_json toJson(std::vector<MacCellId>& cellsID, std::vector<MacNodeId>& uesID) const;


        virtual bool fromJson(const nlohmann::ordered_json& json) override;
        virtual void sendSubscriptionResponse() override;
        virtual void sendNotification(EventNotification *event) override;
        virtual EventNotification* handleSubscription() override;

        virtual bool getCheckImmediate() const { return checkImmediate;}

        bool getFirstNotification() const {return firstNotificationSent;}
        omnetpp::simtime_t getLastoNotification() const { return lastNotification;}

        std::string getResourceUrl() const { return resourceURL;}

        bool findUe(MacNodeId nodeId);

    protected:

        Binder* binder; //used to retrieve NodeId - Ipv4Address mapping
        omnetpp::simtime_t lastNotification;
        bool firstNotificationSent;

        std::map<MacNodeId, bool> users; // optional: NO the bool is the last position wrt the area

        std::vector<TerminalLocation> terminalLocations; //it stores the user that entered or exited the are

        //callbackReference
        std::string callbackData;// optional: YES
        std::string notifyURL; // optional: NO


        std::string resourceURL;
        bool checkImmediate; // optional: NO
        std::string clientCorrelator; // optional: YES

        double frequency; // optional: NO


        inet::Coord center; // optional: NO, used for simulation

        double latitude; // optional: NO, used for simulation
        double longitude;// optional: NO, used for simulation


        double radius; // optional: NO

        int trackingAccuracy; // optional: NO
        LocationUtils::EnteringLeavingCriteria actionCriteria;// optional: NO

};



#endif /* APPS_MEC_MESERVICES_LOCATIONSERVICE_RESOURCES_CIRCLENOTIFICATIONSUBSCRIPTION_H_ */

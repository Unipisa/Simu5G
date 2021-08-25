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

#ifndef APPS_MEC_MESERVICES_RESOURCES_SUBSCRIPTIONBASE_H_
#define APPS_MEC_MESERVICES_RESOURCES_SUBSCRIPTIONBASE_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer_m.h"
#include "nodes/mec/MECPlatform/EventNotification/EventNotification.h"


class LteCellInfo;

class SubscriptionBase
{
    public:
        SubscriptionBase();
        SubscriptionBase(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs);
        virtual ~SubscriptionBase();

//        nlohmann::ordered_json toJson() const override;

        void addEnodeB(std::set<omnetpp::cModule*>& eNodeBs);
        void addEnodeB(omnetpp::cModule* eNodeB);

//        virtual nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const = 0;
//        virtual nlohmann::ordered_json toJsonUe(std::vector<MacNodeId>& uesID) const = 0;
//        virtual nlohmann::ordered_json toJson(std::vector<MacCellId>& cellsID, std::vector<MacNodeId>& uesID) const = 0;

        virtual void set_links(std::string& link);

        virtual bool fromJson(const nlohmann::ordered_json& json);
        virtual void sendSubscriptionResponse() = 0;
        virtual void sendNotification(EventNotification *event) = 0;
        virtual EventNotification* handleSubscription() = 0;


        virtual std::string getSubscriptionType() const;
        virtual int getSubscriptionId() const;
        virtual int getSocketConnId() const;
//        virtual void setNotificationTrigger(subscriptionTimer *nt) { notificationTrigger = nt;}
//        virtual subscriptionTimer*  getNotificationTrigger() { return notificationTrigger;}


    protected:

//        subscriptionTimer *notificationTrigger;
        inet::TcpSocket *socket_;
        TimeStamp timestamp_;

        std::string baseResLocation_;

        std::string clientHost_;
        std::string clientUri_;


        std::map<MacCellId, CellInfo*> eNodeBs_;
        unsigned int subscriptionId_;

        std::string subscriptionType_;
        std::string notificationType_;
        std::string links_;

        std::string callbackReference_;
        TimeStamp expiryTime_;
};







#endif /* APPS_MEC_MESERVICES_RESOURCES_SUBSCRIPTIONBASE_H_ */

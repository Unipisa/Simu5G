/*
 * MeasRepUeNotification.h
 *
 *  Created on: Dec 10, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MESERVICES_RESOURCES_SUBSCRIPTIONBASE_H_
#define APPS_MEC_MESERVICES_RESOURCES_SUBSCRIPTIONBASE_H_

#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "nodes/mec/MEPlatform/MeServices/Resources/TimeStamp.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/httpUtils.h"
#include "nodes/mec/MEPlatform/MeServices/packets/AperiodicSubscriptionTimer_m.h"
#include "nodes/mec/MEPlatform/EventNotification/EventNotification.h"


class LteCellInfo;

class SubscriptionBase
{
    public:
        SubscriptionBase();
        SubscriptionBase(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::vector<omnetpp::cModule*>& eNodeBs);
        virtual ~SubscriptionBase();

//        nlohmann::ordered_json toJson() const override;

        void addEnodeB(std::vector<omnetpp::cModule*>& eNodeBs);
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


        std::map<MacCellId, LteCellInfo*> eNodeBs_;
        unsigned int subscriptionId_;

        std::string subscriptionType_;
        std::string notificationType_;
        std::string links_;

        std::string callbackReference_;
        TimeStamp expiryTime_;
};







#endif /* APPS_MEC_MESERVICES_RESOURCES_SUBSCRIPTIONBASE_H_ */

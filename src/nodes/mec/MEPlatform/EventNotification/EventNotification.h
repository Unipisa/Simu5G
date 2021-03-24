/*
 * EventNotification.h
 *
 *  Created on: Mar 23, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_EVENTNOTIFICATION_H_
#define NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_EVENTNOTIFICATION_H_

#include <string>



class EventNotification {
public:
    EventNotification();
    virtual ~EventNotification();
    EventNotification(const std::string& type, const int& subId);

    // setters
    virtual int getSubId() const;
    virtual std::string getType() const;

    // getters
    virtual void setSubId(int subId);
    virtual void setType(const std::string& type);
protected:
    std::string type_; // type of the subscription to cast to the correct notification Event
    int subId_; // subscription Id
};

#endif /* NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_EVENTNOTIFICATION_H_ */

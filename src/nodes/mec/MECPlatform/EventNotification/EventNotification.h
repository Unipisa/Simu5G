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

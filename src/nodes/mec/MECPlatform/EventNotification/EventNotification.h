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

namespace simu5g {

class EventNotification
{
  public:
    EventNotification();
    EventNotification(const std::string& type, int subId);
    virtual ~EventNotification() {}

    // setters
    virtual int getSubId() const;
    virtual std::string getType() const;

    // getters
    virtual void setSubId(int subId);
    virtual void setType(const std::string& type);

  protected:
    std::string type_; // type of the subscription to cast to the correct notification event
    int subId_; // subscription id
};

} //namespace

#endif /* NODES_MEC_MEPLATFORM_EVENTNOTIFICATION_EVENTNOTIFICATION_H_ */


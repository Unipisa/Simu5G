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

#ifndef _LOCATIONSERVICE_H
#define _LOCATIONSERVICE_H

#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationResource.h"
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"

/**
 * Location Service
 * This class inherits the MECServiceBase module interface for the implementation 
 * of the Location Service defined in ETSI GS MEC 013 Location API.
 * In particular, the current available functionalities are:
 *  - get the current location of a UE (or a group of UEs)
 *  - get the current location of a Base Station (or a group of Base Stations)
 *  - circle notification subscription 
 */


//class Location;
class AperiodicSubscriptionTimer;

class LocationService: public MecServiceBase
{
  private:

    LocationResource LocationResource_;

    double LocationSubscriptionPeriod_;
    omnetpp::cMessage *LocationSubscriptionEvent_;
    
    /*
    * This timer is used to check aperiodic subscriptions, i.e. every period subscription
    * states are checked. For example, in the circle notification subscriptions, the timer is used
    * to check if the UE enters/leaves the circle area    *
    */
    AperiodicSubscriptionTimer *subscriptionTimer_;

  public:
    LocationService();

  protected:

    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;

    virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)   override;
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    override;
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;

    /*
     * This method is called for every element in the subscriptions_ queue.
     */
    virtual bool manageSubscription() override;

    virtual ~LocationService();


};


#endif // ifndef _LOCATIONSERVICE_H


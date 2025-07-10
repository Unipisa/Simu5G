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

#include "simu5g/mec/platform/services/LocationService/resources/LocationResource.h"
#include "simu5g/mec/platform/services/base/MecServiceBase2.h"

namespace simu5g {

using namespace omnetpp;

/**
 * Location Service
 * This class inherits the MecServiceBase module interface for the implementation
 * of the Location Service defined in ETSI GS MEC 013 Location API.
 * In particular, the currently available functionalities are:
 *  - get the current location of a UE (or a group of UEs)
 *  - get the current location of a Base Station (or a group of Base Stations)
 *  - circle notification subscription
 */

//class Location;
class AperiodicSubscriptionTimer;

class LocationService : public MecServiceBase2
{
  private:

    LocationResource LocationResource_;

    double locationSubscriptionPeriod_;
    cMessage *LocationSubscriptionEvent_ = nullptr;

    /*
     * This timer is used to check aperiodic subscriptions, i.e. every period subscription
     * states are checked. For example, in the circle notification subscriptions, the timer is used
     * to check if the UE enters/leaves the circle area.
     */
    AperiodicSubscriptionTimer *subscriptionTimer_ = nullptr;

  public:
    LocationService();

  protected:

    void initialize(int stage) override;
    void finish() override;
    void handleMessage(cMessage *msg) override;

    void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) override;
    void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)   override;
    void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)    override;
    void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) override;

    /*
     * This method is called for every element in the subscriptions_ queue.
     */
    bool manageSubscription() override;

    ~LocationService() override;

};

} //namespace

#endif // ifndef _LOCATIONSERVICE_H


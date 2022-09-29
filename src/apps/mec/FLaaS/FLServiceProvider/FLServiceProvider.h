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

#ifndef _FLSERVICEPROVIDER_H
#define _FLSERVICEPROVIDER_H

//#include "apps/mec/FLaaS/FLServiceProvider/resources/" #if needed
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include "apps/mec/FLaaS/FLServiceProvider/FLService.h"
#include "apps/mec/FLaaS/FLServiceProvider/FLProcess.h"


/**
 * FL Service Provider
 * This class inherits the MECServiceBase module interface for the implementation 
 * of Federated Learning (FL) Service provider
 *
 */


class AperiodicSubscriptionTimer;

class FLServiceProvider: public MecServiceBase
{
  private:
    std::map<std::string, FLService> flServices_;
    std::map<std::string, FLProcess> flProcesses_;

    /*
    * This timer is used to check aperiodic subscriptions, i.e. every period subscription
    * states are checked. For example, in the circle notification subscriptions, the timer is used
    * to check if the UE enters/leaves the circle area    *
    */
    AperiodicSubscriptionTimer *subscriptionTimer_;

  public:
    FLServiceProvider();

  protected:

    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;

    virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)   override;
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    override;
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;

    virtual double calculateRequestServiceTime() override;


    // utils
    void onboardFLServices();
    const FLService& onboardFLService(const char* fileName);


    /*
     * This method is called for every element in the subscriptions_ queue.
     */
    virtual bool manageSubscription() override;

    virtual ~FLServiceProvider();


};


#endif // ifndef _FLSERVICEPROVIDER_H


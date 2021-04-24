/*
 * MeGetApp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_MEBG_APP_H_
#define APPS_MEC_MEAPPS_MEBG_APP_H_

#include "apps/mec/MeApps/MeAppBase.h"
#include "inet/common/lifecycle/NodeStatus.h"

using namespace omnetpp;

class MecRequestBackgroundApp : public MeAppBase
{
protected:

     inet::NodeStatus *nodeStatus = nullptr;
     int numberOfApplications_;    // requests to send in this session
     cMessage *burstTimer;
     cMessage *burstPeriod;
     bool      burstFlag;
     cMessage *sendBurst;

     virtual void handleSelfMessage(cMessage *msg) override;

     virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
     virtual void initialize(int stage) override;

     virtual void sendBulkRequest();

   public:
     MecRequestBackgroundApp() {}
     virtual ~MecRequestBackgroundApp();
     virtual void handleServiceMessage() override;
     virtual void established(int connId) override;
};



#endif /* APPS_MEC_MEAPPS_MEBGAPP_H_ */

/*
 * MeGetApp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_MEBGAPP_H_
#define APPS_MEC_MEAPPS_MEBGAPP_H_

#include "apps/mec/MeApps/MecAppBase.h"
#include "inet/common/lifecycle/NodeStatus.h"

using namespace omnetpp;

class MecRequestBackgroundGeneratorApp : public MecAppBase
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

     virtual void handleServiceMessage() override;
     virtual void handleMp1Message() override;

     virtual void handleUeMessage(omnetpp::cMessage *msg) override {};


     virtual void established(int connId) override;

   public:
     MecRequestBackgroundGeneratorApp() {}
     virtual ~MecRequestBackgroundGeneratorApp();
};



#endif /* APPS_MEC_MEAPPS_MEBGAPP_H_ */

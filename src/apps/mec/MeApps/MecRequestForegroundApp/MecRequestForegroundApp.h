/*
 * MeGetApp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_MEFGAPP_H_
#define APPS_MEC_MEAPPS_MEFGAPP_H_

#include "apps/mec/MeApps/MeAppBase.h"
#include "inet/common/lifecycle/NodeStatus.h"

using namespace omnetpp;

class MecRequestForegroundApp : public MeAppBase
{
protected:
     inet::NodeStatus *nodeStatus = nullptr;
     cMessage *sendFGRequest;

     virtual void handleSelfMessage(cMessage *msg) override;

     virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
     virtual void initialize(int stage) override;

     virtual void sendMsg();

   public:
     MecRequestForegroundApp() {}
     virtual ~MecRequestForegroundApp();
     virtual void handleServiceMessage() override;
     virtual void handleUeMessage(omnetpp::cMessage *msg) override {}
     virtual void established(int connId) override;
};



#endif /* APPS_MEC_MEAPPS_MEFGAPP_H_ */

/*
 * MeGetApp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_DEVICEAPP_H_
#define APPS_MEC_MEAPPS_DEVICEAPP_H_

#include "apps/mec/MeApps/MeAppBase.h"
#include "inet/common/lifecycle/NodeStatus.h"

using namespace omnetpp;

class DeviceApp : public MeAppBase
{
    protected:

      inet::NodeStatus *nodeStatus = nullptr;
      bool earlySend = false;    // if true, don't wait with sendRequest() until established()
      int numRequestsToSend = 0;    // requests to send in this session
      bool flag;

      simsignal_t responseTime_;

//      virtual void sendRequest();
//      virtual void rescheduleOrDeleteTimer(simtime_t d, short int msgKind);


      virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
      virtual void initialize(int stage) override;
      virtual void sendMsg();
//      virtual void handleTimer(cMessage *msg) override;
//      virtual void socketEstablished(int connId, void *yourPtr) override;
//      virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent) override;
//      virtual void socketClosed(int connId, void *yourPtr) override;
//      virtual void socketFailure(int connId, void *yourPtr, int code) override;
//      virtual bool isNodeUp();
//      virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;

      virtual void handleServiceMessage() override;
      virtual void handleUeMessage(omnetpp::cMessage *msg) override {}
      virtual void handleSelfMessage(omnetpp::cMessage *msg) override;
      virtual void established(int connId)override;

    public:
      DeviceApp() {}
      virtual ~DeviceApp();

 };



#endif /* APPS_MEC_MEAPPS_DEVICEAPP_H_ */

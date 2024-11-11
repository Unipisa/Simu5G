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

#ifndef APPS_MEC_MEAPPS_MEFGAPP_H_
#define APPS_MEC_MEAPPS_MEFGAPP_H_

#include <inet/common/lifecycle/NodeStatus.h>

#include "apps/mec/MecApps/MecAppBase.h"

namespace simu5g {

using namespace omnetpp;

class MecRequestForegroundApp : public MecAppBase
{
  protected:
    cMessage *sendFGRequest = nullptr;
    double lambda;

    inet::TcpSocket *serviceSocket_ = nullptr;
    inet::TcpSocket *mp1Socket_ = nullptr;

    HttpBaseMessage *mp1HttpMessage = nullptr;
    HttpBaseMessage *serviceHttpMessage = nullptr;

    void handleSelfMessage(cMessage *msg) override;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;

    void handleHttpMessage(int connId) override;
    void handleServiceMessage(int connId) override;
    void handleMp1Message(int connId) override;

    void handleUeMessage(cMessage *msg) override {};

    void established(int connId) override;

    virtual void sendRequest();

  public:
    ~MecRequestForegroundApp() override;

};

} //namespace

#endif /* APPS_MEC_MEAPPS_MEFGAPP_H_ */


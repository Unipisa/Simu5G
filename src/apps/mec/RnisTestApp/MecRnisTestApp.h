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

#ifndef __MecRnisTestApp_H_
#define __MecRnisTestApp_H_

#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

namespace simu5g {

using namespace omnetpp;

//
// This is a simple MEC app that connects to the Radio Network Information
// Service and periodically requests L2 Measurements from it.
// The response obtained from the RNIS is just sent back to the UE app.
//
class MecRnisTestApp : public MecAppBase
{
    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    inet::TcpSocket *serviceSocket_ = nullptr;
    inet::TcpSocket *mp1Socket_ = nullptr;

    HttpBaseMessage *mp1HttpMessage = nullptr;
    HttpBaseMessage *serviceHttpMessage = nullptr;

    simtime_t rnisQueryingPeriod_;
    cMessage *rnisQueryingTimer_ = nullptr;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;

    void handleProcessedMessage(cMessage *msg) override;

    void handleHttpMessage(int connId) override;
    void handleServiceMessage(int connId) override;
    void handleMp1Message(int connId) override;
    void handleUeMessage(cMessage *msg) override;

    virtual void sendQuery(int cellId, std::string ueIpv4Address);

    void handleSelfMessage(cMessage *msg) override;

    void established(int connId) override;
};

} //namespace

#endif


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

#ifndef __UERNISTESTAPP_H_
#define __UERNISTESTAPP_H_

#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "common/binder/Binder.h"
#include "apps/mec/RnisTestApp/packets/RnisTestAppPacket_m.h"
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"

namespace simu5g {

using namespace omnetpp;

//
// This UE app asks the Device App to instantiate a MecRnisTestApp.
// After the MEC app has been initialized, the UE app requests the MEC app to
// periodically query the RNI Service.
// When the UE app receives the requested info from the MEC app, it simply outputs
// that in the Qtenv log (set the logger parameter to dump the output to a file).
// If the period is set to 0, then only one query is requested.
//
class UeRnisTestApp : public cSimpleModule
{

    // Communication to device app and MEC app
    inet::UdpSocket socket;

    simtime_t period_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    // MEC application end point (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    // Scheduling
    enum MsgKind {
        KIND_SELF_START = 1000,
        KIND_SELF_STOP,
        KIND_SELF_MEC_APP_START,
    };

    cMessage *selfStart_ = nullptr;
    cMessage *selfStop_ = nullptr;

    cMessage *selfMecAppStart_ = nullptr;

    // Used to write in a log file
    bool log;

  public:
    ~UeRnisTestApp() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    void sendStartMecApp();
    void sendMessageToMecApp();
    void sendStopMecApp();

    void handleAckStartMecApp(cMessage *msg);
    void handleInfoMecApp(cMessage *msg);
    void handleAckStopMecApp(cMessage *msg);
};

} //namespace

#endif


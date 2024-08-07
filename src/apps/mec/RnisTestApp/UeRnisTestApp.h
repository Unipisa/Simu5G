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

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"

#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "apps/mec/RnisTestApp/packets/RnisTestAppPacket_m.h"

namespace simu5g {

using namespace omnetpp;

//
// This UE app asks the Device App to instantiate a MecRnisTestApp.
// After the MEC app has been initialized, the UE app requests the MEC app to
// periodically query the RNI Service.
// When the UE app received the requested info from the MEC app, it just outputs
// that in the Qtenv log (set the logger parameter to dump the output to a file).
// If the period is set to 0, then only one query is requested.
//
class UeRnisTestApp : public cSimpleModule
{

    //communication to device app and mec app
    inet::UdpSocket socket;

    simtime_t period_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    //scheduling
    enum MsgKind {
        KIND_SELF_START = 1000,
        KIND_SELF_STOP,
        KIND_SELF_MEC_APP_START,
    };

    cMessage *selfStart_;
    cMessage *selfStop_;

    cMessage *selfMecAppStart_;

    // uses to write in a log a file
    bool log;

  public:
    ~UeRnisTestApp();
    UeRnisTestApp();

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    void sendStartMecApp();
    void sendMessageToMecApp();
    void sendStopMecApp();

    void handleAckStartMecApp(cMessage *msg);
    void handleInfoMecApp(cMessage *msg);
    void handleAckStopMecApp(cMessage *msg);
};

} //namespace

#endif


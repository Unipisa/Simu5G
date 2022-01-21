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

#ifndef __UEPLATOONINGAPP_H_
#define __UEPLATOONINGAPP_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "apps/mec/PlatooningApp/packets/PlatooningPacket_m.h"
#include "apps/mec/PlatooningApp/mobility/LinearAccelerationMobility.h"

using namespace omnetpp;

typedef enum {
    NOT_JOINED,
    JOINED
} UePlatoonStatus;

class UEPlatooningApp: public cSimpleModule
{
    //communication to device app and mec app
    inet::UdpSocket socket;

    int joinRequestPacketSize_;
    int leaveRequestPacketSize_;
    int controllerIndex_;

    // DeviceApp info
    int localPort_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    // mobility information
    cModule* ue;
    LinearAccelerationMobility* mobility;
    inet::Coord position;

    // current status of the UE
    UePlatoonStatus status_;

    //scheduling
    cMessage *selfStart_;
    cMessage *selfStop_;

  public:
    ~UEPlatooningApp();
    UEPlatooningApp();

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    // --- Functions to interact with the DeviceApp --- //
    void sendStartMECPlatooningApp();
    void sendStopMECPlatooningApp();
    void handleAckStartMECPlatooningApp(cMessage* msg);
    void handleAckStopMECPlatooningApp(cMessage* msg);

    // --- Functions to interact with the MECPlatooningApp --- //
    void sendJoinPlatoonRequest();
    void sendLeavePlatoonRequest();
    void recvJoinPlatoonResponse(cMessage* msg);
    void recvLeavePlatoonResponse(cMessage* msg);
    void recvPlatoonCommand(cMessage* msg);


};

#endif

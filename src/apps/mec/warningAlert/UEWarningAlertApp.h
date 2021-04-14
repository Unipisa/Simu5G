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

#ifndef __UEWARNINGALERTAPP_H_
#define __UEWARNINGALERTAPP_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

//WarningAlertPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "apps/mec/warningAlert/packets/WarningAlertPacket_m.h"

using namespace omnetpp;

/**
 * See UEWarningAlertApp.ned
 */
class UEWarningAlertApp : public cSimpleModule
{
    //communication
    inet::UdpSocket socket;
    int size_;
    simtime_t period_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    char* sourceSimbolicAddress;            //Ue[x]
    char* destSimbolicAddress;              //meHost.virtualisationInfrastructure

    // mobility informations
    cModule* ue;
    inet::IMobility *mobility;
    inet::Coord position;

    //resources required
    int requiredRam;
    int requiredDisk;
    double requiredCpu;

    //scheduling
    cMessage *selfStart_;
    cMessage *selfSender_;
    cMessage *selfStop_;

    public:
        ~UEWarningAlertApp();
        UEWarningAlertApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        void sendStartMEWarningAlertApp();
        void sendInfoUEWarningAlertApp();
        void sendStopMEWarningAlertApp();

        void handleAckStartMEWarningAlertApp(cMessage* msg);
        void handleInfoMEWarningAlertApp(cMessage* msg);
        void handleAckStopMEWarningAlertApp(cMessage* msg);
};

#endif

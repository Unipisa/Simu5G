//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __UEWarningAlertApp_rest_REST_H_
#define __UEWarningAlertApp_rest_REST_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "nodes/binder/LteBinder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

//WarningAlertPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "apps/mec/warningAlert/packets/WarningAlertPacket_m.h"

using namespace omnetpp;

/**
 * See UEWarningAlertApp_rest.ned
 */
class UEWarningAlertApp_rest : public cSimpleModule
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


    int mecAppPort_;
    inet::L3Address mecAppAddress_;


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
        ~UEWarningAlertApp_rest();
        UEWarningAlertApp_rest();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        void sendStartMEWarningAlertApp();
        void sendStopMEWarningAlertApp();

        void handleAckStartMEWarningAlertApp(cMessage* msg);
        void handleInfoMEWarningAlertApp(cMessage* msg);
        void handleAckStopMEWarningAlertApp(cMessage* msg);
};

#endif

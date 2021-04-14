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

#ifndef __MECREQUESTAPP_H_
#define __MECREQUESTAPP_H_

#include "apps/mecRequestResponseApp/packets/MigrationTimer_m.h"
#include "apps/mecRequestResponseApp/packets/MecRequestResponsePacket_m.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "stack/phy/layer/NRPhyUe.h"

using namespace omnetpp;

class MecRequestApp : public cSimpleModule
{
    inet::UdpSocket socket;
    simtime_t period_;
    int localPort_;
    int destPort_;
    char* sourceSymbolicAddress_;
    inet::L3Address destAddress_;

    NRPhyUe* nrPhy_;

    unsigned int sno_;
    unsigned int bsId_;
    unsigned int appId_;

    bool enableMigration_;

    //scheduling
    cMessage *selfSender_;

    static simsignal_t requestSize_;
    static simsignal_t requestRTT_;
    static simsignal_t recvResponseSno_;

    public:
        ~MecRequestApp();
        MecRequestApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        void sendRequest();
        void recvResponse(cMessage* msg);
};

#endif

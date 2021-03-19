//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __MEWARNINGALERTAPP_REST_H_
#define __MEWARNINGALERTAPP_REST_H_

#include "omnetpp.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEWarningAlertPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "apps/mec/warningAlert/packets/WarningAlertPacket_m.h"

#include "nodes/mec/MEPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MeApps/MeAppBase.h"


using namespace std;
using namespace omnetpp;


/**
 * See MEWarningAlertApp_rest.ned
 */
class MEWarningAlertApp_rest : public MeAppBase
{
    char* ueSimbolicAddress;
    char* meHostSimbolicAddress;
    int localPort_;

    SockAddr serviceSockAddress;


    int size_;
    std::string subId;

    //references to MeHost module
    cModule* meHost;
    cModule* mePlatform;
    ServiceRegistry * serviceRegistry;

    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    inet::L3Address destAddress_;
    int destPort_;

    protected:

        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

//        void handleInfoUEWarningAlertApp(WarningAlertPacket* pkt);
//        void handleInfoMEWarningAlertApp(WarningAlertPacket* pkt);

        void handleServicePacket();

        virtual void modifySubscription();

        virtual void handleSelfMessage(cMessage *msg) override;
        /* Utility functions */
       virtual void connect() override;
//        virtual void close();
//        virtual void sendPacket(cPacket *pkt);
//
//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;
       virtual void handleTcpMsg() override;

       virtual void handleStartOperation(inet::LifecycleOperation *operation) override;


};

#endif

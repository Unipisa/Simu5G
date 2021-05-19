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

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEWarningAlertPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "apps/mec/warningAlert/packets/WarningAlertPacket_m.h"

#include "nodes/mec/MEPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MeApps/MecAppBase.h"


using namespace std;
using namespace omnetpp;


/**
 * See MEWarningAlertApp_rest.ned
 */
class MEWarningAlertApp_rest : public MecAppBase
{
    /*
     *
     */
    inet::UdpSocket ueAppSocket_;
    int localUePort;

//    char* ueSimbolicAddress;
    inet::L3Address ueAppAddress;
    int ueAppPort;


//    char* meHostSimbolicAddress;
//    inet::L3Address destAddress_;
//    int destPort_;
//
//    int localPort_;


    int size_;
    std::string subId;

    cOvalFigure * circle; // circle danger zone


    ;

    protected:

        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *msg) override;
//        virtual void handleMessageWhenUp(cMessage *msg) override;
        virtual void finish() override;

//        void handleInfoUEWarningAlertApp(WarningAlertPacket* pkt);
//        void handleInfoMEWarningAlertApp(WarningAlertPacket* pkt);

        virtual void handleServiceMessage() override;
        virtual void handleMp1Message() override;

        virtual void handleUeMessage(omnetpp::cMessage *msg) override;

        virtual void modifySubscription();
        virtual void sendSubscription();
        virtual void handleSelfMessage(cMessage *msg) override;
        /* Utility functions */
//       virtual void connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port) override;
//        virtual void close();
//        virtual void sendPacket(cPacket *pkt);
//
//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;

    public:
       MEWarningAlertApp_rest();
       virtual ~MEWarningAlertApp_rest();

};

#endif

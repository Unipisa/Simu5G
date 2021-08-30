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


#ifndef APPS_MEC_MEAPPS_DEVICEAPP_H_
#define APPS_MEC_MEAPPS_DEVICEAPP_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "DeviceAppMessages/DeviceAppPacket_m.h"


/*
 * This is a very simple application implementing some Device Application functionalities,
 * i.e. instantiation and termination of MEC apps requests.
 * It follows the ETSI specification of ETSI GS MEC 016 V2.2.1 (2020-04)
 *
 * In particular it receives requests from the relative UE app (it is supposed that each UE
 * app has it own device app) and it interfaces with the UALCM proxy via the RESTful API.
 *
 * TCP socket management is minimal, it send requests only if the socket is connected and
 * responds with nacks (withe reason to the UE app) if not.
 *
 * Communication with the UE app occurs via set of OMNeT++ messages:
 *  - request instantation of a MEC app
 *  - request termination of a MEC app
 *  - ACK and NACK about the above requests
 *
 * This device app can be also queried by external UE app (in emulation mode). So, the
 * serializer/deserializer for the the above messages is provided within the
 * DeviceAppMessages/Serializers folder
 */

class HttpBaseMessage;

enum State {IDLE, START, APPCREATED, CREATING, DELETING};


class DeviceApp : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback, public inet::UdpSocket::ICallback
{
    protected:

        inet::TcpSocket UALCMPSocket_;
        inet::UdpSocket ueAppSocket_;

        inet::L3Address UALCMPAddress;
        int  UALCMPPort;

        HttpBaseMessage* UALCMPMessage;
        std::string UALCMPMessageBuffer;

        omnetpp::cMessage* processedUALCMPMessage;

        int localPort;

        inet::L3Address ueAppAddress;
        int ueAppPort;

        bool flag;

        std::string appContextUri;
        std::string mecAppEndPoint;

        State appState;
        std::string appName;

        // variable set in ned, if the appDescriptor is not in the MEC orchestrator
//        std::string appProvider; not used
        std::string appPackageSource;


        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void handleMessage(omnetpp::cMessage *msg) override;
        virtual void finish() override;

        /* Utility functions */
        virtual void handleSelfMessage(omnetpp::cMessage *msg);
        virtual void handleUALCMPMessage();
        void sendStartAppContext(inet::Ptr<const DeviceAppPacket> pk);
        void sendStopAppContext(inet::Ptr<const DeviceAppPacket> pk);

        virtual void connectToUALCMP();

        /* inet::TcpSocket::CallbackInterface callback methods */
        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
        virtual void socketEstablished(inet::TcpSocket *socket) override;
        virtual void socketPeerClosed(inet::TcpSocket *socket) override;
        virtual void socketClosed(inet::TcpSocket *socket) override;
        virtual void socketFailure(inet::TcpSocket *socket, int code) override;
        virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
        virtual void socketDeleted(inet::TcpSocket *socket) override {}

        /* inet::UdpSocket::CallbackInterface callback methods */
        virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
        virtual void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override;
        virtual void socketClosed(inet::UdpSocket *socket) override;

    public:
      DeviceApp();
      virtual ~DeviceApp();

 };

#endif /* APPS_MEC_MEAPPS_DEVICEAPP_H_ */

/*
 * MeGetApp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: linofex
 */

#ifndef APPS_MEC_MEAPPS_DEVICEAPP_H_
#define APPS_MEC_MEAPPS_DEVICEAPP_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "DeviceAppMessages/DeviceAppPacket_m.h"

class HttpBaseMessage;

enum State {IDLE, START, CREATE, DELETE};


class DeviceApp : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback, public inet::UdpSocket::ICallback
{
    protected:

        inet::TcpSocket lcmProxySocket_;
        inet::UdpSocket ueAppSocket_;

        inet::L3Address lcmProxyAddress;
        int  lcmProxyPort;

        HttpBaseMessage* lcmProxyMessage;
        std::string lcmProxyMessageBuffer;

        omnetpp::cMessage* processedLcmProxyMessage;

        int localPort;

        inet::L3Address ueAppAddress;
        int ueAppPort;

        bool flag;

        std::string appContextUri;
        std::string mecAppEndPoint;

        State appState;
        std::string appName;

        // variable set in ned, if the appDescriptor is not in the MEC orchestrator
        std::string appProvider;
        std::string appPackageSource;


        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void handleMessage(omnetpp::cMessage *msg) override;
        virtual void finish() override;

        /* Utility functions */
//        virtual void parseReceivedMsg(inet::TcpSocket *socket, HttpBaseMessage* currentHttpMessage,  std::string& packet);
        //    virtual void handleTimer(omnetpp::cMessage *msg) override {};
        virtual void handleSelfMessage(omnetpp::cMessage *msg);
        virtual void handleLcmProxyMessage();
        void sendStartAppContext(inet::Ptr<const DeviceAppPacket> pk);
        void sendStopAppContext(inet::Ptr<const DeviceAppPacket> pk);

        virtual void connectToLcmProxy();

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

        virtual void established(int connId);

    public:
      DeviceApp();
      virtual ~DeviceApp();

 };

#endif /* APPS_MEC_MEAPPS_DEVICEAPP_H_ */

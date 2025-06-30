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

#include <inet/transportlayer/contract/tcp/TcpSocket.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "DeviceAppMessages/DeviceAppPacket_m.h"

namespace simu5g {

using namespace omnetpp;

/*
 * This is a very simple application implementing some Device Application functionalities,
 * i.e. instantiation and termination of MEC apps requests.
 * It follows the ETSI specification of ETSI GS MEC 016 V2.2.1 (2020-04)
 *
 * In particular it receives requests from the relative UE app (it is supposed that each UE
 * app has it own device app) and it interfaces with the UALCM proxy via the RESTful API.
 *
 * TCP socket management is minimal, it send requests only if the socket is connected and
 * responds with nacks (with reason to the UE app) if not.
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

enum State { IDLE, START, APPCREATED, CREATING, DELETING };

class DeviceApp : public cSimpleModule, public inet::TcpSocket::ICallback, public inet::UdpSocket::ICallback
{
  protected:

    inet::TcpSocket UALCMPSocket_;
    inet::UdpSocket ueAppSocket_;

    inet::L3Address UALCMPAddress;
    int UALCMPPort;

    HttpBaseMessage *UALCMPMessage = nullptr;
    std::string UALCMPMessageBuffer;

    cMessage *processedUALCMPMessage = nullptr;

    int localPort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    bool flag;

    std::string appContextUri;
    std::string mecAppEndPoint;

    State appState;
    std::string appName;

    // mapping from app names to app dev id to use (only used for shared apps)
    std::map<std::string, int> devAppIds;

    // variable set in ned, if the appDescriptor is not in the MEC orchestrator
    std::string appPackageSource;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
    void finish() override;

    /* Utility functions */
    virtual void handleSelfMessage(cMessage *msg);
    virtual void handleUALCMPMessage();
    void sendStartAppContext(inet::Ptr<const DeviceAppPacket> pk);
    void sendStopAppContext(inet::Ptr<const DeviceAppPacket> pk);

    virtual void connectToUALCMP();

    /* inet::TcpSocket::CallbackInterface callback methods */
    void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;
    void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    void socketEstablished(inet::TcpSocket *socket) override;
    void socketPeerClosed(inet::TcpSocket *socket) override;
    void socketClosed(inet::TcpSocket *socket) override;
    void socketFailure(inet::TcpSocket *socket, int code) override;
    void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
    void socketDeleted(inet::TcpSocket *socket) override {}

    /* inet::UdpSocket::CallbackInterface callback methods */
    void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
    void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override;
    void socketClosed(inet::UdpSocket *socket) override;

  public:
    ~DeviceApp() override;

};

} //namespace

#endif /* APPS_MEC_MEAPPS_DEVICEAPP_H_ */


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

#ifndef __MECWARNINGALERTAPP_H_
#define __MECWARNINGALERTAPP_H_

#include "omnetpp.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEWarningAlertPacket
//#include "nodes/mec/MECPlatform/MECAppPacket_Types.h"
#include "./packets/PingPongPacket_Types.h"
#include "./packets/PingPongPacket_m.h"


#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "apps/mec/MecApps/MecAppBase.h"

#include "spdlog/spdlog.h"  // logging library
#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
#include <fmt/format.h>

#include  "apps/mec/MecApps/packets/ProcessingTimeMessage_m.h"
//#include "utility.h"

using namespace std;
using namespace omnetpp;

//
//  This is a simple MEC app that receives the coordinates and the radius from the UE app
//  and subscribes to the CircleNotificationSubscription of the Location Service. The latter
//  periodically checks if the UE is inside/outside the area and sends a notification to the
//  MEC App. It then notifies the UE.

//
//  The event behavior flow of the app is:
//  1) receive coordinates from the UE app
//  2) subscribe to the circleNotificationSubscription
//  3) receive the notification
//  4) send the alert event to the UE app
//  5) (optional) receive stop from the UE app
//
// TCP socket management is not fully controlled. It is assumed that connections works
// at the first time (The scenarios used to test the app are simple). If a deeper control
// is needed, feel free to improve it.

//

class MECPingPongApp : public MecAppBase
{
    std::string log_level = "high"; 


    //UDP socket to communicate with the UeApp
    inet::UdpSocket ueSocket;
    int localUePort;

    inet::L3Address ueAppAddress;
    int ueAppPort;

    int iDframe_;
    int size_;
    std::string subId;


    inet::TcpSocket* serviceSocket_;
    inet::TcpSocket* mp1Socket_;

    HttpBaseMessage* mp1HttpMessage;
    HttpBaseMessage* serviceHttpMessage;



    // circle danger zone
    double Radius_;
    cOvalFigure * circle;
    double centerPositionX;
    double centerPositionY;
    double radius;

    //int vehicle;

    std::shared_ptr<spdlog::logger> myLogger;
    std::string csv_filename;
    std::string csv_filename_total;
    std::string csv_filename_send_total;
    std::string csv_filename_arrival_time;
    std::string csv_filename_sendnei_time;
    static int counter;
    static omnetpp::simsignal_t mecapp_ul_delaySignal;
    static omnetpp::simsignal_t mecapp_pk_rcv_sizeSignal;


    std::string log_identifier;

    int numCars;
    int row =0 ;
    double processingTime_;
    double maxCpuSpeed_;
    std::string App_name_;
    int nbSentMsg;
    cQueue pingPongpacketQueue_;
    cMessage* pingPongprocessMessage_;;



    //std::random_device rd;
    //std::mt19937 gen;
    //std::exponential_distribution<> exponential;
    //std::poisson_distribution<int> poisson;
    //std::uniform_real_distribution<> uniform;

    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        //virtual void handleMessage(cMessage *msg) override;

        virtual void finish() override;

        virtual void handleServiceMessage(int connId) override;
        virtual void handleMp1Message(int connId) override;

        virtual void handleUeMessage(omnetpp::cMessage *msg) override;

        virtual void modifySubscription();
        virtual void sendSubscription();
        virtual void sendDeleteSubscription();
        virtual void handleHttpMessage(int connId);
        virtual void handleSelfMessage(cMessage *msg) override;
        virtual void handleProcessedMessage(cMessage *msg);
        virtual void sendPingPacket(cMessage *msg);
        virtual double computationTime(int N);
        virtual double scheduleNextMsg(cMessage* msg) override;
        virtual void handleMessage(omnetpp::cMessage *msg) override;

//        /* TCPSocket::CallbackInterface callback methods */
       virtual void established(int connId) override;
       //double uniformGenerator(int min,int max);
       //double exponentialGenerator(double lambda);
        //int poissonGenerator(double lambda);
       virtual void sendPingPacketNeighboring(cMessage *msg);


    //inet::Coord getCoordinates(const MacNodeId id);
    //std::list<MacNodeId>  getAllCars();
    //inet::Coord getMyCurrentPosition_(cModule* module);
    //cModule* getModule_(const MacNodeId id);
    //void showCircle(double radius, inet::Coord centerPosition,cModule* senderCarModule , cOvalFigure * circle );
    //std::list<MacNodeId>  getNeighbors(std::list<MacNodeId> all_cars,double Radius_,cModule*  mymodule);
    //inet::Coord getCoordinates(cModule* mod);

    public:
       MECPingPongApp();
       virtual ~MECPingPongApp();

};

#endif

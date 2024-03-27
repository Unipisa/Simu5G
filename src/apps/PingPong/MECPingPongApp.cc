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
#define FMT_HEADER_ONLY

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "./packets/PingPongPacket_Types.h"


#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet_m.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/httpUtils/json.hpp"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"


#include <random>


#include <fstream>
#include "MECPingPongApp.h"

#include "spdlog/spdlog.h"  // logging library
#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
#include <fmt/format.h>

#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <map>
#include <iostream>
#include <sys/stat.h>
#include "CommonFunctions.h"




Define_Module(MECPingPongApp);

using namespace inet;
using namespace omnetpp;
//using namespace utility;

simsignal_t MECPingPongApp::mecapp_ul_delaySignal =registerSignal("mecapp_ul_delaySignal") ;
simsignal_t MECPingPongApp:: mecapp_pk_rcv_sizeSignal =registerSignal("mecapp_pk_rcv_sizeSignal") ;




MECPingPongApp::MECPingPongApp(): MecAppBase()
{
    circle = nullptr; // circle danger zone
    pingPongprocessMessage_ = nullptr;


}
MECPingPongApp::~MECPingPongApp()
{
    if(circle != nullptr)
    {
        if(getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }
    cancelAndDelete(pingPongprocessMessage_);

}

int get_packet_dimension() {
    static std::default_random_engine generator;
    static std::normal_distribution<double> distribution(1024.0,100.0);

    return int(distribution(generator));
}


double MECPingPongApp::computationTime(int port)
{
    double ins;
    //ASSERT( port= 5415631641);
    if (port==4000){//td
         ins= exponentialGenerator(1.0 / (500*pow(10, 6)));
              }
    else if (port==4003){//cs
        ins = exponentialGenerator(1.0 / (200*pow(10, 6)));
               }
    else if (port==4005){ //caa
       ins = exponentialGenerator(1.0 / (200*pow(10, 6))) ;
       }
    else if (port==4006){ //cm
       ins = exponentialGenerator(1.0 / (500 * pow(10, 6)));
       
       }
   else {
       ins = 10;
   }
   ins = (ins<0)? 0:ins;
   double processingTime_ = vim->calculateProcessingTime(mecAppId, ins);


   //if (processingTime_ >1000 || processingTime_ <0.0000001 ){
   //    throw cRuntimeError("Impossible processing time %s",processingTime_);
   //}
    return processingTime_;
//    scheduleAt(simTime()+ processingTime_, processingTimer_);
}

double MECPingPongApp::scheduleNextMsg(cMessage* msg)
{
    // determine its source address/port

    if (!strcmp(msg->getName(), "PingPongPacket"))
            {
        try {

        auto pk = check_and_cast<Packet *>(msg);
        ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
        ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();
        auto mecPk = pk->peekAtFront<PingPongPacket>();
        auto pubPk = dynamicPtrCast<const PingPongPacket>(mecPk);

        //check sender name
        std::string car =L3AddressResolver().findHostWithAddress(ueAppAddress)->getFullName();
        //std::string test =L3AddressResolver().findHostWithAddress(ueAppAddress)->getFullPath();

        double processingTime {computationTime(ueAppPort)} ;
        ResourceDescriptor avRes = vim->getAvailableResources();
        double usedCPU = maxCpuSpeed_ - avRes.cpu*pow(10,-6) ;


        auto ev = getSimulation()->getActiveEnvir();
        auto currentRun = ev->getConfigEx()->getActiveRunNumber();
        //add number of mecApp and cars
        ofstream myfile;
        myfile.open (csv_filename_total, ios::app);
        if(myfile.is_open())
             {
            // myfile << "timestamp,arrivalTime,ueAppAddress,ueAppPort,car,recv_packet_size(B),uplink_delay(s),Queueing_delay(s),processing_delay(s),idframe,nbSentMsg" << endl;
                 myfile  << simTime() << "," <<pk->getArrivalTime() << "," << ueAppAddress << "," << ueAppPort <<","<< car << "," << pk->getByteLength() << "," << pk->getArrivalTime() - pk->getCreationTime()<<","<< simTime() - msg->getArrivalTime()  << "," << processingTime << "," << pubPk->getIDframe()<<"," << nbSentMsg++ <<","<< currentRun<<","<<packetQueue_.getLength() << endl;
                 myfile.close();
             }

        return processingTime;

            }
            
        catch (const cRuntimeError& error){
                //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                delete msg;
            }

            }
    else {
        double processingTime = vim->calculateProcessingTime(mecAppId, 20);// int(uniform(200, 400))); this cause machines to invert orders!
        return processingTime;
    }
}

void MECPingPongApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {


        if(strcmp(msg->getName(), "processedMessage") == 0)
        {
            handleProcessedMessage((cMessage*)packetQueue_.pop());
            //delete packetQueue_.pop();
            if(!packetQueue_.isEmpty())
            {
                double processingTime = scheduleNextMsg((cMessage *)packetQueue_.front());
                EV <<"MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
                scheduleAt(simTime() + processingTime, processMessage_);
            }
            else
            {
                EV << "MecAppBase::handleMessage - no more messages are present in the queue" << endl;
            }
        }
        else if(strcmp(msg->getName(), "processedHttpMsg") == 0)
        {
            EV << "MecAppBase::handleMessage(): processedHttpMsg " << endl;
            ProcessingTimeMessage * procMsg = check_and_cast<ProcessingTimeMessage*>(msg);
            int connId = procMsg->getSocketId();
            TcpSocket* sock = (TcpSocket*)sockets_.getSocketById(connId);
            if(sock != nullptr)
            {
                HttpMessageStatus* msgStatus = (HttpMessageStatus *)sock->getUserData();
                //handleHttpMessage(connId);
                delete msgStatus->httpMessageQueue.pop();
                if(!msgStatus->httpMessageQueue.isEmpty())
                {
                    EV << "MecAppBase::handleMessage(): processedHttpMsg - the httpMessageQueue is not empty, schedule next HTTP message" << endl;
                    double time = vim->calculateProcessingTime(mecAppId, 150);
                    scheduleAt(simTime()+time, msg);
                }
            }
        }
        else
        {
            handleSelfMessage(msg);
        }
    }
    else
    {
        if (!strcmp(msg->getName(), "PingPongPacket")){
            try{
            auto pk = check_and_cast<Packet *>(msg);
            ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
            ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();
            auto mecPk = pk->peekAtFront<PingPongPacket>();
            auto pubPk = dynamicPtrCast<const PingPongPacket>(mecPk);
            auto ev = getSimulation()->getActiveEnvir();
            auto currentRun = ev->getConfigEx()->getActiveRunNumber();
            std::string carName =L3AddressResolver().findHostWithAddress(ueAppAddress)->getFullName();

                        ofstream myfile;
                        myfile.open (csv_filename_arrival_time, ios::app);
                        if(myfile.is_open())
                             {
                                 //myfile << "timestamp,ueAppAddress,ueAppPort,packet_size_to_send(B),payload_size_to_send(B)" << endl;
                                 myfile  << simTime() << "," << pk->getArrivalTime()<< ","<< pubPk->getIDframe()<<","<<pk->getArrivalTime() - pk->getCreationTime()<<","<< carName << "," << packetQueue_<<","<<currentRun <<","<<packetQueue_.getLength()<< endl;
                                 myfile.close();

                             }
                             //drop packet if from other cars? 

            }
            catch (const cRuntimeError& error){
        //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
        delete msg;
            }
            }
        if(!processMessage_->isScheduled() && packetQueue_.isEmpty())
        {



            packetQueue_.insert(msg);
            double processingTime;
            if(strcmp(msg->getFullName(), "data") == 0)
                processingTime = MecAppBase::scheduleNextMsg(msg);
            else
               processingTime = scheduleNextMsg(msg);
            EV <<"MecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
            scheduleAt(simTime() + processingTime, processMessage_);

        }
        else if (processMessage_->isScheduled() && !packetQueue_.isEmpty())
        {
            packetQueue_.insert(msg);
        }
        else
        {
            throw cRuntimeError("MecAppBase::handleMessage - This situation is not possible");
        }
    }
}


void MECPingPongApp::initialize(int stage)
{


    MecAppBase::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    if (stage == INITSTAGE_LOCAL)
           {

           }
    nbSentMsg = 0;
    //retrieving parameters


    
    // set Udp Socket
    ueSocket.setOutputGate(gate("socketOut"));
    localUePort = par("localUePort");
    ueSocket.bind(localUePort);
    maxCpuSpeed_ = par("maxCpuSpeed");
    //testing
    EV << "MECPingPongApp::initialize - Mec application "<< getClassName() << " with mecAppId["<< mecAppId << "] has started!" << endl;
    circle = new cOvalFigure("circle");




    Radius_ = par("radius");
    //LOGS
    App_name_ = par("name").stringValue();
    log_level = "low";

    auto ev = getSimulation()->getActiveEnvir();
    auto currentRun = ev->getConfigEx()->getActiveRunNumber();
   // auto nb_vehicles = par("nb_vehicles").stringValue();
    
    auto log_identifier = "logs/"+to_string(currentRun)+"/";
   // Use the create_directory function to create the directory
   if (mkdir(log_identifier.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        std::cout << "Directory created successfully.\n";
    } else {
        std::cerr << "Error creating directory\n";
    }

    csv_filename_total = fmt::format("{}/mec_total_rcv.csv",log_identifier);
    csv_filename_send_total = fmt::format("{}/mec_send_total.csv",log_identifier);
    csv_filename_arrival_time = fmt::format("{}/mec_arrivalTime.csv",log_identifier);
    csv_filename_sendnei_time = fmt::format("{}/mec_sendneiTime.csv",log_identifier);




    if (log_level=="high"){
    auto timestamp = std::time(nullptr);
    auto filename = fmt::format("{}/MECApp.txt",log_identifier);
    static int counter = 0;
    counter+=1;
    ofstream myfile;
    myfile.open(csv_filename_total, ios::app);
    std::ifstream file(csv_filename_total);
    if ( file.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                 myfile << "timestamp,arrivalTime,ueAppAddress,ueAppPort,car,recv_packet_size(B),uplink_delay(s),Queueing_delay(s),processing_delay(s),idframe,nbSentMsg,runNumber,sizeOfQueue" << endl;
               //myfile << "timestamp,arrivalTime,vehicle_id,port,packet_size(B),uplink_delay(s),processing_delay(s),idframe" << endl;
                 myfile.close();
                 row++ ;
             }
    }
    myfile.open(csv_filename_send_total, ios::app);
    std::ifstream file2(csv_filename_send_total);
    if ( file2.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                myfile << "timestamp,ueAppAddress,ueAppPort,car,packet_size_to_send(B),payload_size_to_send(B),runNumber" << endl;
                 //myfile << "timestamp,vehicle_id,port,packet_size(B),payload_size(B)" << endl;
                 myfile.close();
                 row++ ;
             }
    }
    myfile.open(csv_filename_arrival_time, ios::app);
    std::ifstream file3(csv_filename_arrival_time);
    if ( file3.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                myfile << "timestamp,arrivalTime,idFrame,uplink_delay,car,*Queue,runNumber,sizeOfQueue" << endl;
                 myfile.close();
                 row++ ;
             }
    }
    myfile.open(csv_filename_sendnei_time, ios::app);
    std::ifstream file4(csv_filename_sendnei_time);
    if ( file4.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                myfile << "timestamp,ueAppPort,destination,packet_size_to_send(B),payload_size_to_send(B), idFrame, distance, neig,runNumber" << endl;
                 myfile.close();
                 row++ ;
             }
    }

    }
    else 
    {

    //auto timestamp = std::time(nullptr);
    auto filename = fmt::format("{}/MECApp.txt",log_identifier);
    ofstream myfile;
    myfile.open(csv_filename_total, ios::app);
    std::ifstream file(csv_filename_total);
    if ( file.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                 myfile << "timestamp,arrivalTime,ueAppAddress,ueAppPort,car,recv_packet_size(B),uplink_delay(s),Queueing_delay(s),processing_delay(s),idframe,nbSentMsg,runNumber,sizeOfQueue" << endl;
               //myfile << "timestamp,arrivalTime,vehicle_id,port,packet_size(B),uplink_delay(s),processing_delay(s),idframe" << endl;
                 myfile.close();
                 row++ ;
             }
    }
    myfile.open(csv_filename_send_total, ios::app);
    std::ifstream file2(csv_filename_send_total);
    if ( file2.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                myfile << "timestamp,ueAppAddress,ueAppPort,car,packet_size_to_send(B),payload_size_to_send(B),runNumber" << endl;
                 //myfile << "timestamp,vehicle_id,port,packet_size(B),payload_size(B)" << endl;
                 myfile.close();
                 row++ ;
             }
    }
    myfile.open(csv_filename_arrival_time, ios::app);
    std::ifstream file3(csv_filename_arrival_time);
    if ( file3.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                myfile << "timestamp,arrivalTime,idFrame,uplink_delay,car,*Queue,runNumber,sizeOfQueue" << endl;
                 myfile.close();
                 row++ ;
             }
    }
    myfile.open(csv_filename_sendnei_time, ios::app);
    std::ifstream file4(csv_filename_sendnei_time);
    if ( file4.peek() == std::ifstream::traits_type::eof())
    {
        if(myfile.is_open())
             {
                myfile << "timestamp,ueAppPort,destination,packet_size_to_send(B),payload_size_to_send(B), idFrame, distance, neig,runNumber" << endl;
                 myfile.close();
                 row++ ;
             }
    }



    }

 

    //mp1Socket_ = addNewSocket();


    // connect with the service registry
    mp1Socket_ = MecAppBase::addNewSocket();

    cMessage *msg = new cMessage("connectMp1");
    scheduleAt(simTime() + 0, msg);
    // service
    //myLogger->info(fmt::format("initializated:  - {}",simTime().str()));


}

void MECPingPongApp::handleUeMessage(omnetpp::cMessage *msg)
{try {
    // determine its source address/port
    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<PingPongPacket>();
    EV << "MECPingPongApp::handleUeMessage **************** -" << mecPk->getType() << endl;
    if(strcmp(mecPk->getType(), START_WARNING) == 0)
    {
        /*
         * Read center and radius from message
         */
        EV << "MECPingPongApp::handleUeMessage - WarningStartPacket arrived" << endl;
        auto warnPk = dynamicPtrCast<const PingPongStartPacket>(mecPk);
        if(warnPk == nullptr)
            throw cRuntimeError("MECPingPongApp::handleUeMessage - WarningStartPacket is null");

        centerPositionX = warnPk->getCenterPositionX();
        centerPositionY = warnPk->getCenterPositionY();
        radius = warnPk->getRadius();

        if(par("logger").boolValue())
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] MECPingPongApp - Received message from UE, connecting to the Location Service\n";
                myfile.close();
            }
        }

        cMessage *m = new cMessage("connectService");
        scheduleAt(simTime()+0.005, m);
    }
    else if (strcmp(mecPk->getType(), STOP_WARNING) == 0)
    {
        //sendDeleteSubscription();
    }
    else if(!strcmp(msg->getName(), "PingPongPacket")) {
               
               //myLogger->info(fmt::format("handleMessage: Received Ping packet from UE - {}s ",simTime().str()));
               // determine its source address/port




                     emit( mecapp_pk_rcv_sizeSignal,   pk->getByteLength());
                     emit( mecapp_ul_delaySignal,  simTime() - pk->getCreationTime() );



                     sendPingPacket(msg);





                //myLogger->info(fmt::format("handleMessage: Processing time should be - {}s",computationTime(uniform(200,300))));


               //delete msg;
               return;
           }



    else
    {
        throw cRuntimeError("MECPingPongApp::handleUeMessage - packet not recognized");
    }
}
catch (const cRuntimeError& error){
        //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
        delete msg;
    }
}

void MECPingPongApp::handleSelfMessage(cMessage *msg)
{
    if(strcmp(msg->getName(), "connectMp1") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        connect(mp1Socket_, mp1Address, mp1Port);
        //Connect to the service ????? maybe add delay defaut 0.05
        cMessage *m = new cMessage("connectService");
        scheduleAt(simTime()+0.01, m);
    }

    else if(strcmp(msg->getName(), "connectService") == 0)
    {
        EV << "MecAppBase::handleMessage- " << msg->getName() << endl;
        if(!serviceAddress.isUnspecified() && serviceSocket_->getState() != inet::TcpSocket::CONNECTED)
        {
            connect(serviceSocket_, serviceAddress, servicePort);
        }
        else
        {
            if(serviceAddress.isUnspecified())
                EV << "MECWarningAlertApp::handleSelfMessage - service IP address is  unspecified (maybe response from the service registry is arriving)" << endl;
            else if(serviceSocket_->getState() == inet::TcpSocket::CONNECTED)
                EV << "MECWarningAlertApp::handleSelfMessage - service socket is already connected" << endl;
            auto nack = inet::makeShared<PingPongPacket>();
            // the connectService message is scheduled after a start mec app from the UE app, so I can
            //auto pkt_ = inet::makeShared<PingPongPacket>();
                     //pkt_->setType(ANS_PINGPONG);
                     //pkt_->setData("pong");
                     //pkt_->setChunkLength(inet::B(get_packet_dimension()));
                     //pkt_->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
                     //inet::Packet* pkt_to_send = new inet::Packet("PingPongPacket");
                     //pkt_to_send->insertAtBack(pkt_);
                     //ueSocket.sendTo(pkt_to_send, ueAppAddress, ueAppPort);
                     //scheduleAt(simTime()+computationTime(0),mssg); // response to her here
            nack->setType(START_NACK);
            nack->setChunkLength(inet::B(2));
            inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
            packet->insertAtBack(nack);
           // ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

//            throw cRuntimeError("service socket already connected, or service IP address is unspecified");
        }
    }


    delete msg;
}


void MECPingPongApp::finish(){
    MecAppBase::finish();
    EV << "MECPingPongApp::finish()" << endl;

    if(gate("socketOut")->isConnected()){

    }
}


void MECPingPongApp::sendPingPacketNeighboring(cMessage *msg) {
    //myLogger->info(fmt::format("handleMessage: sending ack/Pong packet - {}s",simTime().str()));
try{
       auto pk = check_and_cast<Packet *>(msg);
       ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
       ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();


        auto mecPk = pk->peekAtFront<PingPongPacket>();

    auto pubPk = dynamicPtrCast<const PingPongPacket>(mecPk);
    if (pubPk == nullptr)
        throw cRuntimeError("MECPingPongApp::handleUeMessage - PingPongPacket is null");



    // Conditoion on App_name_ ???
    auto allCars = getAllCars();
    int payload_; //default
    if (ueAppPort == 4000){ //td
        payload_ = 313;
        }
    else if  (ueAppPort ==  4003){ //cs
        payload_ = 313;
        }
    else if (ueAppPort == 4005){ //ca
        payload_ = 313;
        }
    else if (ueAppPort==4006){ //cm
        payload_ = 313;
       }
    else {
        payload_=1 ; //default
    }


    cModule* mymodule = L3AddressResolver().findHostWithAddress(ueAppAddress);
    //inet::Coord myPosition = utility::getMyCurrentPosition_(mymodule);
    inet::IMobility *mobility_ = check_and_cast<inet::IMobility *>(mymodule->getSubmodule("mobility"));
    inet::Coord myPosition =  mobility_->getCurrentPosition();

    auto neighborCars = getNeighbors(allCars, Radius_,mymodule);
    for (auto it=neighborCars.begin(); it!=neighborCars.end();it++){

        if(circle != nullptr)
                    {
                if (getSimulation()->getSystemModule()->getCanvas()->findFigure(circle) != -1)
                {
                    getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);
                }
                    }


        inet::UdpSocket tmp;
        inet::Coord coord = getCoordinates(*it);
        inet::L3Address destadd  = L3AddressResolver().resolve(getModule_(*it)->getFullName());
        int destPort = ueAppPort;

        //utility::showCircle( Radius_, myPosition, mymodule, circle );

        circle->setBounds(cFigure::Rectangle(myPosition.x - Radius_, myPosition.y - Radius_,Radius_*2,Radius_*2));
        circle->setLineWidth(2);
        circle->setLineColor(cFigure::RED);
        getSimulation()->getSystemModule()->getCanvas()->addFigure(circle);


             // sending Ping to UE application
       auto pkt_ = inet::makeShared<PingPongPacket>();
       int idFrame__ =  pubPk->getIDframe(); 
       pkt_->setIDframe(idFrame__);
       pkt_->setType(ANS_PINGPONG);
       pkt_->setData(L3AddressResolver().findHostWithAddress(ueAppAddress)->getFullName());

       pkt_->setChunkLength(inet::B(payload_));

       pkt_->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
       inet::Packet* pkt_to_send = new inet::Packet("PingPongPacket");
       pkt_to_send->insertAtBack(pkt_);

    //myLogger->info(fmt::format("1111Sending to destadd {}///destPort {}///",destadd.str(),destPort));


       ueSocket.sendTo(pkt_to_send, destadd, destPort);

        auto ev = getSimulation()->getActiveEnvir();
        auto currentRun = ev->getConfigEx()->getActiveRunNumber();

       ofstream myfile;
       myfile.open (csv_filename_sendnei_time, ios::app);
       if(myfile.is_open())
            {
           //myfile << "timestamp,ueAppPort,destination,packet_size_to_send(B),payload_size_to_send(B), idFrame, distance" << endl;

                myfile  << simTime()  << "," << ueAppPort <<","<< getModule_(*it)->getFullName() << "," << pkt_to_send->getByteLength() << "," << payload_ << ","<< idFrame__<<","<< coord.distance(myPosition)<<","<< neighborCars.size() <<","<<currentRun << endl;
                myfile.close();

            }


        EV << "sendPingPacketNeighboring" << endl;

    }}
            catch (const cRuntimeError& error){
               // myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                delete msg;
            }

}


void MECPingPongApp::sendPingPacket(cMessage *msg) {
    try{
    sendPingPacketNeighboring(msg);
    //myLogger->info(fmt::format("handleMessage: sending ack/Pong packet - {}s",simTime().str()));

    auto pk = check_and_cast<Packet *>(msg);
    ueAppAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    ueAppPort = pk->getTag<L4PortInd>()->getSrcPort();

    auto mecPk = pk->peekAtFront<PingPongPacket>();

    auto pubPk = dynamicPtrCast<const PingPongPacket>(mecPk);
    if (pubPk == nullptr)
        throw cRuntimeError("MECPingPongApp::handleUeMessage - PingPongPacket is null");

    //check sender name
        std::string car =L3AddressResolver().findHostWithAddress(ueAppAddress)->getFullName();

    // sending Ping to UE application
    auto pkt_ = inet::makeShared<PingPongPacket>();

    pkt_->setIDframe(pubPk->getIDframe());
    pkt_->setType(ANS_PINGPONG);
    pkt_->setData("pong");
   //pkt_->setChunkLength(inet::B(get_packet_dimension()));

    int payload_; //default
    if (ueAppPort == 4000){ //td
        payload_ = 313;
        }
    else if  (ueAppPort ==  4003){ //cs
        payload_ = 313;
        }
    else if (ueAppPort == 4005){ //ca
        payload_ = 313;
        }
    else if (ueAppPort==4006){ //cm
        payload_ = 313;
       }
    else {
        payload_=1 ; //default
    }

    pkt_->setChunkLength(inet::B(payload_));

    pkt_->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    inet::Packet* pkt_to_send = new inet::Packet("PingPongPacket");
    pkt_to_send->insertAtBack(pkt_);
    //myLogger->info(fmt::format("222222Sending to ueAppAddress {}///ueAppPort {}///",ueAppAddress.str(),ueAppPort));

    ueSocket.sendTo(pkt_to_send, ueAppAddress, ueAppPort);


    auto ev = getSimulation()->getActiveEnvir();
    auto currentRun = ev->getConfigEx()->getActiveRunNumber();

    ofstream myfile;
    myfile.open (csv_filename_send_total, ios::app);
    if(myfile.is_open())
         {
             //myfile << "timestamp,ueAppAddress,ueAppPort,packet_size_to_send(B),payload_size_to_send(B)" << endl;
             myfile  << simTime() << "," << ueAppAddress << "," << ueAppPort <<","<< car << "," << pk->getByteLength() << "," << payload_ <<","<<currentRun << endl;
             myfile.close();

         }

    }
            catch (const cRuntimeError& error){
                //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                delete msg;
            }

}




void MECPingPongApp::modifySubscription()
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                       "\"callbackReference\" : {"
                        "\"callbackData\":\"1234\","
                        "\"notifyURL\":\"example.com/notification/1234\"},"
                       "\"checkImmediate\": \"false\","
                        "\"address\": \"" + ueAppAddress.str()+ "\","
                        "\"clientCorrelator\": \"null\","
                        "\"enteringLeavingCriteria\": \"Leaving\","
                        "\"frequency\": 5,"
                        "\"radius\": " + std::to_string(radius) + ","
                        "\"trackingAccuracy\": 10,"
                        "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
                        "\"longitude\": " + std::to_string(centerPositionY) + ""        // as y
                        "}"
                        "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());
    Http::sendPutRequest(serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MECPingPongApp::sendSubscription()
{
    std::string body = "{  \"circleNotificationSubscription\": {"
                           "\"callbackReference\" : {"
                            "\"callbackData\":\"1234\","
                            "\"notifyURL\":\"example.com/notification/1234\"},"
                           "\"checkImmediate\": \"false\","
                            "\"address\": \"" + ueAppAddress.str()+ "\","
                            "\"clientCorrelator\": \"null\","
                            "\"enteringLeavingCriteria\": \"Entering\","
                            "\"frequency\": 5,"
                            "\"radius\": " + std::to_string(radius) + ","
                            "\"trackingAccuracy\": 10,"
                            "\"latitude\": " + std::to_string(centerPositionX) + ","           // as x
                            "\"longitude\": " + std::to_string(centerPositionY) + ""        // as y
                            "}"
                            "}\r\n";
    std::string uri = "/example/location/v2/subscriptions/area/circle";
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());

    if(par("logger").boolValue())
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] MECPingPongApp - Sent POST circleNotificationSubscription the Location Service \n";
            myfile.close();
        }
    }

    Http::sendPostRequest(serviceSocket_, body.c_str(), host.c_str(), uri.c_str());
}

void MECPingPongApp::sendDeleteSubscription()
{
    std::string uri = "/example/location/v2/subscriptions/area/circle/" + subId;
    std::string host = serviceSocket_->getRemoteAddress().str()+":"+std::to_string(serviceSocket_->getRemotePort());
    Http::sendDeleteRequest(serviceSocket_, host.c_str(), uri.c_str());
}

void MECPingPongApp::established(int connId)
{
    if(connId == mp1Socket_->getSocketId())
    {
        EV << "MECPingPongApp::established - Mp1Socket"<< endl;
    ///    // get endPoint of the required service
        const char *uri = "/example/mec_service_mgmt/v1/services?ser_name=LocationService";
        std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());

        Http::sendGetRequest(mp1Socket_, host.c_str(), uri);
        return;
    }
    else if (connId == serviceSocket_->getSocketId())
    {
        EV << "MECPingPongApp::established - serviceSocket"<< endl;
       // the connectService message is scheduled after a start mec app from the UE app, so I can
        // response to her here, once the socket is established
   ///     auto ack = inet::makeShared<WarningAppPacket>();
   ///     ack->setType(START_ACK);
  ///      ack->setChunkLength(inet::B(2));
  ///      inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
  ///      packet->insertAtBack(ack);
  ///      ueSocket.sendTo(packet, ueAppAddress, ueAppPort);
        //sendSubscription();
        return;
    }
    else
    {
       throw cRuntimeError("MecAppBase::socketEstablished - Socket %s not recognized", connId);
    }
}


void MECPingPongApp::handleHttpMessage(int connId)
{
    if (mp1Socket_ != nullptr && connId == mp1Socket_->getSocketId()) {
       handleMp1Message(connId);
    }
    else
    {
       handleServiceMessage(connId);
    }
}


void MECPingPongApp::handleMp1Message(int connId)
{
    EV << "MECPingPongApp::handleMp1Message - payload: " << mp1HttpMessage->getBody() << endl;

    try
    {
        nlohmann::json jsonBody = nlohmann::json::parse(mp1HttpMessage->getBody()); // get the JSON structure
        if(!jsonBody.empty())
        {
            jsonBody = jsonBody[0];
            std::string serName = jsonBody["serName"];
            if(serName.compare("LocationService") == 0)
            {
                if(jsonBody.contains("transportInfo"))
                {
                    nlohmann::json endPoint = jsonBody["transportInfo"]["endPoint"]["addresses"];
                    EV << "address: " << endPoint["host"] << " port: " <<  endPoint["port"] << endl;
                    std::string address = endPoint["host"];
                    serviceAddress = L3AddressResolver().resolve(address.c_str());;
                    servicePort = endPoint["port"];
                }
            }
            else
            {
                EV << "MECPingPongApp::handleMp1Message - LocationService not found"<< endl;
                serviceAddress = L3Address();
            }
        }

    }
    catch(nlohmann::detail::parse_error e)
    {
        EV <<  e.what() << std::endl;
        // body is not correctly formatted in JSON, manage it
        return;
    }

}

void MECPingPongApp::handleServiceMessage(int connId)
{
    if(serviceHttpMessage->getType() == REQUEST)
    {
        Http::send204Response(serviceSocket_); // send back 204 no content

        nlohmann::json jsonBody;
        EV << "MEClusterizeService::handleTcpMsg - REQUEST " << serviceHttpMessage->getBody()<< endl;
        try
        {
           jsonBody = nlohmann::json::parse(serviceHttpMessage->getBody()); // get the JSON structure
        }
        catch(nlohmann::detail::parse_error e)
        {
           std::cout  <<  e.what() << std::endl;
           // body is not correctly formatted in JSON, manage it
           return;
        }

        if(jsonBody.contains("subscriptionNotification"))
        {
            if(jsonBody["subscriptionNotification"].contains("enteringLeavingCriteria"))
            {
                nlohmann::json criteria = jsonBody["subscriptionNotification"]["enteringLeavingCriteria"] ;
                auto alert = inet::makeShared<PingPongWarningPacket>();
                alert->setType(WARNING_ALERT);

                if(criteria == "Entering")
                {
                    EV << "MEClusterizeService::handleTcpMsg - Ue is Entered in the danger zone "<< endl;
                    alert->setDanger(true);

                    if(par("logger").boolValue())
                    {
                        ofstream myfile;
                        myfile.open ("example.txt", ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MECPingPongApp - Received circleNotificationSubscription notification from Location Service. UE's entered the red zone! \n";
                            myfile.close();
                        }
                    }

                    // send subscription for leaving..
                   // modifySubscription();

                }
                else if (criteria == "Leaving")
                {
                    EV << "MEClusterizeService::handleTcpMsg - Ue left from the danger zone "<< endl;
                    alert->setDanger(false);
                    if(par("logger").boolValue())
                    {
                        ofstream myfile;
                        myfile.open ("example.txt", ios::app);
                        if(myfile.is_open())
                        {
                            myfile <<"["<< NOW << "] MEWarningAlertApp - Received circleNotificationSubscription notification from Location Service. UE's exited the red zone! \n";
                            myfile.close();
                        }
                    }
                    //sendDeleteSubscription();
                }

                alert->setPositionX(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["x"]);
                alert->setPositionY(jsonBody["subscriptionNotification"]["terminalLocationList"]["currentLocation"]["y"]);
                alert->setChunkLength(inet::B(20));
                alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

                inet::Packet* packet = new inet::Packet("WarningAlertPacketInfo");
                packet->insertAtBack(alert);
                //myLogger->info(fmt::format("333333Sending to ueAppAddress {}///ueAppPort {}///",ueAppAddress.str(),ueAppPort));

                ueSocket.sendTo(packet, ueAppAddress, ueAppPort);

            }
        }
    }
    else if(serviceHttpMessage->getType() == RESPONSE)
    {
        HttpResponseMessage *rspMsg = dynamic_cast<HttpResponseMessage*>(serviceHttpMessage);

        if(rspMsg->getCode() == 204) // in response to a DELETE
        {
            EV << "MEClusterizeService::handleTcpMsg - response 204, removing circle" << rspMsg->getBody()<< endl;
            serviceSocket_->close();
             getSimulation()->getSystemModule()->getCanvas()->removeFigure(circle);

        }
        else if(rspMsg->getCode() == 201) // in response to a POST
        {
            nlohmann::json jsonBody;
            EV << "MEClusterizeService::handleTcpMsg - response 201 " << rspMsg->getBody()<< endl;
            try
            {
               jsonBody = nlohmann::json::parse(rspMsg->getBody()); // get the JSON structure
            }
            catch(nlohmann::detail::parse_error e)
            {
               EV <<  e.what() << endl;
               // body is not correctly formatted in JSON, manage it
               return;
            }
            std::string resourceUri = jsonBody["circleNotificationSubscription"]["resourceURL"];
            std::size_t lastPart = resourceUri.find_last_of("/");
            if(lastPart == std::string::npos)
            {
                EV << "1" << endl;
                return;
            }
            // find_last_of does not take in to account if the uri has a last /
            // in this case subscriptionType would be empty and the baseUri == uri
            // by the way the next if statement solve this problem
            std::string baseUri = resourceUri.substr(0,lastPart);
            //save the id
            subId = resourceUri.substr(lastPart+1);
            EV << "subId: " << subId << endl;

            circle = new cOvalFigure("circle");
            circle->setBounds(cFigure::Rectangle(centerPositionX - radius, centerPositionY - radius,radius*2,radius*2));
            circle->setLineWidth(2);
            circle->setLineColor(cFigure::RED);
            getSimulation()->getSystemModule()->getCanvas()->addFigure(circle);

        }
    }

}


void MECPingPongApp::handleProcessedMessage(cMessage *msg)
{

    if (msg->isSelfMessage())
    {
        handleSelfMessage(msg);
        return;
    }
    if (!msg->isSelfMessage()){
        if(ueSocket.belongsToSocket(msg))
        {
            handleUeMessage(msg);
            delete msg;
            return;
        }
    }
    else {
        throw cRuntimeError("MECPingPongApp::handleprocessedmessage - not recognized message name %s",msg->getName());
    }
   // MECPingPongApp::handleProcessedMessage(msg);

}





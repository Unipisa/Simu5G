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
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"

#include "./packets/PingPongPacket_Types.h"


#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include <fstream>
#include "UEPingPongApp.h"



#include "spdlog/spdlog.h"  // logging library
#include "spdlog/sinks/basic_file_sink.h"
#include <ctime>
#include <fmt/format.h>

#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <map>

#include "veins_inet/veins_inet.h"
#include "veins_inet/VeinsInetApplicationBase.h"
#include <iostream>
#include <random>
#include <iostream>
#include <sys/stat.h>


#include "CommonFunctions.h"
using namespace inet;
using namespace std;
using namespace veins;
//using namespace utility;



Define_Module(UEPingPongApp);
simsignal_t UEPingPongApp::failed_to_start_mecApp_Signal =registerSignal("failed_to_start_mecApp_Signal") ;



UEPingPongApp::UEPingPongApp(){
    selfStart_ = NULL;
    selfStop_ = NULL;
}

UEPingPongApp::~UEPingPongApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);

}

void UEPingPongApp::initialize(int stage)
{


    EV << "UEPingPongApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    if (stage == INITSTAGE_LOCAL)
       {
        //Instantiation of parameters

        mecApp_on = 0;
        nbRcvMsg = 0;


       }

    // initialize pointers to other modules
    if (FindModule<TraCIMobility*>::findSubModule(getParentModule())) {
        mobility = TraCIMobilityAccess().get(getParentModule());
        traci = mobility->getCommandInterface();
    }

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");
    selfMecAppStart_ = new cMessage("selfMecAppStart");
    selfSendPing_ = new cMessage("selfSendPing");

    size_ = 1;



    mecAppName = par("mecAppName").stringValue();
    //retrieve parameters
    localPort_ = par("localPort");
 
    UE_name_ = par("deviceAppAddress").stringValue();
    log = false;
    maxCpuSpeed_ = par("maxCpuSpeed");
    App_name_ = par("name").stringValue();


    auto timestamp = std::time(nullptr);
    static int counter = 0;

    //log_identifier = par('mecAppName').stringValue();
    auto ev = getSimulation()->getActiveEnvir();
    auto currentRun = ev->getConfigEx()->getActiveRunNumber();    
    auto log_identifier = "logs/"+to_string(currentRun)+"/";
    // Use the create_directory function to create the directory
   if (mkdir(log_identifier.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        std::cout << "Directory created successfully.\n";
    } else {
        std::cerr << "Error creating directory\n";
    }
    
    myLogger = spdlog::basic_logger_mt(fmt::format("UELogger{}_{}", App_name_,counter++), fmt::format("{}/UEApp.txt",log_identifier));



    Fails_Logger = spdlog::basic_logger_mt(fmt::format("ue_fails_Logger{}_{}", App_name_,counter++), fmt::format("{}/fails.txt",log_identifier));

    //How to get any parameter ?
    //int n =  getNumParams();
   // for (int i = 0; i < n; i++)
      //  {


        //myLogger->info(fmt::format("@ Name ={}", p.getName()));
        //myLogger->info(fmt::format("@ Type ={}", cPar::getTypeName(p.getType())));
        //myLogger->info(fmt::format("@ contains ={}", p.str()));
      //  }





    payloadSize_ = par("payloadSize").doubleValue();
   //


    period_ = par("period");


    deviceAppPort_ = par("deviceAppPort");
    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    deviceSimbolicAppAddress_ = (char*)par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceSimbolicAppAddress_);


    csv_filename_rcv_total = fmt::format("{}/ue_rcv_total.csv",log_identifier);
    csv_filename_send_total = fmt::format("{}/ue_send_total.csv",log_identifier);
    log_level = "low";
    if (log_level=="high"){
 
    
    ofstream myfile;
   
    myfile.open (csv_filename_rcv_total, ios::app);
    std::ifstream file(csv_filename_rcv_total);
        if ( file.peek() == std::ifstream::traits_type::eof()) {
      if(myfile.is_open())
      {
          myfile << "timestamp,idFrame,downlink_delay(s),deviceSimbolicAppAddress_,speed,roadID,AppName,ueAppPort,nbRcvMsg, from,nb_neighbor,currentPosition,runNumber,myposition" << endl;
          myfile.close();
      }
        }
    
    
    myfile.open (csv_filename_send_total, ios::app);
    std::ifstream file2(csv_filename_send_total);
        if ( file2.peek() == std::ifstream::traits_type::eof()) {
      if(myfile.is_open())
      {
        
          myfile << "timestamp,interRequestTime(s),payloadsize(B),packetsize(B),deviceAppAddress_,car_sending,ueIP,speed,roadID,laneDensity,AppName,localPort_,idframe,currentPosition,runNumber" << endl;

          myfile.close();
      }
        }

    }
    else
     {
    csv_filename_rcv_total = fmt::format("{}/ue_rcv_total.csv",log_identifier);
    
    ofstream myfile;
   
    myfile.open (csv_filename_rcv_total, ios::app);
    std::ifstream file(csv_filename_rcv_total);
        if ( file.peek() == std::ifstream::traits_type::eof()) {
      if(myfile.is_open())
      {
          myfile << "timestamp,idFrame,downlink_delay(s),deviceSimbolicAppAddress_,speed,roadID,AppName,ueAppPort,nbRcvMsg, from,nb_neighbor,currentPosition,runNumber,myposition" << endl;
          myfile.close();
      }
        }
            myfile.open (csv_filename_send_total, ios::app);
    std::ifstream file2(csv_filename_send_total);
        if ( file2.peek() == std::ifstream::traits_type::eof()) {
      if(myfile.is_open())
      {
        
          myfile << "timestamp,interRequestTime(s),payloadsize(B),packetsize(B),deviceAppAddress_,car_sending,ueIP,speed,roadID,laneDensity,AppName,localPort_,idframe,currentPosition,runNumber" << endl;

          myfile.close();
      }
        }

    }


    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);
    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    //retrieving car cModule
    ue = this->getParentModule();

    


    //starting UEWarningAlertApp
    simtime_t startTime = par("startTime");
    EV << "UEPingPongApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEPingPongApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEPingPongApp::initialize - destAddress: " << deviceSimbolicAppAddress_ << " [" << deviceAppAddress_.str()  <<"]"<< endl;
    EV << "UEPingPongApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;
    //myLogger->info(fmt::format("initialized:  - {}",simTime().str()));

}

void UEPingPongApp::handleMessage(cMessage *msg)
{
    //myLogger->info(fmt::format("handleMessage: received {}; isSelf? {}", msg->getName(), msg->isSelfMessage()));

    EV << "UEPingPongApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {

        if(!strcmp(msg->getName(), "selfStart"))   sendStartMEWarningAlertApp();

             else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

             else if(!strcmp(msg->getName(), "selfMecAppStart"))
             {
                 scheduleAt(simTime() + period_, selfMecAppStart_);
             }


        else if(!strcmp(msg->getName(), "selfSendPing")) {
            double time_to_wait_d ;
            string name = par("name").stringValue();

            if (name == "TeleoperatedDriving"){
                time_to_wait_d= exponentialGenerator(100);
                
                }
            else if (name ==  "CooperativeSensing"){
                time_to_wait_d= exponentialGenerator(100);
                }
            else if (name == "CooperativeAwarness"){
                time_to_wait_d= exponentialGenerator(10);
                }
            else if (name == "CooperativeManeuver"){
                time_to_wait_d= exponentialGenerator(10);
                }
            else {
                //time_to_wait_d = par("interReqTime").doubleValue();
                time_to_wait_d = 0.1;
            }

            simtime_t time_to_wait {time_to_wait_d};

            //myLogger->info(fmt::format("UEPingPongApp::handleMessage- Ping sent at  {}s will wait {}s to send again",simTime().str(),time_to_wait.str()));
            sendPingPacket(time_to_wait);



            simtime_t  stopTime = par("stopTime");
///////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////
            ////////////////////////
            //if (stopTime - simTime()- time_to_wait > 0){
            cancelEvent(selfSendPing_);
            scheduleAt(simTime() + time_to_wait, selfSendPing_);
        }

        else    throw cRuntimeError("UEPingPongApp::handleMessage - \tWARNING: Unrecognized self message",msg->getName());

    }
    // Receiver Side
    else{

        

            try {
                auto ret = dynamic_cast<inet::Packet*>(msg);
                if (!ret){
                           // myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
                //myLogger->info(fmt::format("Message Name {} className {}   kind{}///",msg->getName(), msg->getClassName(),msg->getKind()));
                            delete msg;
                            return; 

                        }
                        //myLogger->info(fmt::format("1{} {} {}1111111111111111111111111111111111111111111111111111111111111111111111",msg->getName(), msg->getClassName(),msg->getKind()));
               // myLogger->info(fmt::format("Message Name {} className {}   kind{}///",msg->getName(), msg->getClassName(),msg->getKind()));

                        //myLogger->info(fmt::format("Message Name {} className {}///",msg->getName(), msg->getClassName()));
                        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
                       // myLogger->info(fmt::format("2222222222222222222222222222222222222222222222222222222222222222222"));

//////////////////////////////////////////////////////////////////////////////////////////////////////////



            inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();
            int port = packet->getTag<L4PortInd>()->getSrcPort();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if(ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) // dev app
        {
            ////////////////////////////////////////
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();

            if (mePkt == 0)
                throw cRuntimeError("UEPingPongApp::handleMessage - \tFATAL! Error when casting to DeviceAppPacket");

            if( !strcmp(mePkt->getType(), ACK_START_MECAPP) )    handleAckStartMEWarningAlertApp(msg);

            else if(!strcmp(mePkt->getType(), ACK_STOP_MECAPP))  handleAckStopMEWarningAlertApp(msg);

            else
            {
                throw cRuntimeError("UEPingPongApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
            }
        }
        // From MEC application
        else
        {
            //myLogger->info(fmt::format("Received from MecAPP: messagetype {}", msg->getName()));
            if (!strcmp(msg->getName(), "PingPongPacket"))
            {

                //myLogger->info(fmt::format("UEPingPongApp::handleMessage- Ack received from MECApp at {}s",simTime().str()));
                //myLogger->info(fmt::format(" YES Received from MecAPP: messagetype {}", msg->getName()));


                auto mecPk = packet->peekAtFront<PingPongPacket>();
                auto pubPk = dynamicPtrCast<const PingPongPacket>(mecPk);

                auto myposition = getMyCurrentPosition_(ue);
                
                auto nbNei =   getNeighbors(getAllCars(), par("radius"), ue).size();

                //auto mecPk = packet->peekAtFront<PingPongPacket>();
                //auto pubPk = dynamicPtrCast<const PingPongPacket>(packet);
                auto ev = getSimulation()->getActiveEnvir();
                auto currentRun = ev->getConfigEx()->getActiveRunNumber();
                ofstream myfile;
                myfile.open (csv_filename_rcv_total, ios::app);
                      if(myfile.is_open())
                      {
                        //myfile << "timestamp,idFrame,downlink_delay(s),deviceAppAddress_,deviceSimbolicAppAddress_,ueIP,speed,roadID,name,ueappport,nbRcvMsg" << endl;

                          myfile << simTime() <<","<< pubPk->getIDframe() << "," << simTime() - packet->getCreationTime() <<","  <<deviceSimbolicAppAddress_ <<","<<mobility->getSpeed() << "," << mobility->getRoadId().c_str()<<","<<par("name").stringValue() <<","<<  par("localPort").intValue()<<","<< nbRcvMsg++<<"," << pubPk->getData()<<","<< nbNei<<","<<"mobility->position"<<","<< currentRun <<","<< myposition <<endl;
                          myfile.close();
                      }



            }




        }
        delete msg;

                    } catch (const cRuntimeError& error){
                   // myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
                  //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                  delete msg;
                  return; 
            } catch (const cRuntimeError error){
                    //myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
                  //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                  delete msg;
                  return; 
            }
            catch (...) {
                
                  delete msg;
                  return; 
            }


    }




}

void UEPingPongApp::finish()
{

}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEPingPongApp::sendStartMEWarningAlertApp()
{

    mobility = TraCIMobilityAccess().get(getParentModule());
   // traci = mobility->getCommandInterface();
    //traciVehicle = mobility->getVehicleCommandInterface();

    //myLogger->info(fmt::format("Attempt to strat mecApp:  - {}",simTime().str()));
    EV << "UEPingPongApp::START MECAPPPP 0000000000000000000000000000000000000"<< endl;


    inet::Packet* packet = new inet::Packet("PingPongPacketStart");

    auto start = inet::makeShared<DeviceAppStartPacket>();




    start->setType(START_MECAPP);


    start->setMecAppName(mecAppName.c_str());
    //start->setMecAppProvider("lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest_External");

    start->setChunkLength(inet::B(2+mecAppName.size()+1));

    //instantiation requirements and info
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(start);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEPingPongApp - UE sent start message to the Device App \n";
            myfile.close();

        }
    }

    //rescheduling
    cancelEvent(selfStart_);
    scheduleAt(simTime() + period_, selfStart_);
    EV << "UEPingPongApp::START MECAPPPP 11111111111111111111111111111"<< endl;

}

void UEPingPongApp::sendStopMEWarningAlertApp()
{
    EV << "UEPingPongApp::   - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);

    stop->setChunkLength(inet::B(size_));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent stop message to the Device App \n";
            myfile.close();
        }
    }

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
    mecApp_on = 0;
    emit( failed_to_start_mecApp_Signal,  mecApp_on );

}
/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UEPingPongApp::handleAckStartMEWarningAlertApp(cMessage* msg)
{
    try{

    inet::Packet* packet = check_and_cast<Packet *>(msg);

    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if(pkt->getResult() == true)
    {
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UEPingPongApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket. mecApp isntance is at: "<< mecAppAddress_<< ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if(!selfStop_->isScheduled()){
            simtime_t  stopTime = par("stopTime");
            //scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UEPingPongApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
        }
        //myLogger->info(fmt::format("UEPingPongApp::handleMessage- Will trigger sending of Ping messages - {}s",simTime().str()));
        cancelEvent(selfSendPing_);
        scheduleAt(simTime(), selfSendPing_);
        mecApp_on = 1;
        emit( failed_to_start_mecApp_Signal,  mecApp_on );
    }

    else
    {
        EV << "UEPingPongApp::handleAckStartMEWarningAlertApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
        if ( !strcmp(pkt->getReason(), "LCM proxy responded 500")){
            //throw cRuntimeError("test");

            nb_fails+=1;
            //ofstream myfile;
            //myfile.open (csv_filename_fails, ios::app);
           //                if(myfile.is_open())
             //              {
             //                  //myfile << simTime() << "," << App_name_ << "," <<  nb_fails  << endl;
             //                  myfile << simTime() << endl;
              //                 myfile.close();
             ///              }
           // myLogger->info(fmt::format("UEPingPongApp::FailToStart -at  {}s -- nb {} ",simTime().str(),nb_fails));
            mecApp_on= 0;
            emit( failed_to_start_mecApp_Signal,  mecApp_on );

            if (nb_fails>2){throw cRuntimeError("test"); }
        }

        // Should Try again
        //////////////////////////////
        //-2();
        //scheduleAt(simTime() + 1s, sendStartMEWarningAlertApp());
    }

    //sendMessageToMECApp();
    //cancelEvent(selfMecAppStart_);
    //scheduleAt(simTime() + period_, selfMecAppStart_);
    }
    catch (const cRuntimeError& error){
                            //myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

    //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
    delete msg;
    return; 
        }
           catch (const cRuntimeError error){
                            //myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

    //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
    delete msg;
    return; 
        }
}



void UEPingPongApp::sendPingPacket(simtime_t interReqTime) {



    //
    // sending Ping to MEC application
    inet::Packet* pkt2 = new inet::Packet("PingPongPacket");
    auto alert2 = inet::makeShared<PingPongPacket>();
    alert2->setType(START_PINGPONG);
    alert2->setData("ping");

    //iDframe_ = ++iDframe_;
    static unsigned int  iDframe_ = 0;
    iDframe_++;
    alert2->setIDframe(iDframe_);

    string name = par("name").stringValue();
    int payload;
    if (name == "TeleoperatedDriving"){
       
        payload = exponentialGenerator(1.0 / 40000);
        
        }

    else if (name ==  "CooperativeSensing"){
    
        payload =exponentialGenerator(1.0 / 2500);//add density
        //payload =1;
        }

    else if (name == "CooperativeAwarness"){
 
        payload =exponentialGenerator(1.0 / 1500);
        }

    else if (name == "CooperativeManeuver"){
        payload =exponentialGenerator(1.0 / 16250);
        }

    else {

        
        payload = exponentialGenerator(1.0 / 40000);
        
        //payload = payloadSize_ ;
        //payload = exponentialGenerator(0.0001) ;
        
    }
    payload = (payload>65527)? 65527:payload;
    payload = (payload<1)? 1:payload;
    alert2->setChunkLength(inet::B(payload));
    ue->bubble("sending");
    alert2->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt2->insertAtBack(alert2);

    auto ev = getSimulation()->getActiveEnvir();
    auto currentRun = ev->getConfigEx()->getActiveRunNumber();

    ofstream myfile;
    myfile.open (csv_filename_send_total, ios::app);
    if(myfile.is_open())
       {
           //myfile << "timestamp,interRequestTime(s),payloadsize(B),packetsize(B),deviceAppAddress_,deviceSimbolicAppAddress_,ueIP,speed,roadID,laneDensity,appName,ueAppport" << endl;
           myfile << simTime() <<","<< interReqTime << "," << payload<< ","<<  pkt2->getByteLength() <<"," <<deviceAppAddress_<<"," <<deviceSimbolicAppAddress_ << "," <<     inet::L3AddressResolver().resolve(sourceSimbolicAddress) << ","<<"traciVehicle->getSpeed()" << "," << "mobility->getRoadId().c_str()"<< ","<< "laneDensity"<<","<< name << ","<< par("localPort").intValue()<< "," << iDframe_ <<"," <<"currentPosition" <<","<<currentRun << endl;
           myfile.close();
       }
    

    socket.sendTo(pkt2, mecAppAddress_, mecAppPort_);

    //myLogger->info("@ Sent ping to the MEC");
}

void UEPingPongApp::handleInfoMEWarningAlertApp(cMessage* msg)
{

try {
    inet::Packet* packet = check_and_cast<Packet *>(msg);
    auto pkt = packet->peekAtFront<PingPongPacket>();

    EV << "UEPingPongApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket"<< endl;

    //updating runtime color of the car icon background

        if(log)
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] UEPingPongApp - UE received danger alert FALSE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UEPingPongApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        //ue->getDisplayString().setTagArg("i",1, "green");

        }
              catch (const cRuntimeError& error){
                                    //myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

                  //myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                  delete msg;
        }

}
void UEPingPongApp::handleAckStopMEWarningAlertApp(cMessage* msg)
{
try {
    inet::Packet* packet = check_and_cast<Packet *>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UEPingPongApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket with result: "<< pkt->getResult() << endl;
    if(pkt->getResult() == false)
        EV << "Reason: "<< pkt->getReason() << endl;
    //updating runtime color of the car icon background
    //ue->getDisplayString().setTagArg("i",1, "white");
    mecApp_on = 0;
    emit( failed_to_start_mecApp_Signal,  mecApp_on );


    cancelEvent(selfStop_);
}
        
              catch (const cRuntimeError& error){
                                   // myLogger->info(fmt::format("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

                 // myLogger->info(fmt::format("ERROR {}///{}///{}///{}///{}///{}///{}///",error.what(),error.what(),error.what(),error.what(),error.what(),error.what(),error.what()));
                  delete msg;
                  return; 
        }
}




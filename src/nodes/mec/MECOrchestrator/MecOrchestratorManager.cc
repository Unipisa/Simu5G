/*
 * MecOrchestratorManager.cc
 *
 *  Created on: Apr 26, 2021
 *      Author: linofex
 */

#include "nodes/mec/MECOrchestrator/MecOrchestratorManager.h"
#include "nodes/mec/VirtualisationInfrastructure/ResourceManager.h"
#include "nodes/mec/VirtualisationInfrastructure/VirtualisationManager.h"
#include "nodes/mec/MecCommon.h"
#include "nodes/mec/MECOrchestrator/MECOpackets/MECOrchestratorMessages_m.h"


Define_Module(MecOrchestratorManager);

MecOrchestratorManager::MecOrchestratorManager()
{
}

void MecOrchestratorManager::initialize(int stage)
{
    EV << "MecOrchestratorManager::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    //------------------------------------
    // Binder module
    binder_ = getBinder();
    //------------------------------------
    //communication with UE APPs
    destPort_ = par("destPort");
    localPort_ = par("localPort");
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);
    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);
    //------------------------------------

    //------------------------------------
    interfaceTableModule = par("interfaceTableModule").stringValue();

    getConnectedMecHosts();

}

void MecOrchestratorManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if(strcmp(msg->getName(), "MECOrchestratorMessage") == 0)
        {
            MECOrchestratorMessage* meoMsg = check_and_cast<MECOrchestratorMessage*>(msg);
            ackMEAppPacket(meoMsg->getUeAppID(), meoMsg->getType());
        }
        delete msg;
        return;

    }



//    //handling resource allocation confirmation
//    if(msg->arrivedOn("resourceManagerIn"))
//    {
//        EV << "MecOrchestratorManager::handleMessage - received message from ResourceManager" << endl;
//        handleResource(msg);
//    }
//    //handling MEAppPacket between UEApp and MEApp
//    else
//    {
      inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
        auto mePkt = packet->peekAtFront<MEAppPacket>();
        if(mePkt == 0)
        {
            EV << "MecOrchestratorManager::handleMessage - \tFATAL! Error when casting to MEAppPacket" << endl;
            throw cRuntimeError("MecOrchestratorManager::handleMessage - \tFATAL! Error when casting to MEAppPacket");
        }
        else{
            EV << "MecOrchestratorManager::handleMessage - received MEAppPacket from " << mePkt->getSourceAddress() << endl;
            handleMEAppPacket(packet);
        }
//    }
}

/*
 * ###################################RESOURCE MANAGER decision HANDLER#################################
 */

//void MecOrchestratorManager::handleResource(cMessage* msg){
//
//    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
//    auto meAppPkt = packet->peekAtFront<MEAppPacket>();
//    if(meAppPkt == 0)
//    {
//        EV << "MecOrchestratorManager::handleResource - \tFATAL! Error when casting to MEAppPacket" << endl;
//        throw cRuntimeError("MecOrchestratorManager::handleResource - \tFATAL! Error when casting to MEAppPacket");
//    }
//    //handling MEAPP instantiation
//    else if(!strcmp(meAppPkt->getType(), START_MEAPP))
//    {
//        EV << "MecOrchestratorManager::handleResource - calling instantiateMEApp" << endl;
//        instantiateMEApp(msg);
//    }
//    //handling MEAPP termination
//    else if(!strcmp(meAppPkt->getType(), STOP_MEAPP))
//    {
//        EV << "MecOrchestratorManager::handleResource - calling terminateMEApp" << endl;
//        terminateMEApp(msg);
//    }
//}

/*
 * ######################################################################################################
 */
/*
 * #######################################MEAPP PACKETS HANDLERS####################################
 */

void MecOrchestratorManager::handleMEAppPacket(inet::Packet* packet){

    auto pkt = packet->peekAtFront<MEAppPacket>();

    EV << "MecOrchestratorManager::handleMEAppPacket - received "<< pkt->getType()<<" with delay: " << (simTime()-pkt->getTimestamp()) << endl;

    /* Handling START_MEAPP */
    if(!strcmp(pkt->getType(), START_MEAPP))
        startMEApp(packet);

    /* Handling STOP_MEAPP */
    else if(!strcmp(pkt->getType(), STOP_MEAPP))
        stopMEApp(packet);
}

void MecOrchestratorManager::startMEApp(inet::Packet* packet){

    EV << "MecOrchestratorManager::startMEApp - processing..." << endl;

    auto pkt = packet->peekAtFront<MEAppPacket>();

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    /*
     * The Mec orchestrator has to decide where to deploy the mec application.
     * It has to check if the Meapp has been already deployed
     *
     */

    if(meAppMap.find(ueAppID) != meAppMap.end())
    {
        meAppMap[ueAppID].lastAckStartSeqNum = pkt->getSno();
        //Sending ACK to the UEApp to confirm the instantiation in case of previous ack lost!
        ackMEAppPacket(ueAppID, ACK_START_MEAPP);
        //testing
        EV << "MecOrchestratorManager::startMEApp - \tWARNING: required MEApp instance ALREADY STARTED on MEC host: " << meAppMap[ueAppID].mecHost->getName() << endl;
        EV << "MecOrchestratorManager::startMEApp  - calling ackMEAppPacket with  "<< ACK_START_MEAPP << endl;
    }

    cModule* bestHost = findBestMecHost(pkt);

    if(bestHost != nullptr)
    {
     // add the new mec app in the map structure
        mecAppMapEntry newMecApp;
        newMecApp.mecUeAppID =  ueAppID;
        newMecApp.mecHost = bestHost;
        newMecApp.ueSymbolicAddres = pkt->getSourceAddress();
        newMecApp.ueAddress = inet::L3AddressResolver().resolve(pkt->getSourceAddress());
        newMecApp.uePort = pkt->getSourcePort();
        newMecApp.virtualisationInfrastructure = bestHost->getSubmodule("virtualisationInfrastructure");
        newMecApp.mecAppName = pkt->getMEModuleName();
        newMecApp.lastAckStartSeqNum = pkt->getSno();

        // call the methods of resource manager and virtualization infrastructure of the selected bestHost

     ResourceManager* resMan = check_and_cast<ResourceManager*>(newMecApp.virtualisationInfrastructure->getSubmodule("resourceManager"));
     VirtualisationManager* virtMan = check_and_cast<VirtualisationManager*>(newMecApp.virtualisationInfrastructure->getSubmodule("virtualisationManager"));

     int reqRam = pkt->getRequiredRam();
     int reqDisk = pkt->getRequiredDisk();
     double reqCpu = pkt->getRequiredCpu();

     bool res = resMan->registerMecApp(ueAppID, reqRam, reqDisk, reqCpu);
     if(!res)
         throw cRuntimeError("MecOrchestratorManager::startMEApp - \tERROR Required resources not available, but the findBestMecHost said yes!!");

     SockAddr endPoint = virtMan->instantiateMEApp(packet);
     EV << "VirtualisationManager::instantiateMEApp port" << endPoint.port << endl;

     newMecApp.mecAppAddress = endPoint.addr;
     newMecApp.mecAppPort = endPoint.port;

     meAppMap[ueAppID] = newMecApp;
     MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
     msg->setUeAppID(ueAppID);
     msg->setType(ACK_START_MEAPP);
     scheduleAt(simTime() + 0.010, msg);
    }
    else
    {
        //send bad request
    }

}


void MecOrchestratorManager::stopMEApp(inet::Packet* packet){

    EV << "MecOrchestratorManager::stopMEApp - processing..." << endl;

    auto pkt = packet->peekAtFront<MEAppPacket>();

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    //checking if ueAppIdToMeAppMapKey entry map does exist
    if(meAppMap.empty() || meAppMap.find(ueAppID) == meAppMap.end())
    {
//        maybe it has been deleted
        EV << "MecOrchestratorManager::stopMEApp - \tWARNING forwarding to meAppMap["<< ueAppID <<"] not found!" << endl;
        throw cRuntimeError("MecOrchestratorManager::stopMEApp - \tERROR ueAppIdToMeAppMapKey entry not found!");
        return;
    }

    // call the methods of resource manager and virtualization infrastructure of the selected mec host to deallocate the resources

     ResourceManager* resMan = check_and_cast<ResourceManager*>(meAppMap[ueAppID].virtualisationInfrastructure->getSubmodule("resourceManager"));
     VirtualisationManager* virtMan = check_and_cast<VirtualisationManager*>(meAppMap[ueAppID].virtualisationInfrastructure->getSubmodule("virtualisationManagerManager"));

     bool res = resMan->deRegisterMecApp(ueAppID);
     if(!res)
         throw cRuntimeError("MecOrchestratorManager::stopMEApp - \tERROR mec Application [%d] not found in its mec Host!!", ueAppID);
     virtMan->terminateMEApp(packet);

     meAppMap.erase(ueAppID);
     EV << "MecOrchestratorManager::stopMEApp - mec Application ["<< ueAppID << "] removed"<< endl;
     ackMEAppPacket(ueAppID, ACK_STOP_MEAPP);

}

void MecOrchestratorManager::ackMEAppPacket(int ueAppID, const char* type)
{

    if(meAppMap.empty() || meAppMap.find(ueAppID) == meAppMap.end())
    {
        EV << "MecOrchestratorManager::ackMEAppPacket - \ERROR meApp["<< ueAppID << "] does not exist!" << endl;
        throw cRuntimeError("MecOrchestratorManager::ackMEAppPacket - \ERROR meApp[%d] does not exist!", ueAppID);
        return;
    }

    mecAppMapEntry mecAppStatus = meAppMap[ueAppID];

    //checking if the UE is in the network & sending by socket
    destAddress_ = mecAppStatus.ueAddress;
    MacNodeId destId = binder_->getMacNodeId(destAddress_.toIpv4());
    if(destId == 0)
    {
        EV << "MecOrchestratorManager::ackMEAppPacket - \tWARNING " << mecAppStatus.ueSymbolicAddres << "has left the network!" << endl;
        //throw cRuntimeError("MecOrchestratorManager::ackMEAppPacket - \tFATAL! Error destination has left the network!");
    }
    else
    {
        EV << "MecOrchestratorManager::ackMEAppPacket - sending ack " << type <<" to "<< mecAppStatus.ueSymbolicAddres << ": [" << destAddress_.str() <<"]" << endl;

        inet::Packet* ackPacket = new inet::Packet();
        auto ack = inet::makeShared<MEAppPacket>();
        ack->setType(type);
        int sno = (strcmp(type, ACK_START_MEAPP) == 0) ? mecAppStatus.lastAckStartSeqNum : mecAppStatus.lastAckStopSeqNum;
        ack->setSno(sno);
        ack->setTimestamp(simTime());
        ack->setChunkLength(inet::B(20));
        ack->setSourceAddress("mecOrchestrator");
        ack->setSourcePort(localPort_);
        ack->setDestinationAddress(mecAppStatus.ueSymbolicAddres.c_str());
        ack->setDestinationPort(mecAppStatus.uePort);
        ack->setDestinationMecAppAddress(mecAppStatus.mecAppAddress.str().c_str());
        ack->setDestinationMecAppPort(mecAppStatus.mecAppPort);
EV << "QQQE" << mecAppStatus.mecAppPort << endl;
        ackPacket->insertAtBack(ack);
        socket.sendTo(ackPacket, destAddress_, mecAppStatus.uePort);
    }
}



cModule* MecOrchestratorManager::findBestMecHost(inet::Ptr<const MEAppPacket> packet){return mecHosts[0];}

void MecOrchestratorManager::getConnectedMecHosts()
{
    //getting the list of mec hosts associated to this mec system from parameter
    if(this->hasPar("mecHostList") && strcmp(par("mecHostList").stringValue(), "")){

        char* token = strtok ( (char*) par("mecHostList").stringValue(), ",");            // split by commas

        while (token != NULL)
        {
            EV <<"RadioNetworkInformation::initialize - mec host (from par): "<< token << endl;
            cModule *mecHostModule = getSimulation()->getModuleByPath(token);
            mecHosts.push_back(mecHostModule);
            token = strtok (NULL, ",");
        }
    }
    else{
        throw cRuntimeError ("MecOrchestratorManager::getConnectedMecHosts - No mecHostList found");
    }


    }


//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __MECORCHESTRATORMANAGER_H_
#define __MECORCHESTRATORMANAGER_H_

//BINDER and UTILITIES
#include "common/LteCommon.h"
#include "nodes/binder/LteBinder.h"           //to handle Car dynamically leaving the Network

//UDP SOCKET for INET COMMUNICATION WITH UE APPs
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEAppPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "nodes/mec/MEPlatform/MEAppPacket_m.h"

using namespace omnetpp;

struct mecAppMapEntry
{
    std::string mecAppName;
    int mecUeAppID;         //ID
    cModule* mecHost; // reference to the mecHost where the mec app has been deployed
    cModule* virtualisationInfrastructure;       //for virtualisationInfrastructure methods
    std::string ueSymbolicAddres;
    inet::L3Address ueAddress;  //for downstream using UDP Socket
    int uePort;
    inet::L3Address mecAppAddress;  //for downstream using UDP Socket
    int mecAppPort;

    int lastAckStartSeqNum;
    int lastAckStopSeqNum;


};

////###########################################################################

/**
 * MecOrchestratorManager
 *
 *  The task of this class is:
 *       1) handling all types of MEAppPacket:
 *              a) START_MEAPP:
 *                  query the ResourceManager & instantiate dynamically the MEApp specified into the meAppModuleType and meAppModuleName fields of the packet
 *              b) STOP_MEAP --> terminate a MEApp & query the ResourceManager to free resources
 *              c) replying with ACK_START_MEAPP or ACK_STOP_MEAPP to the UEApp

 */

class VirtualisationManager;

class MecOrchestratorManager : public cSimpleModule
{
    //------------------------------------
    //Binder module
    LteBinder* binder_;
    //------------------------------------
    //communication with UE APPs
    inet::UdpSocket socket;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;
    //------------------------------------
    //parent modules

    std::vector<cModule*> mecHosts;

    //------------------------------------
    const char* interfaceTableModule;
    //------------------------------------

    //mapping UEApp with the corresponding MEApp instance (using the meAppIn and meAppOut gate index)
    //key = UE App ID - value = ME App gate index
    std::map<int, int> ueAppIdToMeAppMapKey;
    //storing the UEApp and MEApp informations
    //key = ME App gate index - value mecAppMapEntry
    std::map<int, mecAppMapEntry> meAppMap;

    public:
        MecOrchestratorManager();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        /*
         * ----------------------------------Packet from ResourceManager handler---------------------------------
         */
        // receiving back from ResourceManager the start (1. resource allocated) or stop (2. resource deallocated) packets
        // calling:
        //          1) instantiateMEAPP() and then ackMEAppPacket() with ACK_START_MEAPP
        //          2) terminateMEAPP() and then ackMEAppPacket() with ACK_STOP_MEAPP
        void handleResource(cMessage*);
        /*
         * ------------------------------------------------------------------------------------------------------
         */
        /*
         * ------------------------------------------MEAppPacket handlers----------------------------------------
         */
        // handling all possible MEAppPacket types by invoking specific methods
        void handleMEAppPacket(inet::Packet*);

        // handling START_MEAPP type
        // by forwarding the packet to the ResourceManager if there are available MEApp "free slots"
        void startMEApp(inet::Packet*);

        //finding the ME Service requested by UE App among the ME Services available on the ME Host
        //return: the index of service (in mePlatform.udpService) or SERVICE_NOT_AVAILABLE or NO_SERVICE
        int findService(const char* serviceName);


        // handling STOP_MEAPP type
        // forwarding the packet to the ResourceManager
        void stopMEApp(inet::Packet*);


        // sending ACK_START_MEAPP or ACK_STOP_MEAPP (called by instantiateMEApp or terminateMEApp)
        void ackMEAppPacket(int ueAppId, const char*);

        cModule* findBestMecHost(inet::Ptr<const MEAppPacket> packet);

        void getConnectedMecHosts();

    };

#endif

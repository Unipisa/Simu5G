//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __VIM_H_
#define __VIM_H_

//BINDER and UTILITIES
#include "common/LteCommon.h"
#include "nodes/binder/LteBinder.h"           //to handle Car dynamically leaving the Network
#include "inet/networklayer/common/InterfaceTable.h"

//UDP SOCKET for INET COMMUNICATION WITH UE APPs
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEAppPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "nodes/mec/MEPlatform/MEAppPacket_m.h"

#include "nodes/mec/MecCommon.h"

//###########################################################################
//data structures and values

#define NO_SERVICE -1
#define SERVICE_NOT_AVAILABLE -2

using namespace omnetpp;

struct meAppMapEntry
{
    int meAppGateIndex;         //map key
    int serviceIndex;         // service index
    cModule* meAppModule;       //for ME App termination
    inet::L3Address ueAddress;  //for downstream using UDP Socket
    int uePort;
    int ueAppID;                //for identifying the UEApp
    int meAppPort;              //socket port of the meApp
    ResourceDescriptor resources;
};
//###########################################################################



struct MecAppInstanceInfo
{
    bool status;
    std::string instanceId;
    SockAddr endPoint;
};


/**
 * VirtualisationInfrastructureManager
 *
 *  The task of this class is:
 *       1) handling all types of MEAppPacket:
 *              a) START_MEAPP --> query the ResourceManager & instantiate dynamically the MEApp specified into the meAppModuleType and meAppModuleName fields of the packet
 *              b) STOP_MEAP --> terminate a MEApp & query the ResourceManager to free resources
 *              c) replying with ACK_START_MEAPP or ACK_STOP_MEAPP to the UEApp
 *              d) forwarding upstream INFO_UEAPP packets
 *              e) forwarding downstream INFO_MEAPP packets
 */


class CreateAppMessage;
class DeleteAppMessage;

class VirtualisationInfrastructureManager : public cSimpleModule
{
    friend class MecOrchestrator; // Friend Class

    //------------------------------------
    // SIMULTE Binder module
    LteBinder* binder_;
    //------------------------------------

    // other modules
    cModule* meHost;
    cModule* mePlatform;
    cModule* virtualisationInfr;
    //------------------------------------
    std::string interfaceTableModule;
    inet::InterfaceTable* interfaceTable;
    inet::Ipv4Address mecAppLocalAddress_;
    inet::Ipv4Address mecAppRemoteAddress_;
    inet::Ipv4Address mp1Address_;

    //------------------------------------
    //parameters to control the number of ME APPs instantiated and to set gate sizes
    int maxMEApps;
    int currentMEApps;

    int meAppPortCounter; // counter to assign socket ports to MeApps
    //------------------------------------

    //set of ME Services loaded into the ME Host & Platform
    int numServices;
    std::vector<cModule*> meServices;
    //------------------------------------
    //set of free gates to use for connecting ME Apps and ME Services
    std::vector<int> freeGates;
    //------------------------------------

    //storing the UEApp and MEApp informations
    //key = ME App gate index - value meAppMapEntry
    std::map<int, meAppMapEntry> meAppMap;

    //------------------------------------
    // Resources manager
    // maximum resources
       double maxRam;
       double maxDisk;
       double maxCPU;
       //allocated resources
       double allocatedRam;
       double allocatedDisk;
       double allocatedCPU;


    public:
        VirtualisationInfrastructureManager();

        // instancing the requested MEApp (called by handleResource)
        MecAppInstanceInfo instantiateMEApp(CreateAppMessage*);
        bool instantiateEmulatedMEApp(CreateAppMessage*);

        // terminating the correspondent MEApp (called by handleResource)
        bool terminateMEApp(DeleteAppMessage*);
        bool terminateEmulatedMEApp(DeleteAppMessage*);

        // Resources manager methods
       virtual double calculateProcessingTime(int ueAppID, int numOfInstructions);

       bool registerMecApp(int ueAppId, int reqRam, int  reqDisk, double reqCpu);
       bool deRegisterMecApp(int ueAppId);

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);


        //finding the ME Service requested by UE App among the ME Services available on the ME Host
        //return: the index of service (in mePlatform.udpService) or SERVICE_NOT_AVAILABLE or NO_SERVICE
        int findService(const char* serviceName);

        //------------------------------------

        ResourceDescriptor getAvailableResources() const ;

        /*
         * utility
         */
        void printResources(){
            EV << "VirtualisationInfrastructureManager::printResources - allocated Ram: " << allocatedRam << " / " << maxRam << endl;
            EV << "VirtualisationInfrastructureManager::printResources - allocated Disk: " << allocatedDisk << " / " << maxDisk << endl;
            EV << "VirtualisationInfrastructureManager::printResources - allocated CPU: " << allocatedCPU << " / " << maxCPU << endl;
        }

        bool isAllocable(double ram, double disk, double cpu)
        {
            return (currentMEApps < maxMEApps &&
                    ram  < maxRam - allocatedRam &&
                    disk < maxDisk - allocatedDisk &&
                    cpu  < maxCPU - allocatedCPU);
        }

        void allocateResources(double ram, double disk, double cpu){
            allocatedRam += ram;
            allocatedDisk += disk;
            allocatedCPU += cpu;
            printResources();
        }

        void deallocateResources(double ram, double disk, double cpu){
            allocatedRam -= ram;
            allocatedDisk -= disk;
            allocatedCPU -= cpu;
            printResources();
        }




};

#endif

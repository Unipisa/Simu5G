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
#ifndef __VIM_H_
#define __VIM_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "common/LteCommon.h"
#include "common/binder/Binder.h"
#include "nodes/mec/utils/MecCommon.h"

namespace simu5g {

//###########################################################################
//data structures and values

#define NO_SERVICE               -1
#define SERVICE_NOT_AVAILABLE    -2

using namespace omnetpp;

struct mecAppEntry
{
    int meAppGateIndex;         // map key
    int serviceIndex;           // OMNeT service index
    opp_component_ptr<cModule> meAppModule;       // for MEC app termination
    inet::L3Address ueAddress;  // for downstream using UDP Socket
    int uePort;
    int ueAppID;                // for identifying the UEApp
    int meAppPort;              // socket port of the MEC app
    ResourceDescriptor resources;
};

struct MecAppInstanceInfo
{
    bool status;
    std::string instanceId;
    SockAddr endPoint;
    cModule *reference;
};

// used to calculate processing time needed to execute a number of instructions
enum SchedulingMode { SEGREGATION, FAIR_SHARING };

//###########################################################################

//
// VirtualisationInfrastructureManager
//
//  Simple Module for handling the availability of virtualised resources on the MEC host.
//  From ETSI GS MEC 003, it is responsible for allocating, managing and releasing virtualised
//  resources of the virtualisation infrastructure;
//

class CreateAppMessage;
class DeleteAppMessage;

class VirtualisationInfrastructureManager : public cSimpleModule
{
    friend class MecOrchestrator; // Friend Class

    //------------------------------------
    // SIMULTE Binder module
    inet::ModuleRefByPar<Binder> binder_;
    //------------------------------------

    // other modules
    opp_component_ptr<cModule> mecHost;
    opp_component_ptr<cModule> mecPlatform;
    opp_component_ptr<cModule> virtualisationInfr;
    //------------------------------------
    std::string interfaceTableModule;
    opp_component_ptr<inet::InterfaceTable> interfaceTable;
    inet::Ipv4Address mecAppLocalAddress_;
    inet::Ipv4Address mecAppRemoteAddress_;
    inet::Ipv4Address mp1Address_;
    int mp1Port_;

    //------------------------------------
    //parameters to control the number of MEC APPs instantiated and to set gate sizes
    int maxMECApps;
    int currentMEApps = 0;

    int mecAppPortCounter; // counter to assign socket ports to Mec Apps
    //------------------------------------

    //-------------------------------------
    // OMNeT++-like MEC service management
    // set of MEC Services loaded into the MEC host & platform
    int numServices;
    std::vector<cModule *> meServices;
    // ------------------------------------
    // set of free gates to use for connecting MEC apps and MEC Services
    std::vector<int> freeGates;
    // ------------------------------------

    // storing the UEApp and MECApp informations
    // key = MEC App gate index - value mecAppMapEntry
    std::map<int, mecAppEntry> mecAppMap;

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

    SchedulingMode scheduling; // SEGREGATION or FAIR_SHARING

  public:

    // instancing the requested MEC App (called by MECPM)
    MecAppInstanceInfo *instantiateMEApp(CreateAppMessage *msg);
    bool instantiateEmulatedMEApp(CreateAppMessage *msg);

    // terminating the correspondent MEC App (called by MECPM)
    bool terminateMEApp(DeleteAppMessage *msg);
    bool terminateEmulatedMEApp(DeleteAppMessage *msg);

    /*
     * This method is called by the MEC apps that want to simulate
     * processing time for executing a number of instructions.
     * This method allows two types of scheduling:
     *  - Segregation: where the MEC app obtains exactly the amount of computing resources
     *  it has stipulated at the time of admission control, even when no other MEC apps
     *  are running concurrently;
     *
     *  - Fair sharing: where active MEC apps share all the available computing resources
     *  proportionally to their requested rate, possibly obtaining more capacity than
     *  stipulated when contention is low
     *
     *  The variable scheduling selects the mode
     *
     * @param mecAppID to identify the MEC app
     * @param numOfInstructions - number of instructions the MEC app wants to execute
     * @return calculated time to execute numOfInstructions as double
     *
     */
    virtual double calculateProcessingTime(int mecAppID, int numOfInstructions);

    /*
     * Methods called by MEC apps deployed through INI and not by UEs. Such MEC apps
     * are usually used to load the computing resources of the MEC host and
     * the MEC services and are not managed by the MEC orchestrator.
     * By calling these methods, it is possible to allocate (and deallocate) MEC host
     * resources.
     *
     * @param mecAppID to identify the MEC app
     * @param reqRam, reqDisk, reqCpu - computation resources needed by the MEC app
     * @return boolean result of the operation
     */
    bool registerMecApp(int mecAppID, int reqRam, int reqDisk, double reqCpu);
    bool deRegisterMecApp(int mecAppID);
    // ******************************************************************

    /*
     * Method that checks if a MEC app can be instantiated on the MEC host
     * @param reqRam, reqDisk, reqCpu - computation resources needed by the MEC app
     * @return boolean result
     */
    bool isAllocable(double ram, double disk, double cpu)
    {
        return currentMEApps < maxMECApps &&
               ram < maxRam - allocatedRam &&
               disk < maxDisk - allocatedDisk &&
               cpu < maxCPU - allocatedCPU;
    }

    /*
     * This method returns the available resources of the MEC host
     * @return ResourceDescriptor with the available ram, disk and CPU
     */
    ResourceDescriptor getAvailableResources() const;

    /*
     * utility
     */
    void printResources() {
        EV << "VirtualisationInfrastructureManager::printResources - allocated Ram (B): " << allocatedRam << " / " << maxRam << endl;
        EV << "VirtualisationInfrastructureManager::printResources - allocated Disk (B): " << allocatedDisk << " / " << maxDisk << endl;
        EV << "VirtualisationInfrastructureManager::printResources - allocated CPU (MIPS): " << allocatedCPU << " / " << maxCPU << endl;
    }

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // OMNeT++-like MEC service management
    //finding the MEC Service requested by UE App among the MEC Services available on the MEC Host
    //return: the index of service (in mePlatform.udpService) or SERVICE_NOT_AVAILABLE or NO_SERVICE
    int findService(const char *serviceName);

    //------------------------------------

    void allocateResources(double ram, double disk, double cpu) {
        allocatedRam += ram;
        allocatedDisk += disk;
        allocatedCPU += cpu;
        printResources();
    }

    void deallocateResources(double ram, double disk, double cpu) {
        allocatedRam -= ram;
        allocatedDisk -= disk;
        allocatedCPU -= cpu;
        printResources();
    }

    void reserveResourcesBGApps();
};

} //namespace

#endif


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
#include <inet/networklayer/common/L3AddressResolver.h>
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

namespace simu5g {

Define_Module(VirtualisationInfrastructureManager);


void VirtualisationInfrastructureManager::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER - 1)
        return;

    EV << "VirtualisationInfrastructureManager::initialize - stage " << stage << endl;

    //------------------------------------
    // Binder module
    binder_.reference(this, "binderModule", true);
    //------------------------------------

    //------------------------------------
    mecHost = getParentModule();
    if (mecHost->hasPar("maxMECApps"))
        maxMECApps = mecHost->par("maxMECApps");
    else
        throw cRuntimeError("VirtualisationInfrastructureManager::initialize - \tFATAL! Error when getting mecHost.maxMECApps parameter!");

    maxRam = mecHost->par("maxRam").doubleValue();
    maxDisk = mecHost->par("maxDisk").doubleValue();
    maxCPU = mecHost->par("maxCpuSpeed").doubleValue();

    allocatedRam = 0.0;
    allocatedDisk = 0.0;
    allocatedCPU = 0.0;
    printResources();

    const char *schedulingMode = par("scheduling").stringValue();
    if (strcmp(schedulingMode, "segregation") == 0) {
        EV << "VirtualisationInfrastructureManager::initialize - scheduling mode is: segregation" << endl;
        scheduling = SEGREGATION;
    }
    else if (strcmp(schedulingMode, "fair") == 0) {
        EV << "VirtualisationInfrastructureManager::initialize - scheduling mode is: fair" << endl;
        scheduling = FAIR_SHARING;
    }
    else {
        EV << "VirtualisationInfrastructureManager::initialize - scheduling mode: " << schedulingMode << " not recognized. Using default mode: segregation" << endl;
        scheduling = SEGREGATION;
    }

    virtualisationInfr = mecHost->getSubmodule("virtualisationInfrastructure");
    if (virtualisationInfr == nullptr)
        throw cRuntimeError("VirtualisationInfrastructureManager::initialize - mecHost.maxMECApps parameter!");

    // register MEC addresses to the Binder
    inet::L3Address mecHostAddress = inet::L3AddressResolver().resolve(virtualisationInfr->getFullPath().c_str());
    inet::L3Address gtpAddress = inet::L3AddressResolver().resolve(mecHost->getSubmodule("upf_mec")->getFullPath().c_str());
    binder_->registerMecHostUpfAddress(mecHostAddress, gtpAddress);
    binder_->registerMecHost(mecHostAddress);

    virtualisationInfr->setGateSize("meAppOut", maxMECApps);
    virtualisationInfr->setGateSize("meAppIn", maxMECApps);

    mecPlatform = mecHost->getSubmodule("mecPlatform");

    mp1Address_ = mecHostAddress.toIpv4();
    mp1Port_ = par("mp1Port");

    //------------------------------------
    //retrieve the set of free gates
    for (int i = 0; i < maxMECApps; i++)
        freeGates.push_back(i);
    //------------------------------------
    interfaceTable = check_and_cast<inet::InterfaceTable *>(virtualisationInfr->getSubmodule("interfaceTable"));

    /*
     * NOTE: if the mecHost is connected both to ppp and pppENB gates, 2 pppIf interfaces are present in the
     * virtualisationInfrastructure. This case should not occur, so it is not managed.
     */
    int numInterfaces = interfaceTable->getNumInterfaces();
    for (int i = 0; i < numInterfaces; ++i) {
        if (strcmp("pppIf0", interfaceTable->getInterface(i)->getInterfaceName()) == 0) {
            mecAppRemoteAddress_ = interfaceTable->getInterface(i)->getIpv4Address();
        }
        else if (strcmp("eth", interfaceTable->getInterface(i)->getInterfaceName()) == 0) {
            mecAppLocalAddress_ = interfaceTable->getInterface(i)->getIpv4Address();
        }
    }
    mecAppPortCounter = 4001;

    //reserve resources of the background apps!
    reserveResourcesBGApps();
}

void VirtualisationInfrastructureManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;
}

bool VirtualisationInfrastructureManager::instantiateEmulatedMEApp(CreateAppMessage *msg)
{
    EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - processing..." << endl;
    //retrieve UE App ID
    int ueAppID = msg->getUeAppID();
    const char *meModuleName = msg->getMEModuleName();

    //checking if there are ME App slots available and if ueAppIdToMeAppMapKey map entry does not exist (that means ME App not already instantiated)
    if (currentMEApps < maxMECApps && mecAppMap.find(ueAppID) == mecAppMap.end()) {
        //creating ueAppIdToMeAppMapKey map entry
        EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - ueAppIdToMeAppMapKey[" << ueAppID << "]" << endl;
        int key = ueAppID;

        double ram = msg->getRequiredRam();
        double disk = msg->getRequiredDisk();
        double cpu = msg->getRequiredCpu();

        if (isAllocable(ram, disk, cpu))
            allocateResources(ram, disk, cpu);
        else {
            EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - MEC Application with required resources:\n" <<
                "ram: " << ram << endl <<
                "disk: " << disk << endl <<
                "cpu: " << cpu << endl <<
                "cannot be instantiated due to unavailable resources" << endl;

            return false;
        }
        // creating MEApp module instance
        std::stringstream appName;
        appName << meModuleName << "[" << msg->getContextId() << "]";
        EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - meModuleName: " << appName.str() << endl;

        //creating the mecAppMap map entry
        mecAppEntry newAppEntry;

        newAppEntry.meAppGateIndex = -1;
        newAppEntry.meAppModule = nullptr;
        //mecAppMap[key].ueAddress = ueAppAddress;
        //mecAppMap[key].uePort = ueAppPort;
        newAppEntry.ueAppID = ueAppID;
        newAppEntry.resources.ram = ram;
        newAppEntry.resources.disk = disk;
        newAppEntry.resources.cpu = cpu;

        mecAppMap.insert({key, newAppEntry});

        currentMEApps++;

        //Sending ACK to the UEApp
        //testing
        EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - currentMEApps: " << currentMEApps << " / " << maxMECApps << endl;
        return true;
    }
    else {
        return false;
    }
}

MecAppInstanceInfo *VirtualisationInfrastructureManager::instantiateMEApp(CreateAppMessage *msg)
{
    EV << "VirtualisationInfrastructureManager::instantiateMEApp - processing..." << endl;

    int serviceIndex = findService(msg->getRequiredService());

    const char *meModuleName = msg->getMEModuleName();

    // int ueAppPort  = msg->getSourcePort();

    //retrieve UE App ID
    int ueAppID = msg->getUeAppID();

    //checking if there are ME App slots available and if ueAppIdToMeAppMapKey map entry does not exist (that means ME App not already instantiated)
    if (currentMEApps < maxMECApps && mecAppMap.find(ueAppID) == mecAppMap.end()) {
        //getting the first available gate
        int index = freeGates.front();
        freeGates.erase(freeGates.begin());
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - freeGate: " << index << endl;

        //creating ueAppIdToMeAppMapKey map entry
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - ueAppIdToMeAppMapKey[" << ueAppID << "] = " << index << endl;
        int key = ueAppID;

        double ram = msg->getRequiredRam();
        double disk = msg->getRequiredDisk();
        double cpu = msg->getRequiredCpu();

        if (isAllocable(ram, disk, cpu))
            allocateResources(ram, disk, cpu);
        else {
            EV << "VirtualisationInfrastructureManager::instantiateMEApp - MEC Application with required resources:\n" <<
                "ram: " << ram << endl <<
                "disk: " << disk << endl <<
                "cpu: " << cpu << endl <<
                "cannot be instantiated due to unavailable resources" << endl;

            MecAppInstanceInfo *instanceInfo = new MecAppInstanceInfo();
            instanceInfo->status = false;
            return instanceInfo;
        }
        // creating MEApp module instance
        cModuleType *moduleType = cModuleType::get(msg->getMEModuleType());         //MEAPP module package (i.e. path!)
        cModule *module = moduleType->create(meModuleName, mecHost);       //MEAPP module-name & its Parent Module
        std::stringstream appName;
        appName << meModuleName << "\n" << module->getId();
        module->setName(appName.str().c_str());
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - meModuleName: " << appName.str() << endl;
        //creating the mecAppMap map entry
        mecAppEntry newAppEntry;

        newAppEntry.meAppGateIndex = index;
        newAppEntry.serviceIndex = serviceIndex;

        newAppEntry.meAppModule = module;
        //mecAppMap[key].ueAddress = ueAppAddress;
        //mecAppMap[key].uePort = ueAppPort;
        newAppEntry.ueAppID = ueAppID;
        newAppEntry.meAppPort = mecAppPortCounter;
        newAppEntry.resources.ram = ram;
        newAppEntry.resources.disk = disk;
        newAppEntry.resources.cpu = cpu;

        mecAppMap.insert({key, newAppEntry});

        //displaying ME App dynamically created (after 70 they will overlap..)
        std::stringstream display;
        display << "p=" << (80 + ((index % 5) * 200) % 1000) << "," << (100 + (50 * (index / 5) % 350)) << ";is=vs";
        module->setDisplayString(display.str().c_str());

        //initialize IMECApp Parameters
        module->par("mecAppIndex") = index;
        module->par("mecAppId") = ueAppID;
        module->par("requiredRam") = ram;
        module->par("requiredDisk") = disk;
        module->par("requiredCpu") = cpu;
        module->par("localUePort") = mecAppPortCounter;
        module->par("mp1Address") = mp1Address_.str();
        module->par("mp1Port") = mp1Port_;

        module->finalizeParameters();

        MecAppInstanceInfo *instanceInfo = new MecAppInstanceInfo();
        instanceInfo->instanceId = appName.str();

        instanceInfo->endPoint.addr = mecAppRemoteAddress_;
        instanceInfo->endPoint.port = mecAppPortCounter;

        EV << "VirtualisationInfrastructureManager::instantiateMEApp port" << instanceInfo->endPoint.port << endl;

        mecAppPortCounter++;


        //connecting VirtualisationInfrastructure gates to the MEApp gates

        // add gates to the 'at' layer and connect them to the virtualisationInfr gates
        cModule *at = virtualisationInfr->getSubmodule("at");
        if (at == nullptr)
            throw cRuntimeError("at module, i.e. message dispatcher for SAP between application and transport layer not found");

        cGate *newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
        cGate *newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);

        newAtOutGate->connectTo(virtualisationInfr->gate("meAppOut", index));
        virtualisationInfr->gate("meAppOut", index)->connectTo(module->gate("socketIn"));

        // connect virtualisationInfr gates to the meApp
        virtualisationInfr->gate("meAppIn", index)->connectTo(newAtInGate);
        module->gate("socketOut")->connectTo(virtualisationInfr->gate("meAppIn", index));

        /*
         * @author Alessandro Noferi
         *
         * with the new MEC App management (i.e. they are directly connected to the transport layer)
         * it is the MEC App itself that looks for the ME Service through the ServiceRegistry module present in the
         * MEC Platform.
         *
         * This is true for MEC services ETPS compliant, but for OMNeT-like services the classic method is used
         * if there is a service required: link the MEC App to MEC PLATFORM to ME SERVICE
         */

        if (serviceIndex >= 0) {
            EV << "VirtualisationInfrastructureManager::instantiateMEApp - Connecting to the: " << msg->getRequiredService() << endl;
            //connecting ME Platform gates to the ME App gates
            mecPlatform->gate("meAppOut", index)->connectTo(module->gate("mePlatformIn"));
            module->gate("mePlatformOut")->connectTo(mecPlatform->gate("meAppIn", index));

            //connecting internal ME Platform gates to the required ME Service gates
            cModule *meService = meServices.at(serviceIndex);
            meService->gate("meAppOut", index)->connectTo(mecPlatform->gate("meAppOut", index));
            mecPlatform->gate("meAppIn", index)->connectTo(meService->gate("meAppIn", index));
        }
        else {
            EV << "VirtualisationInfrastructureManager::instantiateMEApp - NO OMNeT++-like MEC Services required!" << endl;
        }

        module->buildInside();
        module->scheduleStart(simTime());
        module->callInitialize();

        currentMEApps++;

        //testing
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - " << module->getName() << " instantiated!" << endl;
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - currentMEApps: " << currentMEApps << " / " << maxMECApps << endl;

        instanceInfo->status = true;
        instanceInfo->reference = module;
        return instanceInfo;
    }
    else {
        MecAppInstanceInfo *instanceInfo = new MecAppInstanceInfo();
        instanceInfo->status = false;
        return instanceInfo;
    }
}

bool VirtualisationInfrastructureManager::terminateEmulatedMEApp(DeleteAppMessage *msg)
{
    int ueAppID = msg->getUeAppID();

    if (!mecAppMap.empty() && mecAppMap.find(ueAppID) != mecAppMap.end()) {
        //retrieve mecAppMap map key
        int key = ueAppID;
        EV << "VirtualisationInfrastructureManager::terminateMEApp - " << mecAppMap[key].meAppModule->getName() << " terminated!" << endl;
        currentMEApps--;
        EV << "VirtualisationInfrastructureManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMECApps << endl;

        //deallocate resources
        deallocateResources(mecAppMap[key].resources.ram, mecAppMap[key].resources.disk, mecAppMap[key].resources.cpu);

        //update map
        mecAppMap.erase(ueAppID);

        return true;
    }
    else {
        EV << "VirtualisationInfrastructureManager::terminateMEApp - \tWARNING: NO INSTANCE FOUND!" << endl;
        return false;
    }
}

bool VirtualisationInfrastructureManager::terminateMEApp(DeleteAppMessage *msg)
{
    int ueAppID = msg->getUeAppID();

    if (!mecAppMap.empty() && mecAppMap.find(ueAppID) != mecAppMap.end()) {
        //retrieve mecAppMap map key
        int key = ueAppID;

        EV << "VirtualisationInfrastructureManager::terminateMEApp - " << mecAppMap[key].meAppModule->getName() << " terminated!" << endl;
        //terminating the ME App instance
        mecAppMap[key].meAppModule->callFinish();
        mecAppMap[key].meAppModule->deleteModule();
        currentMEApps--;
        EV << "VirtualisationInfrastructureManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMECApps << endl;

        //Sending ACK_STOP_MEAPP to the UEApp

        //before removing the map entry!
        int index = mecAppMap[key].meAppGateIndex;
        int serviceIndex = mecAppMap[key].serviceIndex;

        // TODO manage gates me app to at

        virtualisationInfr->gate("meAppOut", index)->getPreviousGate()->disconnect();
        virtualisationInfr->gate("meAppIn", index)->disconnect();

        if (serviceIndex >= 0) {
            //disconnecting internal MEPlatform gates to the MEService gates
            (meServices.at(serviceIndex))->gate("meAppOut", index)->disconnect();
            (meServices.at(serviceIndex))->gate("meAppIn", index)->disconnect();
            //disconnecting MEPlatform gates to the MEApp gates
            mecPlatform->gate("meAppOut", index)->disconnect();
            mecPlatform->gate("meAppIn", index)->disconnect();
        }

        //deallocate resources
        deallocateResources(mecAppMap[key].resources.ram, mecAppMap[key].resources.disk, mecAppMap[key].resources.cpu);

        //update map
        mecAppMap.erase(ueAppID);

        freeGates.push_back(index);

        return true;
    }
    else {
        EV << "VirtualisationInfrastructureManager::terminateMEApp - \tWARNING: NO INSTANCE FOUND!" << endl;
        return false;
    }
}

int VirtualisationInfrastructureManager::findService(const char *serviceName) {
    EV << "VirtualisationInfrastructureManager::findService " << serviceName << endl;

    if (!strcmp(serviceName, "NULL"))
        return NO_SERVICE;

    auto it = meServices.begin();
    for (auto service : meServices) {
        if (!strcmp(service->getClassName(), serviceName))
            break;
    }
    if (it == meServices.end())
        return SERVICE_NOT_AVAILABLE;

    return it - meServices.begin();
}

bool VirtualisationInfrastructureManager::registerMecApp(int ueAppID, int reqRam, int reqDisk, double reqCpu)
{
    EV << "VirtualisationInfrastructureManager::registerMecApp - RAM: " << reqRam << " CPU: " << reqCpu << " disk: " << reqDisk << endl;
    printResources();
    if (mecAppMap.find(ueAppID) == mecAppMap.end()) {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - independent MEC application with mecAppId [" << ueAppID << "] already instantiated" << endl;
    }

    if (isAllocable(reqRam, reqDisk, reqCpu)) {
        //storing information about ME App allocated resources
        mecAppEntry appEntry;

        appEntry.ueAppID = ueAppID;
        appEntry.resources.ram = reqRam;
        appEntry.resources.disk = reqDisk;
        double cpu = reqCpu;
        appEntry.resources.cpu = cpu;
        mecAppMap.insert({ ueAppID, appEntry });

        EV << "VirtualisationInfrastructureManager::handleMEAppResources - resources ALLOCATED for independent MecApp with module id " << ueAppID << endl;
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - ram: " << mecAppMap[ueAppID].resources.ram << " disk: " << mecAppMap[ueAppID].resources.disk << " cpu: " << mecAppMap[ueAppID].resources.cpu << endl;
        allocateResources(reqRam, reqDisk, reqCpu);
        return true;
    }
    else {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - resources NOT AVAILABLE for independent MecApp with module id " << ueAppID << endl;
        return false;
    }
}

bool VirtualisationInfrastructureManager::deRegisterMecApp(int ueAppID)
{
    if (!mecAppMap.empty() || mecAppMap.find(ueAppID) != mecAppMap.end()) {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - resources DEALLOCATED for MecApp with UEAppId " << ueAppID << endl;
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - ram: " << mecAppMap[ueAppID].resources.ram << " disk: " << mecAppMap[ueAppID].resources.disk << " cpu: " << mecAppMap[ueAppID].resources.cpu << endl;
        deallocateResources(mecAppMap[ueAppID].resources.ram, mecAppMap[ueAppID].resources.disk, mecAppMap[ueAppID].resources.cpu);
        //erasing map entry
        mecAppMap.erase(ueAppID);
        return true;
    }
    else {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - NO ALLOCATION FOUND for MecApp with UEAppId id " << ueAppID << endl;
        return false;
    }
}

double VirtualisationInfrastructureManager::calculateProcessingTime(int ueAppID, int numOfInstructions)
{
    ASSERT(numOfInstructions >= 0);

    auto ueApp = mecAppMap.find(ueAppID);
    if (ueApp != mecAppMap.end()) {
        double time;
        double currentSpeed;
        if (scheduling == FAIR_SHARING) {
            currentSpeed = ueApp->second.resources.cpu * (maxCPU / allocatedCPU);
            time = numOfInstructions / (pow(10, 6) * currentSpeed);
        }
        else {
            currentSpeed = ueApp->second.resources.cpu;
            time = numOfInstructions / (pow(10, 6) * currentSpeed);
        }
        EV << "VirtualisationInfrastructureManager::calculateProcessingTime - number of instructions: " << numOfInstructions << endl;
        EV << "VirtualisationInfrastructureManager::calculateProcessingTime - currentSpeed (MIPS): " << currentSpeed << endl;
        EV << "VirtualisationInfrastructureManager::calculateProcessingTime - calculated time: " << time << "s" << endl;

        return time;
    }
    else {
        EV << "VirtualisationInfrastructureManager::calculateProcessingTime - ZERO " << endl;
        return 0;
    }
}

ResourceDescriptor VirtualisationInfrastructureManager::getAvailableResources() const {
    ResourceDescriptor avRes;
    avRes.ram = maxRam - allocatedRam;
    avRes.disk = maxDisk - allocatedDisk;
    avRes.cpu = maxCPU - allocatedCPU;
    return avRes;
}

void VirtualisationInfrastructureManager::reserveResourcesBGApps()
{
    int numMecApps = mecHost->par("numBGMecApp");
    EV << "VirtualisationInfrastructureManager::reserveResourcesBGApps - reserving resources for " << numMecApps << " BG apps..." << endl;

    for (int i = 0; i < numMecApps; ++i) {
        cModule *bgApp = mecHost->getSubmodule("bgApp", i);
        if (bgApp == nullptr)
            throw cRuntimeError("VirtualisationInfrastructureManager::reserveResourcesBGApps - Background Mec App bgApp[%d] not found!", i);
        double ram = bgApp->par("ram");
        double disk = bgApp->par("disk");
        double cpu = bgApp->par("cpu");
        registerMecApp(bgApp->getId(), ram, disk, cpu);
    }
}

} //namespace


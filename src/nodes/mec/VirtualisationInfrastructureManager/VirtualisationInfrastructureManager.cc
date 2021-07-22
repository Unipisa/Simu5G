//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

#include "nodes/mec/LCMProxy/LCMProxyMessages/LcmProxyMessages_m.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

Define_Module(VirtualisationInfrastructureManager);

VirtualisationInfrastructureManager::VirtualisationInfrastructureManager()
{
    currentMEApps = 0;
}

void VirtualisationInfrastructureManager::initialize(int stage)
{
    EV << "VirtualisationInfrastructureManager::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER-1)
        return;
    //------------------------------------
    // Binder module
    binder_ = getBinder();
    //------------------------------------


    //------------------------------------
    meHost = getParentModule();
    if(meHost->hasPar("maxMEApps"))
        maxMEApps = meHost->par("maxMEApps");
    else
        throw cRuntimeError("VirtualisationInfrastructureManager::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");

    maxRam = meHost->par("maxRam").doubleValue();
    maxDisk = meHost->par("maxDisk").doubleValue();
    maxCPU = meHost->par("maxCpuSpeed").doubleValue();

    allocatedRam = 0.0;
    allocatedDisk = 0.0;
    allocatedCPU = 0.0;
    printResources();

    //    //setting gate sizes for VirtualisationInfrastructureManager and VirtualisationInfrastructure
//    this->setGateSize("meAppOut", maxMEApps);
//    this->setGateSize("meAppIn", maxMEApps);
    virtualisationInfr = meHost->getSubmodule("virtualisationInfrastructure");
    if(virtualisationInfr == nullptr)
        throw cRuntimeError("VirtualisationInfrastructureManager::initialize - meHost.maxMEApps parameter!");

    virtualisationInfr->setGateSize("meAppOut", maxMEApps);
    virtualisationInfr->setGateSize("meAppIn", maxMEApps);
    //VirtualisationInfrastructure internal gate connections with VirtualisationInfrastructureManager
//        for(int index = 0; index < maxMEApps; index++)
//        {
//            this->gate("meAppOut", index)->connectTo(virtualisationInfr->gate("meAppOut", index));
//            virtualisationInfr->gate("meAppIn", index)->connectTo(this->gate("meAppIn", index));
//        }

    mePlatform = meHost->getSubmodule("mecPlatform");
    //setting  gate sizes for MEPlatform
    if(mePlatform->gateSize("meAppOut") == 0 || mePlatform->gateSize("meAppIn") == 0)
    {
        mePlatform->setGateSize("meAppOut", maxMEApps);
        mePlatform->setGateSize("meAppIn", maxMEApps);
    }

    inet::InterfaceTable* platformInterfaceTable = check_and_cast<inet::InterfaceTable*>(mePlatform->getSubmodule("interfaceTable"));

    int interfaceSize = platformInterfaceTable->getNumInterfaces();
    bool found = false;
    for(int i = 0 ; i < interfaceSize ; ++i)
    {
        if(strcmp("mp1Eth", platformInterfaceTable->getInterface(i)->getInterfaceName()) == 0)
        {
            mp1Address_ = platformInterfaceTable->getInterface(i)->getIpv4Address();
            found = true;
        }
    }

    if(found == false)
        throw cRuntimeError("VirtualisationInfrastructureManager::initialize - interface for mecPlatform not found!!\n"
                "has its name [mp1Eth] been changed?");


    // retrieving all available ME Services loaded
    numServices = mePlatform->par("numOmnetServices");
    for(int i=0; i<numServices; i++)
    {
        meServices.push_back(mePlatform->getSubmodule("omnetService", i));
        EV << "VirtualisationInfrastructureManager::initialize - Available meServices["<<i<<"] " << meServices.at(i)->getClassName() << endl;
    }

    //------------------------------------
    //retrieve the set of free gates
    for(int i = 0; i < maxMEApps; i++)
        freeGates.push_back(i);
//    ------------------------------------
//    interfaceTableModule = par("interfaceTableModule").stringValue();


    interfaceTable = check_and_cast<inet::InterfaceTable*>(meHost->getSubmodule("virtualisationInfrastructure")->getSubmodule("interfaceTable"));

    /*
     * NOTE: if the mecHost is connected both to ppp and pppENB gates, 2 pppIf interfaces are present in the
     * virtualisationInfrastructure. This case should not occur, so it is not managed.
     */
    int numInterfaces = interfaceTable->getNumInterfaces();
    for(int i = 0 ; i < numInterfaces ; ++i)
    {
        if(strcmp("pppIf0", interfaceTable->getInterface(i)->getInterfaceName()) == 0)
        {
            mecAppRemoteAddress_ = interfaceTable->getInterface(i)->getIpv4Address();
        }
        else if (strcmp("eth", interfaceTable->getInterface(i)->getInterfaceName()) == 0)
        {
            mecAppLocalAddress_ = interfaceTable->getInterface(i)->getIpv4Address();
        }
//        else
//        {
//            throw cRuntimeError("VirtualisationInfrastructureManager::initialize - Unknown interface [%s] found. Have you changed the names?", interfaceTable->getInterface(i)->getInterfaceName());
//        }
    }
    meAppPortCounter = 4001;
}

void VirtualisationInfrastructureManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;
}

bool VirtualisationInfrastructureManager::instantiateEmulatedMEApp(CreateAppMessage* msg)
{
    EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - processing..." << endl;
    //retrieve UE App ID
    int ueAppID = msg->getUeAppID();
    char* meModuleName = (char*)msg->getMEModuleName();

    //checking if there are ME App slots available and if ueAppIdToMeAppMapKey map entry does not exist (that means ME App not already instantiated)
    if(currentMEApps < maxMEApps &&  meAppMap.find(ueAppID) == meAppMap.end())
    {
        //creating ueAppIdToMeAppMapKey map entry
        EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - ueAppIdToMeAppMapKey[" << ueAppID << endl;
        int key = ueAppID;

//        //getting the UEApp L3Address
//        inet::L3Address ueAppAddress = inet::L3AddressResolver().resolve(sourceAddress);
//        EV << "VirtualisationInfrastructureManager::instantiateMEApp - UEAppL3Address: " << ueAppAddress.str() << endl;

        double ram = msg->getRequiredRam();
        double disk = msg->getRequiredDisk();
        double cpu = msg->getRequiredCpu();

        if(isAllocable(ram, disk, cpu))
            allocateResources(ram, disk, cpu);
        else
        {
            EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - Mec Application with required resources:\n" <<
                    "ram: " << ram << endl <<
                    "disk: " << disk << endl <<
                    "cpu: " << cpu << endl <<
                    "cannot be instantiated due to unavailable resources" << endl;

            return false;
        }
        // creating MEApp module instance
//        cModuleType *moduleType = cModuleType::get("lte.nodes.mec.MEPlatform.EmulatedMecApplication");
//        cModule *module = moduleType->create(meModuleName, meHost); //MEAPP module-name & its Parent Module


        std::stringstream appName;
        appName << meModuleName << "[" <<  msg->getContextId() << "]";
        EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - meModuleName: " << appName.str() << endl;

        //creating the meAppMap map entry
        meAppMapEntry newAppEntry;

        newAppEntry.meAppGateIndex = -1;
        newAppEntry.meAppModule = nullptr;
        //meAppMap[key].ueAddress = ueAppAddress;
        //meAppMap[key].uePort = ueAppPort;
        newAppEntry.ueAppID = ueAppID;
//        newAppEntry.meAppPort = meAppPortCounter;
        newAppEntry.resources.ram  = ram;
        newAppEntry.resources.disk = disk;
        newAppEntry.resources.cpu  = cpu;

        meAppMap.insert(std::pair<int,meAppMapEntry>(key, newAppEntry));

//        meAppPortCounter++;

        currentMEApps++;

        //Sending ACK to the UEApp
//        EV << "VirtualisationInfrastructureManager::instantiateMEApp - calling ackMEAppPacket with  "<< ACK_START_MEAPP << endl;
//        ackMEAppPacket(packet, ACK_START_MEAPP);

        //testing
        EV << "VirtualisationInfrastructureManager::instantiateEmulatedMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;
        return true;
    }
    else
    {
        return false;
    }
}

MecAppInstanceInfo VirtualisationInfrastructureManager::instantiateMEApp(CreateAppMessage* msg)
{
    EV << "VirtualisationInfrastructureManager::instantiateMEApp - processing..." << endl;

    int serviceIndex = findService(msg->getRequiredService());

    char* meModuleName = (char*)msg->getMEModuleName();

    int ueAppPort  = msg->getSourcePort();

    //retrieve UE App ID
    int ueAppID = msg->getUeAppID();

    //checking if there are ME App slots available and if ueAppIdToMeAppMapKey map entry does not exist (that means ME App not already instantiated)
    if(currentMEApps < maxMEApps &&  meAppMap.find(ueAppID) == meAppMap.end())
    {
        //getting the first available gate
        int index = freeGates.front();
        freeGates.erase(freeGates.begin());
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - freeGate: " << index << endl;

        //creating ueAppIdToMeAppMapKey map entry
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - ueAppIdToMeAppMapKey[" << ueAppID << "] = " << index << endl;
        int key = ueAppID;

//        //getting the UEApp L3Address
//        inet::L3Address ueAppAddress = inet::L3AddressResolver().resolve(sourceAddress);
//        EV << "VirtualisationInfrastructureManager::instantiateMEApp - UEAppL3Address: " << ueAppAddress.str() << endl;

        double ram = msg->getRequiredRam();
        double disk = msg->getRequiredDisk();
        double cpu = msg->getRequiredCpu();

        if(isAllocable(ram, disk, cpu))
            allocateResources(ram, disk, cpu);
        else
        {
            EV << "VirtualisationInfrastructureManager::instantiateMEApp - Mec Applciation with required resources:\n" <<
                    "ram: " << ram << endl <<
                    "disk: " << disk << endl <<
                    "cpu: " << cpu << endl <<
                    "cannot be instantiated due to unavailable resources" << endl;

            MecAppInstanceInfo instanceInfo;
            instanceInfo.status = false;
            return instanceInfo;
        }
        // creating MEApp module instance
        cModuleType *moduleType = cModuleType::get(msg->getMEModuleType());         //MEAPP module package (i.e. path!)
        cModule *module = moduleType->create(meModuleName, meHost);       //MEAPP module-name & its Parent Module
        std::stringstream appName;
        appName << meModuleName << "[" <<  msg->getContextId() << "]";
        module->setName(appName.str().c_str());
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - meModuleName: " << appName.str() << endl;
        //creating the meAppMap map entry
        meAppMapEntry newAppEntry;

        newAppEntry.meAppGateIndex = index;
        newAppEntry.serviceIndex = serviceIndex;

        newAppEntry.meAppModule = module;
        //meAppMap[key].ueAddress = ueAppAddress;
        //meAppMap[key].uePort = ueAppPort;
        newAppEntry.ueAppID = ueAppID;
        newAppEntry.meAppPort = meAppPortCounter;
        newAppEntry.resources.ram  = ram;
        newAppEntry.resources.disk = disk;
        newAppEntry.resources.cpu  = cpu;

        meAppMap.insert(std::pair<int,meAppMapEntry>(key, newAppEntry));


        //displaying ME App dynamically created (after 70 they will overlap..)
        std::stringstream display;
        display << "p=" << (80 + ((index%5)*200)%1000) << "," << (100 + (50*(index/5)%350)) << ";is=vs";
        module->setDisplayString(display.str().c_str());


        //initialize IMEApp Parameters
        /*
         * TODO decide if set the endPoint of the serviceRegistry (i.e. mp1)
         * here, or default in the ned
         */

        module->par("mecAppId") = ueAppID;
        module->par("requiredRam") = ram;
        module->par("requiredDisk") = disk;
        module->par("requiredCpu") = cpu;
        module->par("localUePort") = meAppPortCounter;

        module->par("mp1Address") = mp1Address_.str();

        module->finalizeParameters();

        MecAppInstanceInfo instanceInfo;
        instanceInfo.instanceId = appName.str();

        instanceInfo.endPoint.addr = mecAppRemoteAddress_;
        instanceInfo.endPoint.port = meAppPortCounter;

        EV << "VirtualisationInfrastructureManager::instantiateMEApp port"<< instanceInfo.endPoint.port << endl;

        meAppPortCounter++;

//        EV << "VirtualisationInfrastructureManager::instantiateMEApp - UEAppSimbolicAddress: " << sourceAddress << endl;

        //connecting VirtualisationInfrastructure gates to the MEApp gates

        // add gates to the 'at' layer and connect them to the virtualisationInfr gates
        cModule *at = virtualisationInfr->getSubmodule("at");
        if(at == nullptr)
            throw cRuntimeError("at module, i.e. message dispatcher for SAP between application and transport layer non found");


        cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
        cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);


        newAtOutGate->connectTo(virtualisationInfr->gate("meAppOut", index));
        virtualisationInfr->gate("meAppOut", index)->connectTo(module->gate("socketIn"));

        // connect virtualisationInfr gates to the meApp
        virtualisationInfr->gate("meAppIn", index)->connectTo(newAtInGate);
        module->gate("socketOut")->connectTo(virtualisationInfr->gate("meAppIn", index));


        /*
         * @author Alessandro Noferi
         *
         * with the new MeApp management (i.e. they are directly connected to the transport layer)
         * it is the MeApp itself that looks for the MeService through the ServiceRegistry module present in the
         * MePlatform.
         *
         * This is true for MEC services etsi complaint, but for omnet-like services the classic method is used
         * if there is a service required: link the MEApp to MEPLATFORM to MESERVICE
         */

//        if(serviceIndex != NO_SERVICE)
        if(serviceIndex >= 0)
        {
            EV << "VirtualisationInfrastructureManager::instantiateMEApp - Connecting to the: " << msg->getRequiredService()<< endl;
            //connecting MEPlatform gates to the MEApp gates
            mePlatform->gate("meAppOut", index)->connectTo(module->gate("mePlatformIn"));
            module->gate("mePlatformOut")->connectTo(mePlatform->gate("meAppIn", index));

            //connecting internal MEPlatform gates to the required MEService gates
            (meServices.at(serviceIndex))->gate("meAppOut", index)->connectTo(mePlatform->gate("meAppOut", index));
            mePlatform->gate("meAppIn", index)->connectTo((meServices.at(serviceIndex))->gate("meAppIn", index));
        }

        else EV << "VirtualisationInfrastructureManager::instantiateMEApp - NO omnet++-like MECServices required!"<< endl;

        module->buildInside();
        module->scheduleStart(simTime());
        module->callInitialize();

        currentMEApps++;


        //testing
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - "<< module->getName() <<" instanced!" << endl;
        EV << "VirtualisationInfrastructureManager::instantiateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;

        instanceInfo.status = true;
        return instanceInfo;
    }
    else
    {
        MecAppInstanceInfo instanceInfo;
        instanceInfo.status = false;
        return instanceInfo;
    }
}

bool VirtualisationInfrastructureManager::terminateEmulatedMEApp(DeleteAppMessage* msg)
{
    int ueAppID = msg->getUeAppID();

    if(!meAppMap.empty() && meAppMap.find(ueAppID) != meAppMap.end())
    {
        //retrieve meAppMap map key
        int key = ueAppID;
        EV << "VirtualisationInfrastructureManager::terminateMEApp - " << meAppMap[key].meAppModule->getName() << " terminated!" << endl;
        currentMEApps--;
        EV << "VirtualisationInfrastructureManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;


        //deallocte resources
        deallocateResources(meAppMap[key].resources.ram, meAppMap[key].resources.disk, meAppMap[key].resources.cpu);

        //update map
        meAppMap.erase(ueAppID);

        return true;
    }
    else
    {
        EV << "VirtualisationInfrastructureManager::terminateMEApp - \tWARNING: NO INSTANCE FOUND!" << endl;
        return false;
    }
}

bool VirtualisationInfrastructureManager::terminateMEApp(DeleteAppMessage* msg)
{
    int ueAppID = msg->getUeAppID();

    if(!meAppMap.empty() && meAppMap.find(ueAppID) != meAppMap.end())
    {
        //retrieve meAppMap map key
        int key = ueAppID;

        EV << "VirtualisationInfrastructureManager::terminateMEApp - " << meAppMap[key].meAppModule->getName() << " terminated!" << endl;
        //terminating the ME App instance
        meAppMap[key].meAppModule->callFinish();
        meAppMap[key].meAppModule->deleteModule();
        currentMEApps--;
        EV << "VirtualisationInfrastructureManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;

        //Sending ACK_STOP_MEAPP to the UEApp
        EV << "VirtualisationInfrastructureManager::terminateMEApp - calling ackMEAppPacket with  "<< ACK_STOP_MEAPP << endl;

        //before to remove the map entry!
//        ackMEAppPacket(packet, ACK_STOP_MEAPP);

        int index = meAppMap[key].meAppGateIndex;
        int serviceIndex = meAppMap[key].serviceIndex;

        // TODO manage gates me app to at

        virtualisationInfr->gate("meAppOut", index)->getPreviousGate()->disconnect();
        virtualisationInfr->gate("meAppIn", index)->disconnect();

        if(serviceIndex >= 0)
        {
            //disconnecting internal MEPlatform gates to the MEService gates
            (meServices.at(serviceIndex))->gate("meAppOut", index)->disconnect();
            (meServices.at(serviceIndex))->gate("meAppIn", index)->disconnect();
            //disconnecting MEPlatform gates to the MEApp gates
            mePlatform->gate("meAppOut", index)->disconnect();
            mePlatform->gate("meAppIn", index)->disconnect();
        }


        //deallocte resources
        deallocateResources(meAppMap[key].resources.ram, meAppMap[key].resources.disk, meAppMap[key].resources.cpu);

        //update map
        meAppMap.erase(ueAppID);

        freeGates.push_back(index);

        return true;
    }
    else
    {
        EV << "VirtualisationInfrastructureManager::terminateMEApp - \tWARNING: NO INSTANCE FOUND!" << endl;
        return false;
    }
}

int VirtualisationInfrastructureManager::findService(const char* serviceName){
EV << "VirtualisationInfrastructureManager::findService " << serviceName << endl;

    if(!strcmp(serviceName, "NULL"))
        return NO_SERVICE;

    auto it = meServices.begin();
    for(; it != meServices.end(); ++it){
       if(!strcmp((*it)->getClassName(), serviceName))
           break;
    }
    if(it == meServices.end())
        return SERVICE_NOT_AVAILABLE;

    return it - meServices.begin();
}

bool VirtualisationInfrastructureManager::registerMecApp(int ueAppID, int reqRam, int reqDisk, double reqCpu)
{
    EV << "VirtualisationInfrastructureManager::registerMecApp - RAM: " << reqRam << " CPU: " << reqCpu << " disk: "<< reqDisk<< endl;
    printResources();
    if(meAppMap.find(ueAppID) == meAppMap.end())
    {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - independent MEC application with mecAppId ["<< ueAppID << "] already instantiated"<< endl;
    }

    if(isAllocable(reqRam, reqDisk, reqCpu))
    {
        //storing information about ME App allocated resources
        meAppMapEntry appEntry;

        appEntry.ueAppID = ueAppID;
        appEntry.resources.ram = reqRam;
        appEntry.resources.disk = reqDisk;
        appEntry.resources.cpu = reqCpu;
        meAppMap.insert({ueAppID, appEntry});

        EV << "VirtualisationInfrastructureManager::handleMEAppResources - resources ALLOCATED for independent MecApp with module id " << ueAppID  << endl;
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - ram: " << meAppMap[ueAppID].resources.ram <<" disk: "<< meAppMap[ueAppID].resources.disk <<" cpu: "<< meAppMap[ueAppID].resources.cpu << endl;
        allocateResources(reqRam, reqDisk, reqCpu);
        return true;
    }
    else{
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - resources NOT AVAILABLE for independent MecApp with module id " << ueAppID  << endl;
        return false;
    }
}

bool VirtualisationInfrastructureManager::deRegisterMecApp(int ueAppID)
{
    if(!meAppMap.empty() || meAppMap.find(ueAppID) != meAppMap.end())
    {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - resources DEALLOCATED for MecApp with UEAppId " << ueAppID  << endl;
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - ram: " << meAppMap[ueAppID].resources.ram <<" disk: "<< meAppMap[ueAppID].resources.disk <<" cpu: "<< meAppMap[ueAppID].resources.cpu << endl;
        deallocateResources(meAppMap[ueAppID].resources.ram, meAppMap[ueAppID].resources.disk, meAppMap[ueAppID].resources.cpu);
        //erasing map entry
        meAppMap.erase(ueAppID);
        return true;
    }
    else
    {
        EV << "VirtualisationInfrastructureManager::handleMEAppResources - NO ALLOCATION FOUND for MecApp with UEAppId id " << ueAppID  << endl;
        return false;
    }
}

double VirtualisationInfrastructureManager::calculateProcessingTime(int ueAppID, int numOfInstructions)
{
    ASSERT(numOfInstructions >= 0);

    auto ueApp = meAppMap.find(ueAppID);
    if(ueApp != meAppMap.end())
    {
        double time;
        double currentSpeed = ueApp->second.resources.cpu *(maxCPU/allocatedCPU);
        time = numOfInstructions/currentSpeed;
        EV << "VirtualisationInfrastructureManager::calculateProcessingTime - calculated time: " << time << endl;

        return time;
    }
    else
    {
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






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

#include "nodes/mec/MECOrchestrator/MecOrchestrator.h"

#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_types.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.h"


#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecServiceSelectionBased.h"
#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/AvailableResourcesSelectionBased.h"
#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecHostSelectionBased.h"


//emulation debug
#include <iostream>

Define_Module(MecOrchestrator);

MecOrchestrator::MecOrchestrator()
{
    meAppMap.clear();
    mecApplicationDescriptors_.clear();
    mecHostSelectionPolicy_ = nullptr;
}

void MecOrchestrator::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_LOCAL)
        return;
    EV << "MecOrchestrator::initialize - stage " << stage << endl;

    binder_ = getBinder();

    if(!strcmp(par("selectionPolicy"), "MecServiceBased"))
        mecHostSelectionPolicy_ = new MecServiceSelectionBased(this);
    else if(!strcmp(par("selectionPolicy"), "AvailableResourcesBased"))
        mecHostSelectionPolicy_ = new AvailableResourcesSelectionBased(this);
    else if(!strcmp(par("selectionPolicy"), "MecHostBased"))
            mecHostSelectionPolicy_ = new MecHostSelectionBased(this, par("mecHostIndex"));
    else
        throw cRuntimeError("MecOrchestrator::initialize - Selection policy %s not present!" , par("selectionPolicy").stringValue());

    onboardingTime = par("onboardingTime").doubleValue();
    instantiationTime = par("instantiationTime").doubleValue();
    terminationTime = par("terminationTime").doubleValue();

    getConnectedMecHosts();
    onboardApplicationPackages();

}

void MecOrchestrator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if(strcmp(msg->getName(), "MECOrchestratorMessage") == 0)
        {
            EV << "MecOrchestrator::handleMessage - "  << msg->getName() << endl;
            MECOrchestratorMessage* meoMsg = check_and_cast<MECOrchestratorMessage*>(msg);
            if(strcmp(meoMsg->getType(), CREATE_CONTEXT_APP) == 0)
            {
                if(meoMsg->getSuccess())
                    sendCreateAppContextAck(true, meoMsg->getRequestId(), meoMsg->getContextId());
                else
                    sendCreateAppContextAck(false, meoMsg->getRequestId());
            }
            else if( strcmp(meoMsg->getType(), DELETE_CONTEXT_APP) == 0)
                sendDeleteAppContextAck(meoMsg->getSuccess(), meoMsg->getRequestId(), meoMsg->getContextId());
        }
    }

    // handle message from the LCM proxy
    else if(msg->arrivedOn("fromUALCMP"))
    {
        EV << "MecOrchestrator::handleMessage - "  << msg->getName() << endl;
        handleUALCMPMessage(msg);
    }

    delete msg;
    return;

}

void MecOrchestrator::handleUALCMPMessage(cMessage* msg)
{
    UALCMPMessage* lcmMsg = check_and_cast<UALCMPMessage*>(msg);

    /* Handling CREATE_CONTEXT_APP */
    if(!strcmp(lcmMsg->getType(), CREATE_CONTEXT_APP))
        startMECApp(lcmMsg);

    /* Handling DELETE_CONTEXT_APP */
    else if(!strcmp(lcmMsg->getType(), DELETE_CONTEXT_APP))
        stopMECApp(lcmMsg);
}

void MecOrchestrator::startMECApp(UALCMPMessage* msg)
{
    CreateContextAppMessage* contAppMsg = check_and_cast<CreateContextAppMessage*>(msg);

    EV << "MecOrchestrator::createMeApp - processing... request id: " << contAppMsg->getRequestId() << endl;

    //retrieve UE App ID
    int ueAppID = atoi(contAppMsg->getDevAppId());

    /*
     * The Mec orchestrator has to decide where to deploy the MEC application.
     * - It checks if the MEC app has been already deployed
     * - It selects the most suitable MEC host     *
     */

    for(const auto& contextApp : meAppMap)
    {
        /*
         * TODO
         * set the check to provide multi UE to one mec application scenario.
         * For now the scenario is one to one, since the device application ID is used
         */
        if(contextApp.second.mecUeAppID == ueAppID && contextApp.second.appDId.compare(contAppMsg->getAppDId()) == 0)
        {
            //        meAppMap[ueAppID].lastAckStartSeqNum = pkt->getSno();
            //Sending ACK to the UEApp to confirm the instantiation in case of previous ack lost!
            //        ackMEAppPacket(ueAppID, ACK_START_MEAPP);
            //testing
            EV << "MecOrchestrator::startMECApp - \tWARNING: required MEC App instance ALREADY STARTED on MEC host: " << contextApp.second.mecHost->getName() << endl;
            EV << "MecOrchestrator::startMECApp  - sending ackMEAppPacket with "<< ACK_CREATE_CONTEXT_APP << endl;
            sendCreateAppContextAck(true, contAppMsg->getRequestId(), contextApp.first);
            return;
        }
    }


    std::string appDid;
    double processingTime = 0.0;

    if(contAppMsg->getOnboarded() == false)
    {
        // onboard app descriptor
        EV << "MecOrchestrator::startMECApp - onboarding appDescriptor from: " << contAppMsg->getAppPackagePath() << endl;
        const ApplicationDescriptor& appDesc = onboardApplicationPackage(contAppMsg->getAppPackagePath());
        appDid = appDesc.getAppDId();
        processingTime += onboardingTime;
    }
    else
    {
       appDid = contAppMsg->getAppDId();
    }

    auto it = mecApplicationDescriptors_.find(appDid);
    if(it == mecApplicationDescriptors_.end())
    {
        EV << "MecOrchestrator::startMECApp - Application package with AppDId["<< contAppMsg->getAppDId() << "] not onboarded." << endl;
        sendCreateAppContextAck(false, contAppMsg->getRequestId());
//        throw cRuntimeError("MecOrchestrator::startMECApp - Application package with AppDId[%s] not onboarded", contAppMsg->getAppDId());
    }

    const ApplicationDescriptor& desc = it->second;

    cModule* bestHost = mecHostSelectionPolicy_->findBestMecHost(desc);

    if(bestHost != nullptr)
    {
        CreateAppMessage * createAppMsg = new CreateAppMessage();

        createAppMsg->setUeAppID(atoi(contAppMsg->getDevAppId()));
        createAppMsg->setMEModuleName(desc.getAppName().c_str());
        createAppMsg->setMEModuleType(desc.getAppProvider().c_str());

        createAppMsg->setRequiredCpu(desc.getVirtualResources().cpu);
        createAppMsg->setRequiredRam(desc.getVirtualResources().ram);
        createAppMsg->setRequiredDisk(desc.getVirtualResources().disk);

        // This field is useful for mec services no etsi mec compliant (e.g. omnet++ like)
        // In such case, the vim has to connect the gates between the mec application and the service

        // insert OMNeT like services, only one is supported, for now
        if(!desc.getOmnetppServiceRequired().empty())
            createAppMsg->setRequiredService(desc.getOmnetppServiceRequired().c_str());
        else
            createAppMsg->setRequiredService("NULL");

        createAppMsg->setContextId(contextIdCounter);

     // add the new mec app in the map structure
        mecAppMapEntry newMecApp;
        newMecApp.appDId = appDid;
        newMecApp.mecUeAppID =  ueAppID;
        newMecApp.mecHost = bestHost;
        newMecApp.ueAddress = inet::L3AddressResolver().resolve(contAppMsg->getUeIpAddress());
        newMecApp.vim = bestHost->getSubmodule("vim");
        newMecApp.mecpm = bestHost->getSubmodule("mecPlatformManager");

        newMecApp.mecAppName = desc.getAppName().c_str();
//        newMecApp.lastAckStartSeqNum = pkt->getSno();


         MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(newMecApp.mecpm);

         /*
          * If the application descriptor refers to a simulated mec app, the system eventually instances the mec app object.
          * If the application descriptor refers to a mec application running outside the simulator, i.e emulation mode,
          * the system allocates the resources, without instantiate any module.
          * The application descriptor contains the address and port information to communicate with the mec application
          */

         MecAppInstanceInfo* appInfo = nullptr;

         if(desc.isMecAppEmulated())
         {
             EV << "MecOrchestrator::startMECApp - MEC app is emulated" << endl;
             bool result = mecpm->instantiateEmulatedMEApp(createAppMsg);
             appInfo = new MecAppInstanceInfo();
             appInfo->status = result;
             appInfo->endPoint.addr = inet::L3Address(desc.getExternalAddress().c_str());
             appInfo->endPoint.port = desc.getExternalPort();
             appInfo->instanceId = "emulated_" + desc.getAppName();
             newMecApp.isEmulated = true;

             // register the address of the MEC app to the Binder, so as the GTP knows the endpoint (UPF_MEC) where to forward packets to
             inet::L3Address gtpAddress = inet::L3AddressResolver().resolve(newMecApp.mecHost->getSubmodule("upf_mec")->getFullPath().c_str());
             binder_->registerMecHostUpfAddress(appInfo->endPoint.addr, gtpAddress);
         }
         else
         {
             appInfo = mecpm->instantiateMEApp(createAppMsg);
             newMecApp.isEmulated = false;
         }

         if(!appInfo->status)
         {
             EV << "MecOrchestrator::startMECApp - something went wrong during MEC app instantiation"<< endl;
             MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
             msg->setType(CREATE_CONTEXT_APP);
             msg->setRequestId(contAppMsg->getRequestId());
             msg->setSuccess(false);
             processingTime += instantiationTime;
             scheduleAt(simTime() + processingTime, msg);
             return;
         }

         EV << "MecOrchestrator::startMECApp - new MEC application with name: " << appInfo->instanceId << " instantiated on MEC host []"<< newMecApp.mecHost << " at "<< appInfo->endPoint.addr.str() << ":" << appInfo->endPoint.port << endl;

         newMecApp.mecAppAddress = appInfo->endPoint.addr;
         newMecApp.mecAppPort = appInfo->endPoint.port;
         newMecApp.mecAppIsntanceId = appInfo->instanceId;
         newMecApp.contextId = contextIdCounter;
         meAppMap[contextIdCounter] = newMecApp;

         MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
         msg->setContextId(contextIdCounter);
         msg->setType(CREATE_CONTEXT_APP);
         msg->setRequestId(contAppMsg->getRequestId());
         msg->setSuccess(true);

         contextIdCounter++;

         processingTime += instantiationTime;
         scheduleAt(simTime() + processingTime, msg);

         delete appInfo;
    }
    else
    {
        //throw cRuntimeError("MecOrchestrator::startMECApp - A suitable MEC host has not been selected");
        EV << "MecOrchestrator::startMECApp - A suitable MEC host has not been selected" << endl;
        MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
        msg->setType(CREATE_CONTEXT_APP);
        msg->setRequestId(contAppMsg->getRequestId());
        msg->setSuccess(false);
        processingTime += instantiationTime/2;
        scheduleAt(simTime() + processingTime, msg);
    }

}

void MecOrchestrator::stopMECApp(UALCMPMessage* msg){
    EV << "MecOrchestrator::stopMECApp - processing..." << endl;

    DeleteContextAppMessage* contAppMsg = check_and_cast<DeleteContextAppMessage*>(msg);

    int contextId = contAppMsg->getContextId();
    EV << "MecOrchestrator::stopMECApp - processing contextId: "<< contextId << endl;
    // checking if ueAppIdToMeAppMapKey entry map does exist
    if(meAppMap.empty() || (meAppMap.find(contextId) == meAppMap.end()))
    {
        // maybe it has already been deleted
        EV << "MecOrchestrator::stopMECApp - \tWARNING Mec Application ["<< meAppMap[contextId].mecUeAppID <<"] not found!" << endl;
        sendDeleteAppContextAck(false, contAppMsg->getRequestId(), contextId);
//        throw cRuntimeError("MecOrchestrator::stopMECApp - \tERROR ueAppIdToMeAppMapKey entry not found!");
        return;
    }

    // call the methods of resource manager and virtualization infrastructure of the selected mec host to deallocate the resources

     MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(meAppMap[contextId].mecpm);
//     VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(meAppMap[contextId].vim);

     DeleteAppMessage* deleteAppMsg = new DeleteAppMessage();
     deleteAppMsg->setUeAppID(meAppMap[contextId].mecUeAppID);

     bool isTerminated;
     if(meAppMap[contextId].isEmulated)
     {
         isTerminated =  mecpm->terminateEmulatedMEApp(deleteAppMsg);
         std::cout << "terminateEmulatedMEApp with result: " << isTerminated << std::endl;
     }
     else
     {
         isTerminated =  mecpm->terminateMEApp(deleteAppMsg);
     }

     MECOrchestratorMessage *mecoMsg = new MECOrchestratorMessage("MECOrchestratorMessage");
     mecoMsg->setType(DELETE_CONTEXT_APP);
     mecoMsg->setRequestId(contAppMsg->getRequestId());
     mecoMsg->setContextId(contAppMsg->getContextId());
     if(isTerminated)
     {
         EV << "MecOrchestrator::stopMECApp - mec Application ["<< meAppMap[contextId].mecUeAppID << "] removed"<< endl;
         meAppMap.erase(contextId);
         mecoMsg->setSuccess(true);
     }
     else
     {
         EV << "MecOrchestrator::stopMECApp - mec Application ["<< meAppMap[contextId].mecUeAppID << "] not removed"<< endl;
         mecoMsg->setSuccess(false);
     }

    double processingTime = terminationTime;
    scheduleAt(simTime() + processingTime, mecoMsg);

}

void MecOrchestrator::sendDeleteAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    EV << "MecOrchestrator::sendDeleteAppContextAck - result: "<<result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    DeleteContextAppAckMessage * ack = new DeleteContextAppAckMessage();
    ack->setType(ACK_DELETE_CONTEXT_APP);
    ack->setRequestId(requestSno);
    ack->setSuccess(result);

    send(ack, "toUALCMP");
}

void MecOrchestrator::sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    EV << "MecOrchestrator::sendCreateAppContextAck - result: "<< result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    CreateContextAppAckMessage *ack = new CreateContextAppAckMessage();
    ack->setType(ACK_CREATE_CONTEXT_APP);
    if(result)
    {
        if(meAppMap.empty() || meAppMap.find(contextId) == meAppMap.end())
        {
            EV << "MecOrchestrator::ackMEAppPacket - ERROR meApp["<< contextId << "] does not exist!" << endl;
//            throw cRuntimeError("MecOrchestrator::ackMEAppPacket - ERROR meApp[%d] does not exist!", contextId);
            return;
        }

        mecAppMapEntry mecAppStatus = meAppMap[contextId];

        ack->setSuccess(true);
        ack->setContextId(contextId);
        ack->setAppInstanceId(mecAppStatus.mecAppIsntanceId.c_str());
        ack->setRequestId(requestSno);
        std::stringstream uri;

        uri << mecAppStatus.mecAppAddress.str()<<":"<<mecAppStatus.mecAppPort;

        ack->setAppInstanceUri(uri.str().c_str());
    }
    else
    {
        ack->setSuccess(false);
    }
    send(ack, "toUALCMP");
}


cModule* MecOrchestrator::findBestMecHost(const ApplicationDescriptor& appDesc)
{

    EV << "MecOrchestrator::findBestMecHost - finding best MecHost..." << endl;
    cModule* bestHost = nullptr;

    for(auto mecHost : mecHosts)
    {
        VirtualisationInfrastructureManager *vim = check_and_cast<VirtualisationInfrastructureManager*> (mecHost->getSubmodule("vim"));
        ResourceDescriptor resources = appDesc.getVirtualResources();
        bool res = vim->isAllocable(resources.ram, resources.disk, resources.cpu);
        if(!res)
        {
            EV << "MecOrchestrator::findBestMecHost - MEC host []"<< mecHost << " has not got enough resources. Searching again..." << endl;
            continue;
        }

        // Temporally select this mec host as the best
        EV << "MecOrchestrator::findBestMecHost - MEC host []"<< mecHost << " temporally chosen as bet MEC host, checking for the required MEC services.." << endl;
        bestHost = mecHost;

        MecPlatformManager *mecpm = check_and_cast<MecPlatformManager*> (mecHost->getSubmodule("mecPlatformManager"));
        auto mecServices = mecpm ->getAvailableMecServices();
        std::string serviceName;

        /* I assume the app requires only one mec service */
        if(appDesc.getAppServicesRequired().size() > 0)
        {
            serviceName =  appDesc.getAppServicesRequired()[0];
        }
        else
        {
            break;
        }
        auto it = mecServices->begin();
        for(; it != mecServices->end() ; ++it)
        {
            if(serviceName.compare(it->getName()) == 0)
            {
               bestHost = mecHost;
               break;
            }
        }
    }
    if(bestHost != nullptr)
        EV << "MecOrchestrator::findBestMecHost - best MEC host: " << bestHost << endl;
    else
        EV << "MecOrchestrator::findBestMecHost - no MEC host found"<< endl;

    return bestHost;
}

void MecOrchestrator::getConnectedMecHosts()
{
    //getting the list of mec hosts associated to this mec system from parameter
    if(this->hasPar("mecHostList") && strcmp(par("mecHostList").stringValue(), "")){
        std::string mecHostList = par("mecHostList").stdstringValue();
        EV <<"MecOrchestrator::getConnectedMecHosts - mecHostList: "<< par("mecHostList").stringValue() << endl;
        char* token = strtok ( (char*)mecHostList.c_str(), ", ");            // split by commas

        while (token != NULL)
        {
            EV <<"MecOrchestrator::getConnectedMecHosts - mec host (from par): "<< token << endl;
            cModule *mecHostModule = getSimulation()->getModuleByPath(token);
            mecHosts.push_back(mecHostModule);
            token = strtok (NULL, ", ");
        }
    }
    else{
//        throw cRuntimeError ("MecOrchestrator::getConnectedMecHosts - No mecHostList found");
        EV << "MecOrchestrator::getConnectedMecHosts - No mecHostList found" << endl;
    }

}

const ApplicationDescriptor& MecOrchestrator::onboardApplicationPackage(const char* fileName)
{
    EV <<"MecOrchestrator::onBoardApplicationPackages - onboarding application package (from request): "<< fileName << endl;
    ApplicationDescriptor appDesc(fileName);
    if(mecApplicationDescriptors_.find(appDesc.getAppDId()) != mecApplicationDescriptors_.end())
    {
        EV << "MecOrchestrator::onboardApplicationPackages() - Application descriptor with appName ["<< fileName << "] is already present.\n" << endl;
//        throw cRuntimeError("MecOrchestrator::onboardApplicationPackages() - Application descriptor with appName [%s] is already present.\n"
//                            "Duplicate appDId or application package already onboarded?", fileName);
    }
    else
    {
        mecApplicationDescriptors_[appDesc.getAppDId()] = appDesc; // add to the mecApplicationDescriptors_
    }

    return mecApplicationDescriptors_[appDesc.getAppDId()];
}

void MecOrchestrator::registerMecService(ServiceDescriptor& serviceDescriptor) const
{
    EV << "MecOrchestrator::registerMecService - Registering MEC service [" << serviceDescriptor.name << "]" << endl;
    for(auto mecHost : mecHosts)
    {
        cModule* module = mecHost->getSubmodule("mecPlatform")->getSubmodule("serviceRegistry");
        if(module != nullptr)
        {
            EV << "MecOrchestrator::registerMecService - Registering MEC service ["<<serviceDescriptor.name << "] in MEC host [" << mecHost->getName()<<"]" << endl;
            ServiceRegistry* serviceRegistry = check_and_cast<ServiceRegistry*>(module);
            serviceRegistry->registerMecService(serviceDescriptor);
        }
    }
}

void MecOrchestrator::onboardApplicationPackages()
{
    //getting the list of mec hosts associated to this mec system from parameter
    if(this->hasPar("mecApplicationPackageList") && strcmp(par("mecApplicationPackageList").stringValue(), "")){

        char* token = strtok ( (char*) par("mecApplicationPackageList").stringValue(), ", ");            // split by commas

        while (token != NULL)
        {
            int len = strlen(token);
            char buf[len+strlen(".json")+strlen("ApplicationDescriptors/")+1];
            strcpy(buf,"ApplicationDescriptors/");
            strcat(buf,token);
            strcat(buf,".json");
            onboardApplicationPackage(buf);
            token = strtok (NULL, ", ");
        }
    }
    else{
        EV << "MecOrchestrator::onboardApplicationPackages - No mecApplicationPackageList found" << endl;
    }

}

const ApplicationDescriptor* MecOrchestrator::getApplicationDescriptorByAppName(std::string& appName) const
{
    for(const auto& appDesc : mecApplicationDescriptors_)
    {
        if(appDesc.second.getAppName().compare(appName) == 0)
            return &(appDesc.second);

    }

    return nullptr;
}



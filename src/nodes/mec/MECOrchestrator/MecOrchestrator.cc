/*
 * MecOrchestrator.cc
 *
 *  Created on: Apr 26, 2021
 *      Author: linofex
 */

#include "nodes/mec/MECOrchestrator/MecOrchestrator.h"
#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "nodes/mec/MecCommon.h"
#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

#include "nodes/mec/LCMProxy/LCMProxyMessages/LcmProxyMessages_m.h"
#include "nodes/mec/LCMProxy/LCMProxyMessages/LCMProxyMessages_types.h"
#include "nodes/mec/LCMProxy/LCMProxyMessages/CreateContextAppMessage.h"
#include "nodes/mec/LCMProxy/LCMProxyMessages/CreateContextAppAckMessage.h"


Define_Module(MecOrchestrator);

MecOrchestrator::MecOrchestrator()
{
}

void MecOrchestrator::initialize(int stage)
{
    EV << "MecOrchestrator::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    getConnectedMecHosts();
    onboardApplicationPackages();
    binder_ = getBinder();

}

void MecOrchestrator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if(strcmp(msg->getName(), "MECOrchestratorMessage") == 0)
        {
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

//    //handling resource allocation confirmation
    else if(msg->arrivedOn("fromLcmProxy"))
    {
          handleLcmProxyMessage(msg);
    }

    delete msg;
    return;

}

/*
 * ######################################################################################################
 */
/*
 * #######################################MEAPP PACKETS HANDLERS####################################
 */

void MecOrchestrator::handleLcmProxyMessage(cMessage* msg){

    LcmProxyMessage* lcmMsg = check_and_cast<LcmProxyMessage*>(msg);

    /* Handling START_MEAPP */
    if(!strcmp(lcmMsg->getType(), CREATE_CONTEXT_APP))
        startMEApp(lcmMsg);

    /* Handling STOP_MEAPP */
    else if(!strcmp(lcmMsg->getType(), DELETE_CONTEXT_APP))
        stopMEApp(lcmMsg);
}

void MecOrchestrator::startMEApp(LcmProxyMessage* msg){


    CreateContextAppMessage* contAppMsg = check_and_cast<CreateContextAppMessage*>(msg);

    EV << "MecOrchestrator::createMeApp - processing... request id: " << contAppMsg->getRequestId() << endl;

    //retrieve UE App ID
    int ueAppID = atoi(contAppMsg->getDevAppId());
    /*
     * The Mec orchestrator has to decide where to deploy the mec application.
     * It has to check if the Meapp has been already deployed
     *
     */

    for(const auto& contextApp : meAppMap)
    {
        if(contextApp.second.mecUeAppID == ueAppID)
        {
        //        meAppMap[ueAppID].lastAckStartSeqNum = pkt->getSno();
        //Sending ACK to the UEApp to confirm the instantiation in case of previous ack lost!
        //        ackMEAppPacket(ueAppID, ACK_START_MEAPP);
        //testing
        EV << "MecOrchestrator::startMEApp - \tWARNING: required MEApp instance ALREADY STARTED on MEC host: " << contextApp.second.mecHost->getName() << endl;
        EV << "MecOrchestrator::startMEApp  - calling ackMEAppPacket with  "<< ACK_START_MEAPP << endl;
        sendCreateAppContextAck(true, contAppMsg->getRequestId(), contextApp.first);
        return;
        }
    }


    std::string appDid;
    double processingTime = 0.0;

    if(contAppMsg->getOnboarded() == false)
    {
       const ApplicationDescriptor& appDesc = onboardApplicationPackage(contAppMsg->getAppPackagePath());
       appDid = appDesc.getAppDId();
       processingTime += 0.01;
    }
    else
    {
       appDid = contAppMsg->getAppDId();
    }

    auto it = mecApplicationDescriptors_.find(appDid);
    if(it == mecApplicationDescriptors_.end())
       throw cRuntimeError("MecOrchestrator::startMEApp - Application package with AppDId[%s] not onboarded", contAppMsg->getAppDId());

    const ApplicationDescriptor& desc = it->second;

    cModule* bestHost = findBestMecHost(desc);

    if(bestHost != nullptr)
    {
        CreateAppMessage * createAppMsg = new CreateAppMessage();

        createAppMsg->setUeAppID(atoi(contAppMsg->getDevAppId()));
        createAppMsg->setMEModuleName(desc.getAppName().c_str());
        createAppMsg->setMEModuleType(desc.getAppProvider().c_str());
        createAppMsg->setMEModuleType(desc.getAppProvider().c_str());

        createAppMsg->setRequiredCpu(desc.getVirtualResources().cpu);
        createAppMsg->setRequiredRam(desc.getVirtualResources().ram);
        createAppMsg->setRequiredDisk(desc.getVirtualResources().disk);

        if(desc.getAppServicesRequired().size() != 0)
            createAppMsg->setRequiredService(desc.getAppServicesRequired()[0].c_str());
        else
            createAppMsg->setRequiredService("NULL");

        if(desc.getAppServicesProduced().size() != 0)
            createAppMsg->setProvidedService(desc.getAppServicesProduced()[0].c_str());
        else
            createAppMsg->setProvidedService("NULL");

        createAppMsg->setContextId(contextIdCounter);

     // add the new mec app in the map structure
        mecAppMapEntry newMecApp;
        newMecApp.appDId = appDid;
        newMecApp.mecUeAppID =  ueAppID;
        newMecApp.mecHost = bestHost;
//        newMecApp.ueSymbolicAddres = pkt->getSourceAddress();
        newMecApp.ueAddress = inet::L3AddressResolver().resolve(contAppMsg->getUeIpAddress());
//        newMecApp.uePort = pkt->getSourcePort();
        newMecApp.vim = bestHost->getSubmodule("vim");
        newMecApp.mecpm = bestHost->getSubmodule("mecPlatformManager");

        newMecApp.mecAppName = desc.getAppName().c_str();
//        newMecApp.lastAckStartSeqNum = pkt->getSno();

        // call the methods of resource manager and virtualization infrastructure of the selected bestHost

     MecPlatformManager* mecpm = check_and_cast<MecPlatformManager*>(newMecApp.mecpm);
//     VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(newMecApp.vim);

     double reqRam    = desc.getVirtualResources().ram;
     double reqDisk   = desc.getVirtualResources().disk;
     double reqCpu = desc.getVirtualResources().cpu;


     /*
      * If the application descriptor refers to a simulated mec app, the system eventually instance the mec app object.
      * If the application descriptor refers to a mec application running outside the simulator, i.e emulation mode,
      * the system allocates the resources, without instantiate any module.
      * The applciation descriptor contains the address and port information to communicate with the mec application
      */

     MecAppInstanceInfo appInfo;
     if(desc.isMecAppEmulated())
     {
         bool result = mecpm->instantiateEmulatedMEApp(createAppMsg);
         appInfo.status = result;
         appInfo.endPoint.addr = inet::L3Address(desc.getExternalAddress());
         appInfo.endPoint.port = desc.getExternalPort();
         appInfo.instanceId = "emulated_" + desc.getAppName();
         newMecApp.isEmulated = true;
     }
     else
     {
         appInfo = mecpm->instantiateMEApp(createAppMsg);
         newMecApp.isEmulated = false;

     }

     if(!appInfo.status)
     {
         MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
         msg->setType(CREATE_CONTEXT_APP);
         msg->setRequestId(contAppMsg->getRequestId());
         msg->setSuccess(false);
         processingTime += 0.010;
         scheduleAt(simTime() + processingTime, msg);
         return;
     }

     EV << "VirtualisationManager::instantiateMEApp new MEC applciation with name: " << appInfo.instanceId << " instantiated on " << appInfo.endPoint.addr.str() << ":" << appInfo.endPoint.port << endl;

     newMecApp.mecAppAddress = appInfo.endPoint.addr;
     newMecApp.mecAppPort = appInfo.endPoint.port;
     newMecApp.mecAppIsntanceId = appInfo.instanceId;
     newMecApp.contextId = contextIdCounter;
     meAppMap[contextIdCounter] = newMecApp;

     MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
     msg->setContextId(contextIdCounter);
     msg->setType(CREATE_CONTEXT_APP);
     msg->setRequestId(contAppMsg->getRequestId());
     msg->setSuccess(true);

     contextIdCounter++;

     processingTime += 0.010;
     scheduleAt(simTime() + processingTime, msg);
    }
    else
    {
        MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
        msg->setType(CREATE_CONTEXT_APP);
        msg->setRequestId(contAppMsg->getRequestId());
        msg->setSuccess(false);
        processingTime += 0.010;
        scheduleAt(simTime() + processingTime, msg);
    }

}

void MecOrchestrator::stopMEApp(LcmProxyMessage* msg){

    EV << "MecOrchestrator::stopMEApp - processing..." << endl;

    DeleteContextAppMessage* contAppMsg = check_and_cast<DeleteContextAppMessage*>(msg);

    int contextId = contAppMsg->getContextId();
    EV << "MecOrchestrator::stopMEApp - processing contextId: "<< contextId << endl;
    //checking if ueAppIdToMeAppMapKey entry map does exist
    if(meAppMap.empty() || meAppMap.find(contextId) == meAppMap.end())
    {
//      maybe it has been deleted
        EV << "MecOrchestrator::stopMEApp - \tWARNING forwarding to meAppMap["<< meAppMap[contextId].mecUeAppID <<"] not found!" << endl;
        throw cRuntimeError("MecOrchestrator::stopMEApp - \tERROR ueAppIdToMeAppMapKey entry not found!");
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
         meAppMap.erase(contextId);
         EV << "MecOrchestrator::stopMEApp - mec Application ["<< meAppMap[contextId].mecUeAppID << "] removed"<< endl;
         mecoMsg->setSuccess(true);
     }
     else
     {
         EV << "MecOrchestrator::stopMEApp - mec Application ["<< meAppMap[contextId].mecUeAppID << "] not removed"<< endl;
         mecoMsg->setSuccess(false);
     }

    double processingTime = 0.010;
    scheduleAt(simTime() + processingTime, mecoMsg);

}

void MecOrchestrator::sendDeleteAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    EV << "MecOrchestrator::sendDeleteAppContextAck - result: "<<result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    DeleteContextAppAckMessage * ack = new DeleteContextAppAckMessage();
    ack->setType(ACK_DELETE_CONTEXT_APP);
    ack->setRequestId(requestSno);
    ack->setSuccess(result);

    send(ack, "toLcmProxy");
}

void MecOrchestrator::sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    EV << "MecOrchestrator::sendCreateAppContextAck - result: "<<result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    CreateContextAppAckMessage *ack = new CreateContextAppAckMessage();
    ack->setType(ACK_CREATE_CONTEXT_APP);
    if(result)
    {
        if(meAppMap.empty() || meAppMap.find(contextId) == meAppMap.end())
        {
            EV << "MecOrchestrator::ackMEAppPacket - ERROR meApp["<< contextId << "] does not exist!" << endl;
            throw cRuntimeError("MecOrchestrator::ackMEAppPacket - ERROR meApp[%d] does not exist!", contextId);
            return;
        }
    //
        mecAppMapEntry mecAppStatus = meAppMap[contextId];

//        EV << "MecOrchestrator::ackMEAppPacket - sending successful ack  to: [" << destAddress_.str() <<"]" << endl;
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
    send(ack, "toLcmProxy");
}




cModule* MecOrchestrator::findBestMecHost(const ApplicationDescriptor& appDesc){return mecHosts[0];}

void MecOrchestrator::getConnectedMecHosts()
{
    //getting the list of mec hosts associated to this mec system from parameter
    if(this->hasPar("mecHostList") && strcmp(par("mecHostList").stringValue(), "")){

        char* token = strtok ( (char*) par("mecHostList").stringValue(), ", ");            // split by commas

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
    EV <<"MecOrchestrator::onBoardApplicationPackages - onboarding application package (from par): "<< fileName << endl;
    ApplicationDescriptor appDesc(fileName);
    if(mecApplicationDescriptors_.find(appDesc.getAppDId()) != mecApplicationDescriptors_.end())
        throw cRuntimeError("MecOrchestrator::onboardApplicationPackages() - Application descriptor with appName [%s] is already present.\n"
                            "Duplicate appName or application package already onboarded?");

    mecApplicationDescriptors_[appDesc.getAppDId()] = appDesc;
    return mecApplicationDescriptors_[appDesc.getAppDId()];
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



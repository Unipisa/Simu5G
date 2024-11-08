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
#include "apps/mec/MecApps/MultiUEMECApp.h"

#include "nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.h"

#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_types.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.h"

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecServiceSelectionBased.h"
#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/AvailableResourcesSelectionBased.h"
#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecHostSelectionBased.h"

// Emulation debug
#include <iostream>

namespace simu5g {

Define_Module(MecOrchestrator);


void MecOrchestrator::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    // Avoid multiple initializations
    if (stage != inet::INITSTAGE_LOCAL)
        return;
    EV << "MecOrchestrator::initialize - stage " << stage << endl;

    binder_.reference(this, "binderModule", true);

    const char *selectionPolicyPar = par("selectionPolicy");
    if (!strcmp(selectionPolicyPar, "MecServiceBased"))
        mecHostSelectionPolicy_ = new MecServiceSelectionBased(this);
    else if (!strcmp(selectionPolicyPar, "AvailableResourcesBased"))
        mecHostSelectionPolicy_ = new AvailableResourcesSelectionBased(this);
    else if (!strcmp(selectionPolicyPar, "MecHostBased"))
        mecHostSelectionPolicy_ = new MecHostSelectionBased(this, par("mecHostIndex"));
    else
        throw cRuntimeError("MecOrchestrator::initialize - Selection policy '%s' not present!", selectionPolicyPar);

    onboardingTime = par("onboardingTime").doubleValue();
    instantiationTime = par("instantiationTime").doubleValue();
    terminationTime = par("terminationTime").doubleValue();

    getConnectedMecHosts();
    onboardApplicationPackages();
}

void MecOrchestrator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "MECOrchestratorMessage") == 0) {
            EV << "MecOrchestrator::handleMessage - " << msg->getName() << endl;
            MECOrchestratorMessage *meoMsg = check_and_cast<MECOrchestratorMessage *>(msg);
            if (strcmp(meoMsg->getType(), CREATE_CONTEXT_APP) == 0) {
                if (meoMsg->getSuccess())
                    sendCreateAppContextAck(true, meoMsg->getRequestId(), meoMsg->getContextId());
                else
                    sendCreateAppContextAck(false, meoMsg->getRequestId());
            }
            else if (strcmp(meoMsg->getType(), DELETE_CONTEXT_APP) == 0)
                sendDeleteAppContextAck(meoMsg->getSuccess(), meoMsg->getRequestId(), meoMsg->getContextId());
        }
    }
    // Handle message from the LCM proxy
    else if (msg->arrivedOn("fromUALCMP")) {
        EV << "MecOrchestrator::handleMessage - " << msg->getName() << endl;
        handleUALCMPMessage(msg);
    }

    delete msg;
}

void MecOrchestrator::handleUALCMPMessage(cMessage *msg)
{
    UALCMPMessage *lcmMsg = check_and_cast<UALCMPMessage *>(msg);

    // Handling CREATE_CONTEXT_APP
    if (!strcmp(lcmMsg->getType(), CREATE_CONTEXT_APP))
        startMECApp(lcmMsg);

    // Handling DELETE_CONTEXT_APP
    else if (!strcmp(lcmMsg->getType(), DELETE_CONTEXT_APP))
        stopMECApp(lcmMsg);
}

void MecOrchestrator::startMECApp(UALCMPMessage *msg)
{
    CreateContextAppMessage *contAppMsg = check_and_cast<CreateContextAppMessage *>(msg);

    EV << "MecOrchestrator::createMeApp - processing... request id: " << contAppMsg->getRequestId() << endl;

    // Retrieve UE App ID
    int ueAppID = atoi(contAppMsg->getDevAppId());

    /*
     * The MEC orchestrator has to decide where to deploy the MEC application.
     * - It checks if the MEC app has already been deployed
     * - It selects the most suitable MEC host
     */

    for (const auto& contextApp : meAppMap)
    {
        if (contextApp.second.mecUeAppID == ueAppID && contextApp.second.appDId.compare(contAppMsg->getAppDId()) == 0)
        {
            EV << "MecOrchestrator::startMECApp - \tWARNING: required MEC App instance ALREADY STARTED on MEC host: " << contextApp.second.mecHost->getName() << endl;
            EV << "MecOrchestrator::startMECApp  - sending ackMEAppPacket with " << ACK_CREATE_CONTEXT_APP << endl;
            sendCreateAppContextAck(true, contAppMsg->getRequestId(), contextApp.first);
            auto* existingMECApp = dynamic_cast<MultiUEMECApp*>(contextApp.second.reference);
            if (existingMECApp) {
                // if the app already exist and it is an app supporting multiple UEs, then notify the app about the new UE
                struct UE_MEC_CLIENT newUE;
                newUE.address = inet::L3Address(contAppMsg->getUeIpAddress());
                // the UE port is not known at this stage
                newUE.port = -1;
                existingMECApp->addNewUE(newUE);
            }
            else
                return;
        }
    }

    std::string appDid;
    double processingTime = 0.0;

    if (contAppMsg->getOnboarded() == false) {
        // Onboard app descriptor
        EV << "MecOrchestrator::startMECApp - onboarding appDescriptor from: " << contAppMsg->getAppPackagePath() << endl;
        const ApplicationDescriptor& appDesc = onboardApplicationPackage(contAppMsg->getAppPackagePath());
        appDid = appDesc.getAppDId();
        processingTime += onboardingTime;
    }
    else {
        appDid = contAppMsg->getAppDId();
    }

    auto it = mecApplicationDescriptors_.find(appDid);
    if (it == mecApplicationDescriptors_.end()) {
        EV << "MecOrchestrator::startMECApp - Application package with AppDId[" << contAppMsg->getAppDId() << "] not onboarded." << endl;
        sendCreateAppContextAck(false, contAppMsg->getRequestId());
    }

    const ApplicationDescriptor& desc = it->second;

    cModule *bestHost = mecHostSelectionPolicy_->findBestMecHost(desc);

    if (bestHost != nullptr) {
        CreateAppMessage *createAppMsg = new CreateAppMessage();

        createAppMsg->setUeAppID(atoi(contAppMsg->getDevAppId()));
        createAppMsg->setMEModuleName(desc.getAppName().c_str());
        createAppMsg->setMEModuleType(desc.getAppProvider().c_str());

        createAppMsg->setRequiredCpu(desc.getVirtualResources().cpu);
        createAppMsg->setRequiredRam(desc.getVirtualResources().ram);
        createAppMsg->setRequiredDisk(desc.getVirtualResources().disk);

        // This field is useful for MEC services not ETSI MEC compliant (e.g. OMNeT++ like)
        // In such cases, the VIM must connect the gates between the MEC application and the service

        // Insert OMNeT like services, only one is supported for now
        if (!desc.getOmnetppServiceRequired().empty())
            createAppMsg->setRequiredService(desc.getOmnetppServiceRequired().c_str());
        else
            createAppMsg->setRequiredService("NULL");

        createAppMsg->setContextId(contextIdCounter);

        // Add the new MEC app in the map structure
        mecAppMapEntry newMecApp;
        newMecApp.appDId = appDid;
        newMecApp.mecUeAppID = ueAppID;
        newMecApp.mecHost = bestHost;
        newMecApp.ueAddress = inet::L3AddressResolver().resolve(contAppMsg->getUeIpAddress());
        newMecApp.vim = bestHost->getSubmodule("vim");
        newMecApp.mecpm = bestHost->getSubmodule("mecPlatformManager");

        newMecApp.mecAppName = desc.getAppName().c_str();
        MecPlatformManager *mecpm = check_and_cast<MecPlatformManager *>(newMecApp.mecpm);

        /*
         * If the application descriptor refers to a simulated MEC app, the system eventually instantiates the MEC app object.
         * If the application descriptor refers to a MEC application running outside the simulator, i.e., emulation mode,
         * the system allocates the resources without instantiating any module.
         * The application descriptor contains the address and port information to communicate with the MEC application.
         */

        MecAppInstanceInfo *appInfo = nullptr;

        if (desc.isMecAppEmulated()) {
            EV << "MecOrchestrator::startMECApp - MEC app is emulated" << endl;
            bool result = mecpm->instantiateEmulatedMEApp(createAppMsg);
            appInfo = new MecAppInstanceInfo();
            appInfo->status = result;
            appInfo->endPoint.addr = inet::L3Address(desc.getExternalAddress().c_str());
            appInfo->endPoint.port = desc.getExternalPort();
            appInfo->instanceId = "emulated_" + desc.getAppName();
            newMecApp.isEmulated = true;

            // Register the address of the MEC app to the Binder, so the GTP knows the endpoint (UPF_MEC) where to forward packets to
            inet::L3Address gtpAddress = inet::L3AddressResolver().resolve(newMecApp.mecHost->getSubmodule("upf_mec")->getFullPath().c_str());
            binder_->registerMecHostUpfAddress(appInfo->endPoint.addr, gtpAddress);
        }
        else {
            appInfo = mecpm->instantiateMEApp(createAppMsg);
            newMecApp.isEmulated = false;
        }

        if (!appInfo->status) {
            EV << "MecOrchestrator::startMECApp - something went wrong during MEC app instantiation" << endl;
            MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
            msg->setType(CREATE_CONTEXT_APP);
            msg->setRequestId(contAppMsg->getRequestId());
            msg->setSuccess(false);
            processingTime += instantiationTime;
            scheduleAt(simTime() + processingTime, msg);
            return;
        }

        EV << "MecOrchestrator::startMECApp - new MEC application with name: " << appInfo->instanceId << " instantiated on MEC host []" << newMecApp.mecHost << " at " << appInfo->endPoint.addr.str() << ":" << appInfo->endPoint.port << endl;

        newMecApp.mecAppAddress = appInfo->endPoint.addr;
        newMecApp.mecAppPort = appInfo->endPoint.port;
        newMecApp.mecAppInstanceId = appInfo->instanceId;
        newMecApp.contextId = contextIdCounter;
        meAppMap[contextIdCounter] = newMecApp;

        MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
        msg->setContextId(contextIdCounter);
        msg->setType(CREATE_CONTEXT_APP);
        msg->setRequestId(contAppMsg->getRequestId());
        msg->setSuccess(true);

         newMecApp.mecAppAddress = appInfo->endPoint.addr;
         newMecApp.mecAppPort = appInfo->endPoint.port;
         newMecApp.mecAppInstanceId = appInfo->instanceId;
         newMecApp.contextId = contextIdCounter;
         newMecApp.reference = appInfo->reference;
         meAppMap[contextIdCounter] = newMecApp;

        processingTime += instantiationTime;
        scheduleAt(simTime() + processingTime, msg);

        delete appInfo;
    }
    else {
        // throw cRuntimeError("MecOrchestrator::startMECApp - A suitable MEC host has not been selected");
        EV << "MecOrchestrator::startMECApp - A suitable MEC host has not been selected" << endl;
        MECOrchestratorMessage *msg = new MECOrchestratorMessage("MECOrchestratorMessage");
        msg->setType(CREATE_CONTEXT_APP);
        msg->setRequestId(contAppMsg->getRequestId());
        msg->setSuccess(false);
        processingTime += instantiationTime / 2;
        scheduleAt(simTime() + processingTime, msg);
    }
}

void MecOrchestrator::stopMECApp(UALCMPMessage *msg) {
    EV << "MecOrchestrator::stopMECApp - processing..." << endl;

    DeleteContextAppMessage *contAppMsg = check_and_cast<DeleteContextAppMessage *>(msg);

    int contextId = contAppMsg->getContextId();
    EV << "MecOrchestrator::stopMECApp - processing contextId: " << contextId << endl;
    // Checking if ueAppIdToMeAppMapKey entry map does exist
    if (meAppMap.empty() || (meAppMap.find(contextId) == meAppMap.end())) {
        // Maybe it has already been deleted
        EV << "MecOrchestrator::stopMECApp - \tWARNING MEC Application [" << meAppMap[contextId].mecUeAppID << "] not found!" << endl;
        sendDeleteAppContextAck(false, contAppMsg->getRequestId(), contextId);
        return;
    }

    // Call the methods of resource manager and virtualization infrastructure of the selected MEC host to deallocate the resources

    MecPlatformManager *mecpm = check_and_cast<MecPlatformManager *>(meAppMap[contextId].mecpm);
    DeleteAppMessage *deleteAppMsg = new DeleteAppMessage();
    deleteAppMsg->setUeAppID(meAppMap[contextId].mecUeAppID);

    bool isTerminated;
    if (meAppMap[contextId].isEmulated) {
        isTerminated = mecpm->terminateEmulatedMEApp(deleteAppMsg);
        std::cout << "terminateEmulatedMEApp with result: " << isTerminated << std::endl;
    }
    else {
        isTerminated = mecpm->terminateMEApp(deleteAppMsg);
    }

    MECOrchestratorMessage *mecoMsg = new MECOrchestratorMessage("MECOrchestratorMessage");
    mecoMsg->setType(DELETE_CONTEXT_APP);
    mecoMsg->setRequestId(contAppMsg->getRequestId());
    mecoMsg->setContextId(contAppMsg->getContextId());
    if (isTerminated) {
        EV << "MecOrchestrator::stopMECApp - MEC Application [" << meAppMap[contextId].mecUeAppID << "] removed" << endl;
        meAppMap.erase(contextId);
        mecoMsg->setSuccess(true);
    }
    else {
        EV << "MecOrchestrator::stopMECApp - MEC Application [" << meAppMap[contextId].mecUeAppID << "] not removed" << endl;
        mecoMsg->setSuccess(false);
    }

    double processingTime = terminationTime;
    scheduleAt(simTime() + processingTime, mecoMsg);
}

void MecOrchestrator::sendDeleteAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    EV << "MecOrchestrator::sendDeleteAppContextAck - result: " << result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    DeleteContextAppAckMessage *ack = new DeleteContextAppAckMessage();
    ack->setType(ACK_DELETE_CONTEXT_APP);
    ack->setRequestId(requestSno);
    ack->setSuccess(result);

    send(ack, "toUALCMP");
}

void MecOrchestrator::sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId)
{
    EV << "MecOrchestrator::sendCreateAppContextAck - result: " << result << " reqSno: " << requestSno << " contextId: " << contextId << endl;
    CreateContextAppAckMessage *ack = new CreateContextAppAckMessage();
    ack->setType(ACK_CREATE_CONTEXT_APP);

    if (result) {
        if (meAppMap.empty() || meAppMap.find(contextId) == meAppMap.end()) {
            EV << "MecOrchestrator::ackMEAppPacket - ERROR meApp[" << contextId << "] does not exist!" << endl;
            return;
        }

        mecAppMapEntry mecAppStatus = meAppMap[contextId];

        ack->setSuccess(true);
        ack->setContextId(contextId);
        ack->setAppInstanceId(mecAppStatus.mecAppInstanceId.c_str());
        ack->setRequestId(requestSno);
        std::stringstream uri;

        uri << mecAppStatus.mecAppAddress.str() << ":" << mecAppStatus.mecAppPort;

        ack->setAppInstanceUri(uri.str().c_str());
    }
    else {
        ack->setRequestId(requestSno);
        ack->setSuccess(false);
    }
    send(ack, "toUALCMP");
}

cModule *MecOrchestrator::findBestMecHost(const ApplicationDescriptor& appDesc)
{
    EV << "MecOrchestrator::findBestMecHost - finding best MEC host..." << endl;
    cModule *bestHost = nullptr;

    for (auto mecHost : mecHosts) {
        VirtualisationInfrastructureManager *vim = check_and_cast<VirtualisationInfrastructureManager *>(mecHost->getSubmodule("vim"));
        ResourceDescriptor resources = appDesc.getVirtualResources();
        bool res = vim->isAllocable(resources.ram, resources.disk, resources.cpu);
        if (!res) {
            EV << "MecOrchestrator::findBestMecHost - MEC host [" << mecHost->getName() << "] has not got enough resources. Searching again..." << endl;
            continue;
        }

        // Temporarily select this MEC host as the best
        EV << "MecOrchestrator::findBestMecHost - MEC host [" << mecHost->getName() << "] temporarily chosen as best MEC host, checking for the required MEC services.." << endl;
        bestHost = mecHost;

        MecPlatformManager *mecpm = check_and_cast<MecPlatformManager *>(mecHost->getSubmodule("mecPlatformManager"));
        auto mecServices = mecpm->getAvailableMecServices();
        std::string serviceName;

        // I assume the app requires only one MEC service
        if (appDesc.getAppServicesRequired().size() > 0) {
            serviceName = appDesc.getAppServicesRequired()[0];
        }
        else {
            break;
        }
        for (const auto& service : *mecServices) {
            if (serviceName == service.getName()) {
                bestHost = mecHost;
                break;
            }
        }
    }
    if (bestHost != nullptr)
        EV << "MecOrchestrator::findBestMecHost - best MEC host: " << bestHost->getName() << endl;
    else
        EV << "MecOrchestrator::findBestMecHost - no MEC host found" << endl;

    return bestHost;
}

void MecOrchestrator::getConnectedMecHosts()
{
    EV << "MecOrchestrator::getConnectedMecHosts - mecHostList: " << par("mecHostList").str() << endl;

    // Getting the list of MEC hosts associated with this MEC system from parameter
    auto mecHostList = check_and_cast<cValueArray *>(par("mecHostList").objectValue());
    if (mecHostList->size() > 0) {
        for (int i = 0; i < mecHostList->size(); i++) {
            const char *token = mecHostList->get(i).stringValue();
            EV << "MecOrchestrator::getConnectedMecHosts - mec host (from par): " << token << endl;
            cModule *mecHostModule = getSimulation()->getModuleByPath(token);
            mecHosts.push_back(mecHostModule);
        }
    }
    else {
        EV << "MecOrchestrator::getConnectedMecHosts - No mecHostList found" << endl;
    }
}

const ApplicationDescriptor& MecOrchestrator::onboardApplicationPackage(const char *fileName)
{
    EV << "MecOrchestrator::onBoardApplicationPackages - onboarding application package (from request): " << fileName << endl;
    ApplicationDescriptor appDesc(fileName);
    if (mecApplicationDescriptors_.find(appDesc.getAppDId()) != mecApplicationDescriptors_.end()) {
        EV << "MecOrchestrator::onboardApplicationPackages() - Application descriptor with appName [" << fileName << "] is already present.\n" << endl;
    }
    else {
        mecApplicationDescriptors_[appDesc.getAppDId()] = appDesc; // add to the mecApplicationDescriptors_
    }

    return mecApplicationDescriptors_[appDesc.getAppDId()];
}

void MecOrchestrator::registerMecService(ServiceDescriptor& serviceDescriptor) const
{
    EV << "MecOrchestrator::registerMecService - Registering MEC service [" << serviceDescriptor.name << "]" << endl;
    for (auto mecHost : mecHosts) {
        cModule *module = mecHost->getSubmodule("mecPlatform")->getSubmodule("serviceRegistry");
        if (module != nullptr) {
            EV << "MecOrchestrator::registerMecService - Registering MEC service [" << serviceDescriptor.name << "] in MEC host [" << mecHost->getName() << "]" << endl;
            ServiceRegistry *serviceRegistry = check_and_cast<ServiceRegistry *>(module);
            serviceRegistry->registerMecService(serviceDescriptor);
        }
    }
}

void MecOrchestrator::onboardApplicationPackages()
{
    // Getting the list of MEC hosts associated with this MEC system from parameter
    auto mecApplicationPackageList = check_and_cast<cValueArray *>(par("mecApplicationPackageList").objectValue());
    if (mecApplicationPackageList->size() > 0) {
        for (int i = 0; i < mecApplicationPackageList->size(); i++) {
            const char *token = mecApplicationPackageList->get(i).stringValue();
            std::string buf = std::string("ApplicationDescriptors/") + token + ".json";
            onboardApplicationPackage(buf.c_str());
        }
    }
    else {
        EV << "MecOrchestrator::onboardApplicationPackages - No mecApplicationPackageList found" << endl;
    }
}

const ApplicationDescriptor *MecOrchestrator::getApplicationDescriptorByAppName(const std::string& appName) const
{
    for (const auto& appDesc : mecApplicationDescriptors_) {
        if (appDesc.second.getAppName() == appName)
            return &(appDesc.second);
    }

    return nullptr;
}

} //namespace


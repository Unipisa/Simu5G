/*
 * bgMecAppManager.cc
 *
 *  Created on: Jul 15, 2022
 *      Author: linofex
 */


#include "BgMecAppManager.h"
#include "nodes/mec/bgMecAppManager/timer/BgTimer_m.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"

using namespace omnetpp;


Define_Module(BgMecAppManager);


BgMecAppManager::~BgMecAppManager()
{
    for (auto& bgMecApp: bgMecApps_)
    {
        cancelAndDelete(bgMecApp.second.timer);
        // the modules are deleted by the omnet platform? I hope
    }

    bgMecApps_.clear();

    cancelAndDelete(deltaMsg_);
}

BgMecAppManager::BgMecAppManager()
{
    deltaMsg_ = nullptr;
    snapMsg_ = nullptr;
}


void BgMecAppManager::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    bgAppsVector_.setName("MecBgApps");

    fromTraceFile_ = par("fromTraceFile").boolValue();
    lastMecHostActiveted_ = -1;
    mecHostActivationTime_ = par("mecHostActivation");
    maxBgMecApp_ = par("maxBgMecApps");
    admissionControl_ = par("admissionControl");
    currentBgMecApps_ = 0;
    readMecHosts();

    defaultRam_ = par("defaultRam");
    defaultDisk_ = par("defaultDisk");
    defaultCpu_ = par("defaultCpu");
    if(!fromTraceFile_)
    {
        deltaTime_ = par("deltaTime").doubleValue();
        deltaMsg_ = new cMessage("deltaMsg");
        mode_ = CREATE;
        scheduleAfter(deltaTime_, deltaMsg_);
    }
    else
    {
      // read the tracefile and create structure with snapshots
        std::string fileName = par("traceFileName").stringValue();

        simtime_t timeLine = simTime();
        snapshotPeriod_ = par("snapshotPeriod").doubleValue();
        bool realTimeStamp = snapshotPeriod_ > 0? false: true;
        std::ifstream inputFileStream;
        inputFileStream.open(fileName);
        if (!inputFileStream)
            throw cRuntimeError("Error opening data file '%s'", fileName.c_str());

        while(true)
        {
            Snapshot newsnapShot;
            // read line by line
            double time;
            int num;
            if(realTimeStamp) //read two numbers
            {
                inputFileStream >> time; // newsnapShot.snapShotTime;
            }
            else
            {
                timeLine += snapshotPeriod_;
                time = timeLine.dbl();
            }
            inputFileStream >> num;  // newsnapShot.numMecApps;
            if(!inputFileStream.eof())
            {
                newsnapShot.snapShotTime = time;
                newsnapShot.numMecApps = num;
                snapshotList_.push_back(newsnapShot);
            }
            else
            {
                break;
            }
        }

        std::cout << "****" << std::endl;
//  debug
        for(auto& snap : snapshotList_)
        {
            std::cout << "time " << snap.snapShotTime << " apps " << snap.numMecApps << std::endl;
        }
        std::cout << "###" << std::endl;

        // schedule first snapshot
        snapMsg_ = new cMessage("snapshotMsg");
        scheduleNextSnapshot();
    }
}


void BgMecAppManager::scheduleNextSnapshot()
{
    if(!snapshotList_.empty())
    {
        double timer = snapshotList_.front().snapShotTime;
        EV << "BgMecAppManager::handleMessage - next snapshot is scheduled at time: " << timer << endl;
        // TODO create structure for quantum and smoother line
        scheduleAt(timer, snapMsg_);
    }
    else
    {
        EV << "BgMecAppManager::scheduleNextSnapshot()- no more snapshot available" << endl;
    }
}

bool BgMecAppManager::createBgModules()
 {
     int id = currentBgMecApps_;
     ResourceDescriptor resource = {defaultRam_, defaultDisk_, defaultCpu_};
     BgMecAppDescriptor appDescriptor;
     appDescriptor.centerX = par("centerX");
     appDescriptor.centerY = par("centerY");
     appDescriptor.radius = par("radius");
     appDescriptor.mecHost = chooseMecHost();
     appDescriptor.resources = resource;
     appDescriptor.timer = nullptr;

     bgMecApps_[id] = appDescriptor;

     cModule* bgAppModule = createBgMecApp(id);

     if(bgAppModule != nullptr) // resource available
     {
         cModule* bgUeModule = createBgUE(id);
         auto desc = bgMecApps_.find(id);
         desc->second.bgMecApp = bgAppModule;
         desc->second.bgUe = bgUeModule;
         EV << "BgMecAppManager::handleMessage - bg environment with id [" << id << "] started" << endl;
         currentBgMecApps_++;
         return true;
     }
     else
     {
         EV << "BgMecAppManager::handleMessage - bg environment with id [" << id << "] NOT started" << endl;
         bgMecApps_.erase(id);
         return false;
     }

 }

 void BgMecAppManager::deleteBgModules()
 {
     int id = --currentBgMecApps_;
     deleteBgMecApp(id);
     deleteBgUe(id);
     if(isMecHostEmpty(bgMecApps_[id].mecHost))
         deactivateNewMecHost(bgMecApps_[id].mecHost);
     //check if the MecHost is empty and in case deactivate it
     EV << "BgMecAppManager::handleMessage - bg environment with id [" << id << "] stopped" << endl;
 }

 void BgMecAppManager::handleMessage(cMessage* msg)
{
    if(msg->isSelfMessage())
    {
        if(msg->isName("snapshotMsg"))
        {
            // read running bgApp
            int numApps = snapshotList_.front().numMecApps;
            EV << "BgMecAppManager::handleMessage (snapshotMsg) - current number of BG Mec Apps " << currentBgMecApps_ << ", expected " << numApps << endl;
            while(currentBgMecApps_ != numApps)
            {
                if(currentBgMecApps_ < numApps)
                {
                    if(!createBgModules())
                        break;

                }
                else
                {
                    deleteBgModules();
                }
                if(currentBgMecApps_ == maxBgMecApp_)
                {
                    EV << "BgMecAppManager::handleMessage (snapshotMsg) - Scheduled activation of a new Mec host in " << mecHostActivationTime_ << " seconds" << endl;
                    cMessage* activateMecHostMsg = new cMessage("mecHostActivation");
                    scheduleAfter(mecHostActivationTime_, activateMecHostMsg);
                }
            }

            //schedule next snapshot
            snapshotList_.pop_front();
            bgAppsVector_.record(currentBgMecApps_);
            scheduleNextSnapshot();
        }
        else if(msg->isName("deltaMsg"))
        {
            if(mode_ == CREATE)
            {
                bool res = createBgModules();
                EV << "BgMecAppManager::handleMessage (deltaMsg) - currentBgMecApps: " << currentBgMecApps_ << endl;
                if(currentBgMecApps_ == maxBgMecApp_/2)
                {
                    EV << "BgMecAppManager::handleMessage (deltaMsg) - Scheduled activation of a new Mec host in " << mecHostActivationTime_ << " seconds" << endl;
                    cMessage* activateMecHostMsg = new cMessage("mecHostActivation");
                    scheduleAfter(mecHostActivationTime_, activateMecHostMsg);
                }

                if(currentBgMecApps_ == maxBgMecApp_ || res == false)
                         mode_ = DELETE;
            }
            else
            {
                deleteBgModules();
                if(currentBgMecApps_ == 0)
                    mode_ = CREATE;

            }
            scheduleAfter(deltaTime_, deltaMsg_);
            bgAppsVector_.record(currentBgMecApps_);
        }
        else if(msg->isName("mecHostActivation"))
        {
            activateNewMecHost();
            delete msg;
        }
    }

}


cModule* BgMecAppManager::createBgMecApp(int id)
{
    VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(bgMecApps_[id].mecHost->getSubmodule("vim"));

    //create bgMecApp module and get the id used for register the mec app to the VIM
    cModuleType *moduleType = cModuleType::get("simu5g.nodes.mec.bgMecAppManager.bgModules.BgMecApp");         //MEAPP module package (i.e. path!)
    std::string moduleName("bgMecApp_");
    std::stringstream appName;
    appName << moduleName << id;

//    cModule *module = moduleType->create(appName.str().c_str(), bgMecApps_[id].mecHost);       //MEAPP module-name & its Parent Module
    //or
    cModule *module = moduleType->createScheduleInit(appName.str().c_str(), bgMecApps_[id].mecHost);       //MEAPP module-name & its Parent Module


    module->setName(appName.str().c_str());

    int moduleId = module->getId();

    bool success = vim->registerMecApp(moduleId, bgMecApps_[id].resources.ram, bgMecApps_[id].resources.disk, bgMecApps_[id].resources.cpu, admissionControl_);

    if(success)
    {
        EV << "BgMecAppManager::createBgMecApp: bgMecApp created " << appName.str() << endl;
        return module;
    }
    else{
        EV << "BgMecAppManager::createBgMecApp: bgMecApp NOT created " << appName.str() << endl;
        module->deleteModule();
        return nullptr;
    }
}

void BgMecAppManager::deleteBgMecApp(int id)
{
    VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(bgMecApps_[id].mecHost->getSubmodule("vim"));

    bool success = vim->deRegisterMecApp(bgMecApps_[id].bgMecApp->getId());
    if(success)
    {
        if(bgMecApps_[id].bgMecApp != nullptr)
        {
            bgMecApps_[id].bgMecApp->deleteModule();
            EV << "BgMecAppManager::deleteBgMecApp: bgMecApp with id [" << id << "] deleted " << endl;
        }
    }
}

cModule* BgMecAppManager::createBgUE(int id)
{
    cModuleType *moduleType = cModuleType::get("simu5g.nodes.mec.bgMecAppManager.bgModules.BgUe");         //MEAPP module package (i.e. path!)
    std::string moduleName("bgUe_");
    std::stringstream appName;
    appName << moduleName << id;

    cModule *module = moduleType->create(appName.str().c_str(), getParentModule());       //MEAPP module-name & its Parent Module
    module->setName(appName.str().c_str());

    double x,y;
    x = bgMecApps_[id].centerX + uniform(bgMecApps_[id].radius/2, bgMecApps_[id].radius)*(intuniform(0, 1)*2 -1);
    y = bgMecApps_[id].centerY + uniform(bgMecApps_[id].radius/2, bgMecApps_[id].radius)*(intuniform(0, 1)*2 -1);
    module->finalizeParameters();
    module->buildInside();

//    module->getSubmodule("mobility")->par("initialX") = x;
//    module->getSubmodule("mobility")->par("initialY") = y;
    module->callInitialize();

    std::stringstream display;
    display << "i=device/pocketpc;p=" << x << "," << y;
    module->setDisplayString(display.str().c_str());
//    module->getSubmodule("mobility")->setDisplayString(display.str().c_str());


    if(module != nullptr)
    {
        EV << "BgMecAppManager::createBgUE: BgUe created " << appName.str() << endl;
        return module;
    }
    else{
        EV << "BgMecAppManager::createBgUE: BgUe NOT created " << appName.str() << endl;
        module->deleteModule();
        return nullptr;
    }

}

void BgMecAppManager::deleteBgUe(int id)
{
    if(bgMecApps_[id].bgUe != nullptr)
    {
        bgMecApps_[id].bgUe->deleteModule();
        EV << "BgMecAppManager::deleteBgUe: BGUe with id [" << id << "] deleted " << endl;
    }
}

void BgMecAppManager::readMecHosts()
{
    EV <<"BgMecAppManager::readMecHosts" << endl;
    //getting the list of mec hosts associated to this mec system from parameter
    if(hasPar("mecHostList") && strcmp(par("mecHostList").stringValue(), "")){
        std::string mecHostList = par("mecHostList").stdstringValue();
        EV <<"BgMecAppManager::readMecHosts list " << (char*)par("mecHostList").stringValue() << endl;
        char* token = strtok ((char*)mecHostList.c_str() , ", ");            // split by commas
        while (token != NULL)
        {
            EV <<"BgMecAppManager::readMecHosts list " << token << endl;
            cModule *mhModule = getSimulation()->getModuleByPath(token);
            mhModule->getDisplayString().setTagArg("i",1, "red");
            mecHosts_.push_back(mhModule);
            token = strtok (NULL, ", ");
        }
    }

    if(mecHosts_.size() > 1)
        activateNewMecHost(); //activate the firstMecHostAdded
    return;
}


cModule* BgMecAppManager::chooseMecHost()
{
    return runningMecHosts_.back();
}

// the policy used to activate and deactivate does not require the  runningMecHosts_ var
// just the lastMecHostActiveted_ is necessary, but more complex policies will be implemented

void BgMecAppManager::activateNewMecHost()
{
    if(lastMecHostActiveted_ == mecHosts_.size() -1)
    {
        EV << "BgMecAppManager::activateNewMecHost() - no more mecHosts are available" << endl;
    }
    else
    {
        EV << "BgMecAppManager::activateNewMecHost() - turning on Mec host with index " << lastMecHostActiveted_+1  << endl;
        cModule* mh = mecHosts_[++lastMecHostActiveted_];
        mh->getDisplayString().setTagArg("i",1, "green");
        runningMecHosts_.push_back(mh);
    }
}

void BgMecAppManager::deactivateNewMecHost(cModule* mecHost)
{
    if(runningMecHosts_.size() == 1)
    {
        EV << "BgMecAppManager::deactivateNewMecHost: at least one MEC host must be present" << endl;
        return;
    }
    auto it = runningMecHosts_.begin();
    for(; it != runningMecHosts_.end(); ++it)
    {
        if(*it == mecHost)
        {
            EV << "BgMecAppManager::deactivateNewMecHost() - mec Host deactivated" << endl;
            (*it)->getDisplayString().setTagArg("i",1, "red");
            runningMecHosts_.erase(it);
            lastMecHostActiveted_--;
            return;
        }
    }
    EV << "BgMecAppManager::deactivateNewMecHost() - mec Host not found in running mecHostList" << endl;
}

bool BgMecAppManager::isMecHostEmpty(cModule* mecHost)
{
    for(auto& mh : runningMecHosts_)
    {
        if(mh == mecHost)
        {
            VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(mecHost->getSubmodule("vim"));
            return (vim->getCurrentMecApps() == 0);
        }
    }
    EV << "BgMecAppManager::isEmpty - the mecHost is not running" << endl;
    throw cRuntimeError("BgMecAppManager::isEmpty - the mecHost [%s] is not running", mecHost->getFullName());
}

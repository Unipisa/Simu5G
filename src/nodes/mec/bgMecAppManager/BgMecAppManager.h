/*
 * bgAppManager.h
 *
 *  Created on: Jul 15, 2022
 *      Author: linofex
 */

#ifndef NODES_MEC_BGMECAPPMANAGER_BGMECAPPMANAGER_H_
#define NODES_MEC_BGMECAPPMANAGER_BGMECAPPMANAGER_H_

#include "omnetpp.h"
#include <map>
#include <list>

#include "nodes/mec/utils/MecCommon.h"

typedef enum
{
    START_TIMER = 0,
    STOP_TIMER = 1
} TimerState;
typedef enum
{
    CREATE = 0,
    DELETE = 1
} Mode;


typedef struct
{
    int id;
    omnetpp::cModule* mecHost;
    omnetpp::cModule* bgMecApp;
    omnetpp::cModule* bgUe;
    double startTime;
    double stopTime;
    double centerX;
    double centerY;
    double radius;
    ResourceDescriptor resources;
    omnetpp::cMessage* timer;
} BgMecAppDescriptor;

typedef struct
{
    double snapShotTime; // the real snapshot if present in file, or the next calulated snapshot
    int numMecApps;
} Snapshot;

class BgMecAppManager : public omnetpp::cSimpleModule {

    private:
        std::map<int, BgMecAppDescriptor> bgMecApps_;
        int lastMecHostActiveted_; // maintans the index of mecHosts_ vector of the last mec host
        std::vector<cModule*> mecHosts_;
        std::vector<cModule*> runningMecHosts_;

        omnetpp::cOutVector bgAppsVector_;

        bool fromTraceFile_;
        std::list<Snapshot> snapshotList_;
        double snapshotPeriod_;
        omnetpp::cMessage* snapMsg_;

        bool admissionControl_;

        double defaultRam_;
        double defaultDisk_;
        double defaultCpu_;

        omnetpp::cMessage* deltaMsg_;
        double deltaTime_;
        double mecHostActivationTime_;
        int maxBgMecApp_;
        int currentBgMecApps_;
        Mode mode_;


    public:
        BgMecAppManager();
        ~BgMecAppManager();

        int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(omnetpp::cMessage *msg);

        void scheduleNextSnapshot();

        // this method calls createBgMecApp and createBgUe
        bool createBgModules();
        cModule * createBgMecApp(int id);
        cModule * createBgUE(int id);

        // this method calls deleteBgMecApp and deleteBgUe
        void deleteBgModules();
        void deleteBgMecApp(int id);
        void deleteBgUe(int id);

        void readMecHosts();
        // dummy method. It chooses the last started mecHost ALWYAS supposing it has enough resources
        cModule* chooseMecHost();
        void activateNewMecHost();
        void deactivateNewMecHost(cModule* module);
        bool isMecHostEmpty(cModule* module);



};


#endif /* NODES_MEC_BGMECAPPMANAGER_BGMECAPPMANAGER_H_ */

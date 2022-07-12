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


#ifndef __MECPLATFORMMANAGER_H_
#define __MECPLATFORMMANAGER_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.h"

using namespace omnetpp;

//
// simple module implementing the MEC platform manager (MECPM) entity of a
// MEC system. It does not follow the ETSI specs, but acts only as a
// passtrhough between the MEC orchestrator and the MEC host modules
//
// The mecOrchestrator module is used to link the MECPM with the MEC orchestrator

class ServiceRegistry;
class MecOrchestrator;

class MecPlatformManager : public cSimpleModule
{
    protected:

        MecOrchestrator* mecOrchestrator;
        VirtualisationInfrastructureManager* vim;
        ServiceRegistry* serviceRegistry;
    public:
        MecPlatformManager();

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage*){}
        virtual void finish(){}

        /* instancing the requested MECApp
         *
         * The argument is a message even if it is called as a direct method call from the
         * MEC orchestrator. It could be useful in the future if the MECPM were enriched
         * with gates and deeper functionalities.
         *
         * For the instantiateMEApp method:
         * @return MecAppInstanceInfo structure with the endpoint of the MEC app.
         *
         * For the instantiateEmulatedMEApp method only a bool is returned, since the endpoint
         * is known at the MEC orchestrator (in the appDescriptor)
         */
        // instantiating the MEC app
        MecAppInstanceInfo* instantiateMEApp(CreateAppMessage*);
        bool instantiateEmulatedMEApp(CreateAppMessage*);
        // terminating the correspondent MEC app
        bool terminateMEApp(DeleteAppMessage*);
        bool terminateEmulatedMEApp(DeleteAppMessage*);

        const std::vector<ServiceInfo>* getAvailableMecServices() const;

        /*
         * method called by the MEC service to notify its presence to the MEC system
         */

        void registerMecService(ServiceDescriptor&) const;
};

#endif

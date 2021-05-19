//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


#ifndef __MECPLATFORMMANAGER_H_
#define __MECPLATFORMMANAGER_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "nodes/mec/MEPlatform/ServiceRegistry/ServiceRegistry.h"

using namespace omnetpp;

/**
 * VirtualisationInfrastructureManager
 *
 *  The task of this class is:
 */

class ServiceRegistry;

class MecPlatformManager : public cSimpleModule
{
    protected:
        VirtualisationInfrastructureManager* vim;
        ServiceRegistry* serviceRegistry;
    public:
        MecPlatformManager();

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage*){}
        virtual void finish(){}

    // instancing the requested MEApp (called by handleResource)
    MecAppInstanceInfo instantiateMEApp(CreateAppMessage*);
    bool instantiateEmulatedMEApp(CreateAppMessage*);

    // terminating the correspondent MEApp (called by handleResource)
    bool terminateMEApp(DeleteAppMessage*);
    bool terminateEmulatedMEApp(DeleteAppMessage*);

    const MecServicesMap* getAvailableServices();

};

#endif

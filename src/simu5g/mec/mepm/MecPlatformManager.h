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

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/mec/utils/MecCommon.h"
#include "simu5g/mec/vim/VirtualisationInfrastructureManager.h"
#include "simu5g/mec/platform/ServiceRegistry/ServiceRegistry.h"

namespace simu5g {

using namespace omnetpp;

//
// simple module implementing the MEC platform manager (MECPM) entity of a
// MEC system. It does not follow the ETSI specs, but acts only as a
// passthrough between the MEC orchestrator and the MEC host modules
//
// The mecOrchestrator module is used to link the MECPM with the MEC orchestrator

class ServiceRegistry;
class MecOrchestrator;

class MecPlatformManager : public cSimpleModule
{
  protected:
    inet::ModuleRefByPar<MecOrchestrator> mecOrchestrator;
    inet::ModuleRefByPar<VirtualisationInfrastructureManager> vim;
    inet::ModuleRefByPar<ServiceRegistry> serviceRegistry;

  public:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override {}
    void finish() override {}

    /* instantiating the requested MecApp
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
    MecAppInstanceInfo *instantiateMEApp(CreateAppMessage *msg);
    bool instantiateEmulatedMEApp(CreateAppMessage *msg);
    // terminating the corresponding MEC app
    bool terminateMEApp(DeleteAppMessage *msg);
    bool terminateEmulatedMEApp(DeleteAppMessage *msg);

    const std::vector<ServiceInfo> *getAvailableMecServices() const;

    /*
     * method called by the MEC service to notify its presence to the MEC system
     */

    void registerMecService(ServiceDescriptor&) const;
};

} //namespace

#endif


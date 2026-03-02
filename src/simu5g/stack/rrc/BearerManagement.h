//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _BEARER_MANAGEMENT_H_
#define _BEARER_MANAGEMENT_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteControlInfo.h"
#include <inet/common/ModuleRefByPar.h>

using namespace omnetpp;

namespace simu5g {

class LteMacBase;
class PdcpEntityManager;
class RlcEntityManager;
class Registration;

/**
 * @brief RRC Bearer Management — creates and tears down PDCP, RLC and MAC
 *        entities for data radio bearers.
 */
class BearerManagement : public cSimpleModule
{
  private:
    Registration *registration_ = nullptr;

    inet::ModuleRefByPar<PdcpEntityManager> pdcpModule;
    inet::ModuleRefByPar<RlcEntityManager> rlcUmModule;  // Compound module with TM/UM/AM submodules
    inet::ModuleRefByPar<RlcEntityManager> nrRlcUmModule;  // same
    inet::ModuleRefByPar<LteMacBase> macModule;
    inet::ModuleRefByPar<LteMacBase> nrMacModule;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:
    virtual void createIncomingConnection(FlowControlInfo *lteInfo, bool withPdcp=true);
    virtual void createOutgoingConnection(FlowControlInfo *lteInfo, bool withPdcp=true);
};

} // namespace simu5g

#endif

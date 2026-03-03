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
class RlcUpperMux;
class RlcLowerMux;
class RlcTxEntityBase;
class RlcRxEntityBase;
class UmTxEntity;
class UpperMux;
class LowerMux;
class DcMux;
class PdcpTxEntityBase;
class PdcpRxEntityBase;
class Registration;

/**
 * @brief RRC Bearer Management — creates and tears down PDCP, RLC and MAC
 *        entities for data radio bearers.
 */
class BearerManagement : public cSimpleModule
{
  private:
    Registration *registration_ = nullptr;

    // PDCP entity types (resolved from NED params)
    cModuleType *pdcpRxEntityModuleType_ = nullptr;
    cModuleType *pdcpTxEntityModuleType_ = nullptr;
    cModuleType *pdcpBypassRxEntityModuleType_ = nullptr;
    cModuleType *pdcpBypassTxEntityModuleType_ = nullptr;
    cModule *pdcpCompound_ = nullptr;  // NIC module (parent of PDCP submodules and entities)

    // RLC entity types (resolved from NED params)
    cModuleType *rlcUmTxEntityModuleType_ = nullptr;
    cModuleType *rlcUmRxEntityModuleType_ = nullptr;
    cModuleType *rlcTmTxEntityModuleType_ = nullptr;
    cModuleType *rlcTmRxEntityModuleType_ = nullptr;
    cModuleType *rlcAmTxEntityModuleType_ = nullptr;
    cModuleType *rlcAmRxEntityModuleType_ = nullptr;

    inet::ModuleRefByPar<PdcpEntityManager> pdcpModule;
    inet::ModuleRefByPar<RlcEntityManager> rlcUmModule;  // Compound module with TM/UM/AM submodules
    inet::ModuleRefByPar<RlcEntityManager> nrRlcUmModule;  // same
    inet::ModuleRefByPar<LteMacBase> macModule;
    inet::ModuleRefByPar<LteMacBase> nrMacModule;

    // Entity registries (CP owns the lifecycle of all entities)
    std::map<DrbKey, PdcpTxEntityBase *> pdcpTxEntities_;
    std::map<DrbKey, PdcpRxEntityBase *> pdcpRxEntities_;
    std::map<DrbKey, PdcpTxEntityBase *> pdcpBypassTxEntities_;
    std::map<DrbKey, PdcpRxEntityBase *> pdcpBypassRxEntities_;
    std::map<DrbKey, RlcTxEntityBase *> rlcTxEntities_;
    std::map<DrbKey, RlcRxEntityBase *> rlcRxEntities_;
    std::map<DrbKey, RlcTxEntityBase *> nrRlcTxEntities_;
    std::map<DrbKey, RlcRxEntityBase *> nrRlcRxEntities_;

    void setEntityParamsFromRlcMgr(cModule *entity, RlcEntityManager *rlcMgr);
    RlcTxEntityBase *createAndInstallRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcEntityManager *rlcMgr);
    RlcRxEntityBase *createAndInstallRlcRxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcEntityManager *rlcMgr);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

  public:
    virtual void createIncomingConnection(FlowControlInfo *lteInfo, bool withPdcp=true);
    virtual void createOutgoingConnection(FlowControlInfo *lteInfo, bool withPdcp=true);
    virtual RlcTxEntityBase *createRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo);
    virtual RlcRxEntityBase *createRlcRxBuffer(DrbKey id, FlowControlInfo *lteInfo);
    virtual RlcTxEntityBase *lookupRlcTxBuffer(DrbKey id);
    PdcpTxEntityBase *lookupPdcpTxEntity(DrbKey id);
    PdcpRxEntityBase *lookupPdcpRxEntity(DrbKey id);
    virtual void deleteLocalPdcpEntities(MacNodeId nodeId);
    virtual void deleteLocalRlcQueues(MacNodeId nodeId, bool nrStack=false);
    void pdcpActiveUeUL(std::set<MacNodeId> *ueSet);
};

} // namespace simu5g

#endif

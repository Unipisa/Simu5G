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

#include "simu5g/stack/rrc/BearerManagement.h"
#include "simu5g/stack/rrc/Registration.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/RlcEntityManager.h"
#include "simu5g/stack/rlc/RlcUpperMux.h"
#include "simu5g/stack/rlc/RlcLowerMux.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"
#include "simu5g/stack/pdcp/PdcpEntityManager.h"
#include "simu5g/stack/pdcp/UpperMux.h"
#include "simu5g/stack/pdcp/LowerMux.h"
#include "simu5g/stack/pdcp/DcMux.h"
#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

using namespace inet;

Define_Module(BearerManagement);

void BearerManagement::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        registration_ = check_and_cast<Registration *>(getParentModule()->getSubmodule("registration"));

        // Resolve PDCP entity types
        pdcpTxEntityModuleType_ = cModuleType::get(par("pdcpTxEntityModuleType").stringValue());
        pdcpRxEntityModuleType_ = cModuleType::get(par("pdcpRxEntityModuleType").stringValue());
        pdcpBypassTxEntityModuleType_ = cModuleType::get(par("pdcpBypassTxEntityModuleType").stringValue());
        pdcpBypassRxEntityModuleType_ = cModuleType::get(par("pdcpBypassRxEntityModuleType").stringValue());

        // Resolve RLC entity types
        rlcUmTxEntityModuleType_ = cModuleType::get(par("rlcUmTxEntityModuleType").stringValue());
        rlcUmRxEntityModuleType_ = cModuleType::get(par("rlcUmRxEntityModuleType").stringValue());
        rlcTmTxEntityModuleType_ = cModuleType::get(par("rlcTmTxEntityModuleType").stringValue());
        rlcTmRxEntityModuleType_ = cModuleType::get(par("rlcTmRxEntityModuleType").stringValue());
        rlcAmTxEntityModuleType_ = cModuleType::get(par("rlcAmTxEntityModuleType").stringValue());
        rlcAmRxEntityModuleType_ = cModuleType::get(par("rlcAmRxEntityModuleType").stringValue());

        pdcpModule.reference(this, "pdcpModule", true);
        pdcpCompound_ = pdcpModule->getPdcpCompoundModule();

        rlcUmModule.reference(this, "rlcUmModule", true);
        nrRlcUmModule.reference(this, "nrRlcUmModule", false);
        macModule.reference(this, "macModule", true);
        nrMacModule.reference(this, "nrMacModule", false);
    }
}

void BearerManagement::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not process messages");
}

void BearerManagement::createIncomingConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    Enter_Method_Silent("createIncomingConnection()");

    EV << "BearerManagement::createIncomingConnection - " << " srcId=" << lteInfo->getSourceId() << " destId=" << lteInfo->getDestId()
        << " groupId=" << lteInfo->getMulticastGroupId() << " drbId=" << lteInfo->getDrbId()
        << " direction=" << dirToA(lteInfo->getDirection())
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(lteInfo->getDestId() == registration_->getLteNodeId() || lteInfo->getDestId() == registration_->getNrNodeId() || lteInfo->getMulticastGroupId() != NODEID_NONE);

    // Create MAC incoming connection
    FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
    MacNodeId senderId = desc.getSourceId();
    auto mac = (registration_->getNodeType()==UE && isNrUe(lteInfo->getDestId())) ? nrMacModule.get() : macModule.get(); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    LogicalCid lcid = mac->drbIdToLcid(desc.getDrbId());
    MacCid cid = MacCid(senderId, lcid);
    mac->createIncomingConnection(cid, desc);

    // RLC entity creation
    DrbKey rlcId = ctrlInfoToRxDrbKey(lteInfo);
    auto rlcUm = (registration_->getNodeType()==UE && isNrUe(lteInfo->getDestId())) ? nrRlcUmModule.get() : rlcUmModule.get(); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    createAndInstallRlcRxBuffer(rlcId, lteInfo, rlcUm);

    // PDCP entity creation
    auto *pdcpUpperMux = check_and_cast<UpperMux *>(pdcpCompound_->getSubmodule("pdcpUpperMux"));
    auto *pdcpLowerMux = check_and_cast<LowerMux *>(pdcpCompound_->getSubmodule("pdcpLowerMux"));
    auto *pdcpDcMux = check_and_cast<DcMux *>(pdcpCompound_->getSubmodule("pdcpDcMux"));

    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        std::string name = "pdcp-rx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpRxEntityModuleType_->create(name.c_str(), pdcpCompound_);
        module->par("headerCompressedSize") = par("headerCompressedSize");
        module->finalizeParameters();
        module->buildInside();

        // Wire LowerMux → entity in gate
        int idx = pdcpLowerMux->gateSize("toRxEntity");
        pdcpLowerMux->setGateSize("toRxEntity", idx + 1);
        pdcpLowerMux->gate("toRxEntity", idx)->connectTo(module->gate("in"));

        // Wire entity out gate → UpperMux fromRxEntity
        int fromIdx = pdcpUpperMux->gateSize("fromRxEntity");
        pdcpUpperMux->setGateSize("fromRxEntity", fromIdx + 1);
        module->gate("out")->connectTo(pdcpUpperMux->gate("fromRxEntity", fromIdx));

        // Wire DcMux → entity dcIn gate (for UL X2 dispatch)
        if (module->hasGate("dcIn")) {
            int dcIdx = pdcpDcMux->gateSize("toRxEntity");
            pdcpDcMux->setGateSize("toRxEntity", dcIdx + 1);
            pdcpDcMux->gate("toRxEntity", dcIdx)->connectTo(module->gate("dcIn"));
        }

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
        pdcpLowerMux->registerRxEntity(id, rxEnt);
        pdcpRxEntities_[id] = rxEnt;
    }
    else {
        // DC secondary node: create bypass RX entity (forwards UL to master via X2)
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        std::string name = "pdcp-bypass-rx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpBypassRxEntityModuleType_->create(name.c_str(), pdcpCompound_);
        module->finalizeParameters();
        module->buildInside();

        // Wire LowerMux → entity in gate (same dispatch as normal RX)
        int idx = pdcpLowerMux->gateSize("toRxEntity");
        pdcpLowerMux->setGateSize("toRxEntity", idx + 1);
        pdcpLowerMux->gate("toRxEntity", idx)->connectTo(module->gate("in"));

        // Wire entity out gate → DcMux (bypass RX sends to X2 via DcMux)
        int fromIdx = pdcpDcMux->gateSize("fromEntity");
        pdcpDcMux->setGateSize("fromEntity", fromIdx + 1);
        module->gate("out")->connectTo(pdcpDcMux->gate("fromEntity", fromIdx));

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
        pdcpLowerMux->registerRxEntity(id, rxEnt);
        pdcpBypassRxEntities_[id] = rxEnt;
    }
}

void BearerManagement::createOutgoingConnection(FlowControlInfo *lteInfo, bool withPdcp)
{
    Enter_Method_Silent("createOutgoingConnection()");

    EV << "BearerManagement::createOutgoingConnection - " << " srcId=" << lteInfo->getSourceId() << " destId=" << lteInfo->getDestId()
        << " groupId=" << lteInfo->getMulticastGroupId() << " drbId=" << lteInfo->getDrbId()
        << " direction=" << dirToA(lteInfo->getDirection())
        << " withPdcp=" << (withPdcp ? "yes" : "no") << endl;

    ASSERT(lteInfo->getSourceId() == registration_->getLteNodeId() || lteInfo->getSourceId() == registration_->getNrNodeId());

    // Create MAC outgoing connection
    FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
    MacNodeId destId = desc.getDestId();
    auto mac = (registration_->getNodeType()==UE && isNrUe(lteInfo->getSourceId())) ? nrMacModule.get() : macModule.get();
    LogicalCid lcid = mac->drbIdToLcid(desc.getDrbId());
    MacCid cid = MacCid(destId, lcid);
    mac->createOutgoingConnection(cid, desc);

    // RLC entity creation
    DrbKey rlcId = ctrlInfoToTxDrbKey(lteInfo);
    auto rlcUm = (registration_->getNodeType()==UE && isNrUe(lteInfo->getSourceId())) ? nrRlcUmModule.get() : rlcUmModule.get();
    createAndInstallRlcTxBuffer(rlcId, lteInfo, rlcUm);

    // PDCP entity creation
    auto *pdcpUpperMux = check_and_cast<UpperMux *>(pdcpCompound_->getSubmodule("pdcpUpperMux"));
    auto *pdcpLowerMux = check_and_cast<LowerMux *>(pdcpCompound_->getSubmodule("pdcpLowerMux"));
    auto *pdcpDcMux = check_and_cast<DcMux *>(pdcpCompound_->getSubmodule("pdcpDcMux"));

    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        std::string name = "pdcp-tx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpTxEntityModuleType_->create(name.c_str(), pdcpCompound_);
        module->par("headerCompressedSize") = par("headerCompressedSize");
        module->finalizeParameters();
        module->buildInside();

        // Wire UpperMux → entity in gate
        int idx = pdcpUpperMux->gateSize("toTxEntity");
        pdcpUpperMux->setGateSize("toTxEntity", idx + 1);
        pdcpUpperMux->gate("toTxEntity", idx)->connectTo(module->gate("in"));

        // Wire entity out gate → LowerMux fromTxEntity
        int fromIdx = pdcpLowerMux->gateSize("fromTxEntity");
        pdcpLowerMux->setGateSize("fromTxEntity", fromIdx + 1);
        module->gate("out")->connectTo(pdcpLowerMux->gate("fromTxEntity", fromIdx));

        // Wire dcOut gate → DcMux (if entity has one, e.g. NrTxPdcpEntity)
        if (module->hasGate("dcOut")) {
            int dcIdx = pdcpDcMux->gateSize("fromEntity");
            pdcpDcMux->setGateSize("fromEntity", dcIdx + 1);
            module->gate("dcOut")->connectTo(pdcpDcMux->gate("fromEntity", dcIdx));
        }

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
        pdcpUpperMux->registerTxEntity(id, txEnt);
        pdcpTxEntities_[id] = txEnt;
    }
    else {
        // DC secondary node: create bypass TX entity (forwards DL from master to RLC)
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        std::string name = "pdcp-bypass-tx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpBypassTxEntityModuleType_->create(name.c_str(), pdcpCompound_);
        module->finalizeParameters();
        module->buildInside();

        // Wire DcMux → entity in gate (DcMux dispatches incoming DL X2)
        int idx = pdcpDcMux->gateSize("toBypassTxEntity");
        pdcpDcMux->setGateSize("toBypassTxEntity", idx + 1);
        pdcpDcMux->gate("toBypassTxEntity", idx)->connectTo(module->gate("in"));

        // Wire entity out gate → LowerMux fromTxEntity (entity sends to RLC via LowerMux)
        int fromIdx = pdcpLowerMux->gateSize("fromTxEntity");
        pdcpLowerMux->setGateSize("fromTxEntity", fromIdx + 1);
        module->gate("out")->connectTo(pdcpLowerMux->gate("fromTxEntity", fromIdx));

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
        pdcpDcMux->registerBypassTxEntity(id, txEnt);
        pdcpBypassTxEntities_[id] = txEnt;
    }
}

void BearerManagement::setEntityParamsFromRlcMgr(cModule *entity, RlcEntityManager *rlcMgr)
{
    // Entities are NIC-level siblings of the RLC submodules, so their
    // defaults (which assumed being inside the LteRlc compound) are stale.
    // Propagate correct paths from the rlcMgr's own parameters.
    std::string mgrPath = std::string("^.") + rlcMgr->getName();
    if (entity->hasPar("macModule"))
        entity->par("macModule").setStringValue(rlcMgr->par("macModule").stringValue());
    if (entity->hasPar("umModule"))
        entity->par("umModule").setStringValue(mgrPath);
    if (entity->hasPar("upperMuxModule"))
        entity->par("upperMuxModule").setStringValue(rlcMgr->par("upperMuxModule").stringValue());
    if (entity->hasPar("isNR"))
        entity->par("isNR").setBoolValue(rlcMgr->par("isNR").boolValue());
}

RlcTxEntityBase *BearerManagement::createAndInstallRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcEntityManager *rlcMgr)
{
    LteRlcType rlcType = static_cast<LteRlcType>(lteInfo->getRlcType());
    cModuleType *moduleType;
    const char *prefix;
    switch (rlcType) {
        case TM: moduleType = rlcTmTxEntityModuleType_; prefix = "tm-tx"; break;
        case AM: moduleType = rlcAmTxEntityModuleType_; prefix = "am-tx"; break;
        default: moduleType = rlcUmTxEntityModuleType_; prefix = "um-tx"; break;
    }
    bool isNr = rlcMgr->par("isNR").boolValue();
    std::string name = std::string(isNr ? "nrRlc-" : "rlc-") + prefix + "-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    cModule *rlcCompound = rlcMgr->getRlcCompoundModule();
    auto *module = moduleType->create(name.c_str(), rlcCompound);
    setEntityParamsFromRlcMgr(module, rlcMgr);
    module->finalizeParameters();
    module->buildInside();

    // Wire gates: UpperMux → entity → LowerMux
    auto *upperMux = rlcMgr->getUpperMux();
    auto *lowerMux = rlcMgr->getLowerMux();

    // Wire UpperMux → entity in gate
    int idx = upperMux->gateSize("toTxEntity");
    upperMux->setGateSize("toTxEntity", idx + 1);
    upperMux->gate("toTxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity out gate → LowerMux fromTxEntity
    int fromIdx = lowerMux->gateSize("fromTxEntity");
    lowerMux->setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("out")->connectTo(lowerMux->gate("fromTxEntity", fromIdx));

    // Wire LowerMux macToTxEntity → entity macIn gate
    int macIdx = lowerMux->gateSize("macToTxEntity");
    lowerMux->setGateSize("macToTxEntity", macIdx + 1);
    lowerMux->gate("macToTxEntity", macIdx)->connectTo(module->gate("macIn"));

    module->scheduleStart(simTime());
    module->callInitialize();

    RlcTxEntityBase *txEnt = check_and_cast<RlcTxEntityBase *>(module);
    txEnt->setFlowControlInfo(lteInfo);

    // Register in mux routing table and CP entity map
    upperMux->registerTxBuffer(id, txEnt);
    (isNr ? nrRlcTxEntities_ : rlcTxEntities_)[id] = txEnt;

    // D2D: register per-peer tracking (UM entities only)
    bool hasD2DSupport = rlcMgr->par("hasD2DSupport").boolValue();
    if (hasD2DSupport) {
        UmTxEntity *umTxEnt = dynamic_cast<UmTxEntity *>(txEnt);
        if (umTxEnt != nullptr)
            upperMux->registerD2DPeerTxEntity(lteInfo->getD2dRxPeerId(), umTxEnt);
    }

    return txEnt;
}

RlcRxEntityBase *BearerManagement::createAndInstallRlcRxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcEntityManager *rlcMgr)
{
    LteRlcType rlcType = static_cast<LteRlcType>(lteInfo->getRlcType());
    cModuleType *moduleType;
    const char *prefix;
    switch (rlcType) {
        case TM: moduleType = rlcTmRxEntityModuleType_; prefix = "tm-rx"; break;
        case AM: moduleType = rlcAmRxEntityModuleType_; prefix = "am-rx"; break;
        default: moduleType = rlcUmRxEntityModuleType_; prefix = "um-rx"; break;
    }
    bool isNr = rlcMgr->par("isNR").boolValue();
    std::string name = std::string(isNr ? "nrRlc-" : "rlc-") + prefix + "-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    cModule *rlcCompound = rlcMgr->getRlcCompoundModule();
    auto *module = moduleType->create(name.c_str(), rlcCompound);
    setEntityParamsFromRlcMgr(module, rlcMgr);
    module->finalizeParameters();
    module->buildInside();

    // Wire gates: LowerMux → entity → UpperMux
    auto *upperMux = rlcMgr->getUpperMux();
    auto *lowerMux = rlcMgr->getLowerMux();

    // Wire LowerMux → entity in gate
    int idx = lowerMux->gateSize("toRxEntity");
    lowerMux->setGateSize("toRxEntity", idx + 1);
    lowerMux->gate("toRxEntity", idx)->connectTo(module->gate("in"));

    // Wire entity out gate → UpperMux fromRxEntity
    int fromIdx = upperMux->gateSize("fromRxEntity");
    upperMux->setGateSize("fromRxEntity", fromIdx + 1);
    module->gate("out")->connectTo(upperMux->gate("fromRxEntity", fromIdx));

    module->scheduleStart(simTime());
    module->callInitialize();

    RlcRxEntityBase *rxEnt = check_and_cast<RlcRxEntityBase *>(module);
    rxEnt->setFlowControlInfo(lteInfo);

    // Register in mux routing table and CP entity map
    lowerMux->registerRxBuffer(id, rxEnt);
    (isNr ? nrRlcRxEntities_ : rlcRxEntities_)[id] = rxEnt;

    return rxEnt;
}

RlcTxEntityBase *BearerManagement::createRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    Enter_Method_Silent("createRlcTxBuffer()");
    return createAndInstallRlcTxBuffer(id, lteInfo, rlcUmModule.get());
}

RlcRxEntityBase *BearerManagement::createRlcRxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    Enter_Method_Silent("createRlcRxBuffer()");
    return createAndInstallRlcRxBuffer(id, lteInfo, rlcUmModule.get());
}

void BearerManagement::deleteLocalPdcpEntities(MacNodeId nodeId)
{
    Enter_Method_Silent("deleteLocalPdcpEntities()");

    auto *pdcpUpperMux = check_and_cast<UpperMux *>(pdcpCompound_->getSubmodule("pdcpUpperMux"));
    auto *pdcpLowerMux = check_and_cast<LowerMux *>(pdcpCompound_->getSubmodule("pdcpLowerMux"));
    auto *pdcpDcMux = check_and_cast<DcMux *>(pdcpCompound_->getSubmodule("pdcpDcMux"));

    bool isEnb = (registration_->getNodeType() == NODEB);

    // Delete PDCP TX entities
    for (auto it = pdcpTxEntities_.begin(); it != pdcpTxEntities_.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            pdcpUpperMux->unregisterTxEntity(it->first);
            it->second->deleteModule();
            it = pdcpTxEntities_.erase(it);
        } else ++it;
    }

    // Delete PDCP RX entities
    for (auto it = pdcpRxEntities_.begin(); it != pdcpRxEntities_.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            pdcpLowerMux->unregisterRxEntity(it->first);
            it->second->deleteModule();
            it = pdcpRxEntities_.erase(it);
        } else ++it;
    }

    // Delete bypass TX entities
    for (auto it = pdcpBypassTxEntities_.begin(); it != pdcpBypassTxEntities_.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            pdcpDcMux->unregisterBypassTxEntity(it->first);
            it->second->deleteModule();
            it = pdcpBypassTxEntities_.erase(it);
        } else ++it;
    }

    // Delete bypass RX entities
    for (auto it = pdcpBypassRxEntities_.begin(); it != pdcpBypassRxEntities_.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            pdcpLowerMux->unregisterRxEntity(it->first);
            it->second->deleteModule();
            it = pdcpBypassRxEntities_.erase(it);
        } else ++it;
    }
}

void BearerManagement::deleteLocalRlcQueues(MacNodeId nodeId, bool nrStack)
{
    Enter_Method_Silent("deleteLocalRlcQueues()");

    auto &txMap = nrStack ? nrRlcTxEntities_ : rlcTxEntities_;
    auto &rxMap = nrStack ? nrRlcRxEntities_ : rlcRxEntities_;
    RlcEntityManager *rlcMgr = nrStack ? (nrRlcUmModule ? nrRlcUmModule.get() : nullptr) : rlcUmModule.get();
    if (!rlcMgr)
        return;

    auto *upperMux = rlcMgr->getUpperMux();
    auto *lowerMux = rlcMgr->getLowerMux();
    bool isEnb = (registration_->getNodeType() == NODEB);

    // Delete RLC TX entities
    for (auto it = txMap.begin(); it != txMap.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            upperMux->unregisterTxBuffer(it->first);
            it->second->deleteModule();
            it = txMap.erase(it);
        } else ++it;
    }

    // Delete RLC RX entities
    for (auto it = rxMap.begin(); it != rxMap.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            lowerMux->unregisterRxBuffer(it->first);
            it->second->deleteModule();
            it = rxMap.erase(it);
        } else ++it;
    }
}

} // namespace simu5g

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
#include "simu5g/stack/rrc/D2DModeController.h"
#include "simu5g/stack/rrc/Registration.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/RlcMux.h"
#include "simu5g/stack/rlc/um/UmTxEntity.h"
#include "simu5g/stack/pdcp/UpperMux.h"
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

        nicModule_ = inet::getContainingNicModule(this);

        rlcMuxModule.reference(this, "rlcMuxModule", true);
        nrRlcMuxModule.reference(this, "nrRlcMuxModule", false);
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
    bool isNr = (registration_->getNodeType()==UE && isNrUe(lteInfo->getDestId())); //TODO FIXME! DOES NOT WORK FOR MULTICAST!!!!!
    auto *rlcMux = isNr ? nrRlcMuxModule.get() : rlcMuxModule.get();
    createAndInstallRlcRxBuffer(rlcId, lteInfo, rlcMux, isNr);

    // PDCP entity creation
    auto *pdcpMux = check_and_cast<UpperMux *>(nicModule_->getSubmodule("pdcpMux"));
    auto *pdcpDcMux = dynamic_cast<DcMux *>(nicModule_->getSubmodule("pdcpDcMux")); // nullptr on UEs (no X2)

    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        std::string name = "pdcp-rx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpRxEntityModuleType_->create(name.c_str(), nicModule_);
        module->par("headerCompressedSize") = par("headerCompressedSize");
        module->finalizeParameters();
        module->buildInside();
        setEntityDisplayPosition(module, true, rlcMux, num(id.getDrbId()));

        // Wire RLC RX out → PDCP RX in (direct per-DRB connection)
        auto rlcIt = (isNrUe(lteInfo->getDestId()) ? nrRlcRxEntities_ : rlcRxEntities_).find(rlcId);
        ASSERT(rlcIt != (isNrUe(lteInfo->getDestId()) ? nrRlcRxEntities_ : rlcRxEntities_).end());
        rlcIt->second->gate("out")->connectTo(module->gate("in"));

        // Wire entity out gate → UpperMux fromRxEntity
        int fromIdx = pdcpMux->gateSize("fromRxEntity");
        pdcpMux->setGateSize("fromRxEntity", fromIdx + 1);
        module->gate("out")->connectTo(pdcpMux->gate("fromRxEntity", fromIdx));

        // Wire DcMux → entity dcIn gate (for UL X2 dispatch, eNB only)
        if (pdcpDcMux && module->hasGate("dcIn")) {
            int dcIdx = pdcpDcMux->gateSize("toRxEntity");
            pdcpDcMux->setGateSize("toRxEntity", dcIdx + 1);
            pdcpDcMux->gate("toRxEntity", dcIdx)->connectTo(module->gate("dcIn"));
        }

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
        pdcpRxEntities_[id] = rxEnt;
    }
    else {
        // DC secondary node: create bypass RX entity (forwards UL to master via X2)
        ASSERT(pdcpDcMux != nullptr); // bypass entities are eNB-only
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        std::string name = "pdcp-bypass-rx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpBypassRxEntityModuleType_->create(name.c_str(), nicModule_);
        module->finalizeParameters();
        module->buildInside();
        setEntityDisplayPosition(module, true, rlcMux, num(id.getDrbId()));

        // Wire RLC RX out → bypass PDCP RX in (direct per-DRB connection)
        auto rlcIt2 = (isNrUe(lteInfo->getDestId()) ? nrRlcRxEntities_ : rlcRxEntities_).find(rlcId);
        ASSERT(rlcIt2 != (isNrUe(lteInfo->getDestId()) ? nrRlcRxEntities_ : rlcRxEntities_).end());
        rlcIt2->second->gate("out")->connectTo(module->gate("in"));

        // Wire entity out gate → DcMux (bypass RX sends to X2 via DcMux)
        int fromIdx = pdcpDcMux->gateSize("fromEntity");
        pdcpDcMux->setGateSize("fromEntity", fromIdx + 1);
        module->gate("out")->connectTo(pdcpDcMux->gate("fromEntity", fromIdx));

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *rxEnt = check_and_cast<PdcpRxEntityBase *>(module);
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
    bool isNr = (registration_->getNodeType()==UE && isNrUe(lteInfo->getSourceId()));
    auto *rlcMux = isNr ? nrRlcMuxModule.get() : rlcMuxModule.get();
    createAndInstallRlcTxBuffer(rlcId, lteInfo, rlcMux, isNr);

    // PDCP entity creation
    auto *pdcpMux = check_and_cast<UpperMux *>(nicModule_->getSubmodule("pdcpMux"));
    auto *pdcpDcMux = dynamic_cast<DcMux *>(nicModule_->getSubmodule("pdcpDcMux")); // nullptr on UEs (no X2)

    if (withPdcp) {
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());

        // DC UE NR leg: check if a master (LTE-leg) PDCP TX entity exists for the same DRB.
        // If so, wire its nrOut gate to the NR RLC TX entity instead of creating a new PDCP entity.
        bool wiredToMaster = false;
        if (registration_->getNodeType()==UE && isNrUe(lteInfo->getSourceId())) {
            for (auto& [key, masterEntity] : pdcpTxEntities_) {
                if (key.getDrbId() == id.getDrbId()) {
                    auto *masterModule = check_and_cast<cModule *>(masterEntity);
                    if (masterModule->hasGate("nrOut")) {
                        auto nrRlcIt = nrRlcTxEntities_.find(rlcId);
                        ASSERT(nrRlcIt != nrRlcTxEntities_.end());
                        masterModule->gate("nrOut")->connectTo(nrRlcIt->second->gate("in"));
                        wiredToMaster = true;
                    }
                    break;
                }
            }
        }
        if (!wiredToMaster) {
            // Normal case: create PDCP TX entity
            std::string name = "pdcp-tx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
            auto *module = pdcpTxEntityModuleType_->create(name.c_str(), nicModule_);
            module->par("headerCompressedSize") = par("headerCompressedSize");
            module->finalizeParameters();
            module->buildInside();
            setEntityDisplayPosition(module, true, rlcMux, num(id.getDrbId()));

            // Wire UpperMux → entity in gate
            int idx = pdcpMux->gateSize("toTxEntity");
            pdcpMux->setGateSize("toTxEntity", idx + 1);
            pdcpMux->gate("toTxEntity", idx)->connectTo(module->gate("in"));

            // Wire PDCP TX out → RLC TX in (direct per-DRB connection)
            auto rlcIt = (isNrUe(lteInfo->getSourceId()) ? nrRlcTxEntities_ : rlcTxEntities_).find(rlcId);
            ASSERT(rlcIt != (isNrUe(lteInfo->getSourceId()) ? nrRlcTxEntities_ : rlcTxEntities_).end());
            module->gate("out")->connectTo(rlcIt->second->gate("in"));

            // Wire dcOut gate → DcMux (if entity has one, e.g. NrTxPdcpEntity; eNB only)
            if (pdcpDcMux && module->hasGate("dcOut")) {
                int dcIdx = pdcpDcMux->gateSize("fromEntity");
                pdcpDcMux->setGateSize("fromEntity", dcIdx + 1);
                module->gate("dcOut")->connectTo(pdcpDcMux->gate("fromEntity", dcIdx));
            }

            module->scheduleStart(simTime());
            module->callInitialize();
            auto *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
            pdcpMux->registerTxEntity(id, txEnt);
            pdcpTxEntities_[id] = txEnt;
        }
    }
    else {
        // DC secondary node: create bypass TX entity (forwards DL from master to RLC)
        ASSERT(pdcpDcMux != nullptr); // bypass entities are eNB-only
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        std::string name = "pdcp-bypass-tx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
        auto *module = pdcpBypassTxEntityModuleType_->create(name.c_str(), nicModule_);
        module->finalizeParameters();
        module->buildInside();
        setEntityDisplayPosition(module, true, rlcMux, num(id.getDrbId()));

        // Wire DcMux → entity in gate (DcMux dispatches incoming DL X2)
        int idx = pdcpDcMux->gateSize("toBypassTxEntity");
        pdcpDcMux->setGateSize("toBypassTxEntity", idx + 1);
        pdcpDcMux->gate("toBypassTxEntity", idx)->connectTo(module->gate("in"));

        // Wire bypass TX out → RLC TX in (direct per-DRB connection)
        auto rlcIt2 = (isNrUe(lteInfo->getSourceId()) ? nrRlcTxEntities_ : rlcTxEntities_).find(rlcId);
        ASSERT(rlcIt2 != (isNrUe(lteInfo->getSourceId()) ? nrRlcTxEntities_ : rlcTxEntities_).end());
        module->gate("out")->connectTo(rlcIt2->second->gate("in"));

        module->scheduleStart(simTime());
        module->callInitialize();
        auto *txEnt = check_and_cast<PdcpTxEntityBase *>(module);
        pdcpDcMux->registerBypassTxEntity(id, txEnt);
        pdcpBypassTxEntities_[id] = txEnt;
    }
}

void BearerManagement::setRlcEntityParams(cModule *entity, bool isNr)
{
    if (entity->hasPar("macModule"))
        entity->par("macModule").setStringValue(isNr ? "^.nrMac" : "^.mac");
    if (entity->hasPar("isNR"))
        entity->par("isNR").setBoolValue(isNr);
}

void BearerManagement::setEntityDisplayPosition(cModule *entity, bool isPdcpEntity, cModule *rlcMux, int bearerIndex)
{
    auto *pdcpMux = nicModule_->getSubmodule("pdcpMux");
    if (!pdcpMux || !rlcMux)
        return;

    int uy = atoi(pdcpMux->getDisplayString().getTagArg("p", 1));
    int lx = atoi(rlcMux->getDisplayString().getTagArg("p", 0));
    int ly = atoi(rlcMux->getDisplayString().getTagArg("p", 1));

    int x = lx + 60 * bearerIndex;
    int y = isPdcpEntity ? uy + (ly - uy) / 3 : uy + 2 * (ly - uy) / 3;

    entity->getDisplayString().setTagArg("p", 0, x);
    entity->getDisplayString().setTagArg("p", 1, y);
}

RlcTxEntityBase *BearerManagement::createAndInstallRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcMux *rlcMux, bool isNr)
{
    LteRlcType rlcType = static_cast<LteRlcType>(lteInfo->getRlcType());
    cModuleType *moduleType;
    const char *prefix;
    switch (rlcType) {
        case TM: moduleType = rlcTmTxEntityModuleType_; prefix = "tm-tx"; break;
        case AM: moduleType = rlcAmTxEntityModuleType_; prefix = "am-tx"; break;
        default: moduleType = rlcUmTxEntityModuleType_; prefix = "um-tx"; break;
    }
    std::string name = std::string(isNr ? "nrRlc-" : "rlc-") + prefix + "-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    auto *module = moduleType->create(name.c_str(), nicModule_);
    setRlcEntityParams(module, isNr);
    module->finalizeParameters();
    module->buildInside();
    setEntityDisplayPosition(module, false, rlcMux, num(id.getDrbId()));

    // Wire gates: entity → LowerMux (RLC TX 'in' gate is wired from PDCP TX in createOutgoingConnection)

    // Wire entity out gate → LowerMux fromTxEntity
    int fromIdx = rlcMux->gateSize("fromTxEntity");
    rlcMux->setGateSize("fromTxEntity", fromIdx + 1);
    module->gate("out")->connectTo(rlcMux->gate("fromTxEntity", fromIdx));

    // Wire LowerMux macToTxEntity → entity macIn gate
    int macIdx = rlcMux->gateSize("macToTxEntity");
    rlcMux->setGateSize("macToTxEntity", macIdx + 1);
    rlcMux->gate("macToTxEntity", macIdx)->connectTo(module->gate("macIn"));

    module->scheduleStart(simTime());
    module->callInitialize();

    RlcTxEntityBase *txEnt = check_and_cast<RlcTxEntityBase *>(module);
    txEnt->setFlowControlInfo(lteInfo);

    // Register in CP entity map
    (isNr ? nrRlcTxEntities_ : rlcTxEntities_)[id] = txEnt;

    // D2D peer tracking (only for UM TX entities)
    if (rlcType == UM) {
        auto *d2dCtrl = dynamic_cast<D2DModeController *>(nicModule_->getSubmodule("rrc")->getSubmodule("d2dModeController"));
        if (d2dCtrl) {
            auto *umTxEnt = check_and_cast<UmTxEntity *>(txEnt);
            d2dCtrl->registerD2DPeerTxEntity(MacNodeId(lteInfo->getD2dRxPeerId()), umTxEnt);
        }
    }

    return txEnt;
}

RlcRxEntityBase *BearerManagement::createAndInstallRlcRxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcMux *rlcMux, bool isNr)
{
    LteRlcType rlcType = static_cast<LteRlcType>(lteInfo->getRlcType());
    cModuleType *moduleType;
    const char *prefix;
    switch (rlcType) {
        case TM: moduleType = rlcTmRxEntityModuleType_; prefix = "tm-rx"; break;
        case AM: moduleType = rlcAmRxEntityModuleType_; prefix = "am-rx"; break;
        default: moduleType = rlcUmRxEntityModuleType_; prefix = "um-rx"; break;
    }
    std::string name = std::string(isNr ? "nrRlc-" : "rlc-") + prefix + "-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
    auto *module = moduleType->create(name.c_str(), nicModule_);
    setRlcEntityParams(module, isNr);
    module->finalizeParameters();
    module->buildInside();
    setEntityDisplayPosition(module, false, rlcMux, num(id.getDrbId()));

    // Wire gates: LowerMux → entity (RLC RX 'out' gate is wired to PDCP RX in createIncomingConnection)

    // Wire LowerMux → entity in gate
    int idx = rlcMux->gateSize("toRxEntity");
    rlcMux->setGateSize("toRxEntity", idx + 1);
    rlcMux->gate("toRxEntity", idx)->connectTo(module->gate("in"));

    module->scheduleStart(simTime());
    module->callInitialize();

    RlcRxEntityBase *rxEnt = check_and_cast<RlcRxEntityBase *>(module);
    rxEnt->setFlowControlInfo(lteInfo);

    // Register in mux routing table and CP entity map
    rlcMux->registerRxBuffer(id, rxEnt);
    (isNr ? nrRlcRxEntities_ : rlcRxEntities_)[id] = rxEnt;

    return rxEnt;
}

RlcTxEntityBase *BearerManagement::createRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    Enter_Method_Silent("createRlcTxBuffer()");
    return createAndInstallRlcTxBuffer(id, lteInfo, rlcMuxModule.get(), false);
}

RlcRxEntityBase *BearerManagement::createRlcRxBuffer(DrbKey id, FlowControlInfo *lteInfo)
{
    Enter_Method_Silent("createRlcRxBuffer()");
    return createAndInstallRlcRxBuffer(id, lteInfo, rlcMuxModule.get(), false);
}

void BearerManagement::deleteLocalPdcpEntities(MacNodeId nodeId)
{
    Enter_Method_Silent("deleteLocalPdcpEntities()");

    auto *pdcpMux = check_and_cast<UpperMux *>(nicModule_->getSubmodule("pdcpMux"));
    auto *pdcpDcMux = dynamic_cast<DcMux *>(nicModule_->getSubmodule("pdcpDcMux")); // nullptr on UEs (no X2)

    bool isEnb = (registration_->getNodeType() == NODEB);

    // Delete PDCP TX entities
    for (auto it = pdcpTxEntities_.begin(); it != pdcpTxEntities_.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            pdcpMux->unregisterTxEntity(it->first);
            it->second->deleteModule();
            it = pdcpTxEntities_.erase(it);
        } else ++it;
    }

    // Delete PDCP RX entities
    for (auto it = pdcpRxEntities_.begin(); it != pdcpRxEntities_.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            it->second->deleteModule();
            it = pdcpRxEntities_.erase(it);
        } else ++it;
    }

    // Delete bypass TX entities (eNB-only)
    ASSERT(pdcpBypassTxEntities_.empty() || pdcpDcMux != nullptr);
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
    RlcMux *rlcMux = nrStack ? (nrRlcMuxModule ? nrRlcMuxModule.get() : nullptr) : rlcMuxModule.get();
    if (!rlcMux)
        return;

    bool isEnb = (registration_->getNodeType() == NODEB);

    // Delete RLC TX entities
    for (auto it = txMap.begin(); it != txMap.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            it->second->deleteModule();
            it = txMap.erase(it);
        } else ++it;
    }

    // Delete RLC RX entities
    for (auto it = rxMap.begin(); it != rxMap.end(); ) {
        if (isEnb ? it->first.getNodeId() == nodeId : true) {
            rlcMux->unregisterRxBuffer(it->first);
            it->second->deleteModule();
            it = rxMap.erase(it);
        } else ++it;
    }
}

PdcpRxEntityBase *BearerManagement::lookupPdcpRxEntity(DrbKey id)
{
    auto it = pdcpRxEntities_.find(id);
    if (it != pdcpRxEntities_.end())
        return it->second;
    auto it2 = pdcpBypassRxEntities_.find(id);
    return it2 != pdcpBypassRxEntities_.end() ? it2->second : nullptr;
}

RlcTxEntityBase *BearerManagement::lookupRlcTxBuffer(DrbKey id)
{
    auto it = rlcTxEntities_.find(id);
    if (it != rlcTxEntities_.end())
        return it->second;
    auto it2 = nrRlcTxEntities_.find(id);
    return it2 != nrRlcTxEntities_.end() ? it2->second : nullptr;
}

PdcpTxEntityBase *BearerManagement::lookupPdcpTxEntity(DrbKey id)
{
    auto it = pdcpTxEntities_.find(id);
    return it != pdcpTxEntities_.end() ? it->second : nullptr;
}

void BearerManagement::pdcpActiveUeUL(std::set<MacNodeId> *ueSet)
{
    for (const auto& [id, rxEntity] : pdcpRxEntities_) {
        if (!rxEntity->isEmpty())
            ueSet->insert(id.getNodeId());
    }
}

} // namespace simu5g

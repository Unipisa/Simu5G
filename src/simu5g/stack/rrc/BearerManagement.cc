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
        std::string name = "rx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
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
    }
    else {
        // DC secondary node: create bypass RX entity (forwards UL to master via X2)
        DrbKey id = DrbKey(lteInfo->getSourceId(), lteInfo->getDrbId());
        std::string name = "bypass-rx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
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
        std::string name = "tx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
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
    }
    else {
        // DC secondary node: create bypass TX entity (forwards DL from master to RLC)
        DrbKey id = DrbKey(lteInfo->getDestId(), lteInfo->getDrbId());
        std::string name = "bypass-tx-" + std::to_string(num(id.getNodeId())) + "-" + std::to_string(num(id.getDrbId()));
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
    }
}

RlcTxEntityBase *BearerManagement::createAndInstallRlcTxBuffer(DrbKey id, FlowControlInfo *lteInfo, RlcEntityManager *rlcMgr)
{
    LteRlcType rlcType = static_cast<LteRlcType>(lteInfo->getRlcType());
    cModuleType *moduleType;
    const char *prefix;
    switch (rlcType) {
        case TM: moduleType = rlcTmTxEntityModuleType_; prefix = "TmTxEntity"; break;
        case AM: moduleType = rlcAmTxEntityModuleType_; prefix = "AmTxQueue"; break;
        default: moduleType = rlcUmTxEntityModuleType_; prefix = "UmTxEntity"; break;
    }
    std::string name = std::string(prefix) + " Lcid: " + std::to_string(num(id.getDrbId())) + " cid: " + std::to_string(id.asPackedInt());
    cModule *rlcCompound = rlcMgr->getRlcCompoundModule();
    auto *module = moduleType->create(name.c_str(), rlcCompound);
    module->finalizeParameters();
    module->buildInside();

    // Wire gates: UpperMux → entity → LowerMux
    auto *upperMux = check_and_cast<RlcUpperMux *>(rlcCompound->getSubmodule("upperMux"));
    auto *lowerMux = check_and_cast<RlcLowerMux *>(rlcCompound->getSubmodule("lowerMux"));

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

    // Register in mux map
    upperMux->registerTxBuffer(id, txEnt);

    // D2D: register per-peer tracking (UM entities only)
    bool hasD2DSupport = rlcCompound->par("d2dCapable").boolValue();
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
        case TM: moduleType = rlcTmRxEntityModuleType_; prefix = "TmRxEntity"; break;
        case AM: moduleType = rlcAmRxEntityModuleType_; prefix = "AmRxQueue"; break;
        default: moduleType = rlcUmRxEntityModuleType_; prefix = "UmRxEntity"; break;
    }
    std::string name = std::string(prefix) + " Lcid: " + std::to_string(num(id.getDrbId())) + " cid: " + std::to_string(id.asPackedInt());
    cModule *rlcCompound = rlcMgr->getRlcCompoundModule();
    auto *module = moduleType->create(name.c_str(), rlcCompound);
    module->finalizeParameters();
    module->buildInside();

    // Wire gates: LowerMux → entity → UpperMux
    auto *upperMux = check_and_cast<RlcUpperMux *>(rlcCompound->getSubmodule("upperMux"));
    auto *lowerMux = check_and_cast<RlcLowerMux *>(rlcCompound->getSubmodule("lowerMux"));

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

    // Register in mux map
    lowerMux->registerRxBuffer(id, rxEnt);

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
    pdcpModule->deleteEntities(nodeId);
}

void BearerManagement::deleteLocalRlcQueues(MacNodeId nodeId, bool nrStack)
{
    Enter_Method_Silent("deleteLocalRlcQueues()");
    if (nrStack) {
        if (nrRlcUmModule)
            nrRlcUmModule->deleteQueues(nodeId);
    }
    else {
        rlcUmModule->deleteQueues(nodeId);
    }
}

} // namespace simu5g

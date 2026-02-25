#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include "simu5g/stack/ip2nic/TechnologyDecision.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

using namespace inet;

Define_Module(TechnologyDecision);

TechnologyDecision::~TechnologyDecision()
{
    delete dcExpression_;
    delete useNrExpression_;
}

static cDynamicExpression *getExpressionFromPar(cPar& par, cDynamicExpression::IResolver *resolver)
{
    cObject *obj = par.objectValue();
    auto *exprObj = dynamic_cast<cOwnedDynamicExpression *>(obj);
    if (!exprObj)
        throw cRuntimeError("Parameter '%s' must be an expr() expression", par.getFullPath().c_str());
    auto *expr = exprObj->dup();
    expr->setResolver(resolver);
    return expr;
}

void TechnologyDecision::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        lowerLayerOut_ = gate("lowerLayerOut");

        nodeType_ = aToNodeType(par("nodeType").stdstringValue());

        binder_.reference(this, "binderModule", true);

        auto *networkIf = getContainingNicModule(this);
        dualConnectivityEnabled_ = networkIf->par("dualConnectivityEnabled").boolValue();

        if (nodeType_ == NODEB) {
            cModule *bs = getContainingNode(this);
            nodeId_ = MacNodeId(bs->par("macNodeId").intValue());
        }
        else if (nodeType_ == UE) {
            cModule *ue = getContainingNode(this);
            nodeId_ = MacNodeId(ue->par("macNodeId").intValue());
            if (ue->hasPar("nrMacNodeId"))
                nrNodeId_ = MacNodeId(ue->par("nrMacNodeId").intValue());
        }

        // Set up the policy expressions
        dcExpression_ = getExpressionFromPar(par("dcUseNrCondition"), new PolicyResolver(this));
        useNrExpression_ = getExpressionFromPar(par("useNrCondition"), new PolicyResolver(this));
    }
}

void TechnologyDecision::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);

    auto ipHeader = pkt->peekAtFront<inet::Ipv4Header>();
    auto srcAddr = ipHeader->getSrcAddress();
    auto destAddr = ipHeader->getDestAddress();
    short int tos = ipHeader->getTypeOfService();

    // Set evaluation context
    currentTypeOfService_ = tos;

    // Helper: compute per-flow packet ordinal (only needed when an expression is evaluated)
    auto computePacketOrdinal = [&]() {
        FlowKey key{srcAddr.getInt(), destAddr.getInt(), (uint16_t)tos};
        currentPacketOrdinal_ = splitBearersTable_[key]++;
    };

    bool useNr;

    if (nodeType_ == NODEB) {
        MacNodeId ueId = binder_->getMacNodeId(destAddr);
        MacNodeId nrUeId = binder_->getNrMacNodeId(destAddr);
        bool ueLteStack = (binder_->getServingNodeOrSelf(ueId) != NODEID_NONE);
        bool ueNrStack = (binder_->getServingNodeOrSelf(nrUeId) != NODEID_NONE);

        if (!ueLteStack && !ueNrStack) {
            EV << "TechnologyDecision: UE is not attached to any serving node. Delete packet." << endl;
            delete pkt;
            return;
        }
        else if (!ueNrStack)
            useNr = false;
        else if (!ueLteStack)
            useNr = true;
        else if (dualConnectivityEnabled_) {
            computePacketOrdinal();
            useNr = dcExpression_->boolValue();
        }
        else {
            computePacketOrdinal();
            useNr = useNrExpression_->boolValue();
        }
    }
    else { // UE
        // KLUDGE: use HandoverPacketHolder's serving node IDs instead of binder's,
        // to prevent a runtime error in one of the simulations:
        //
        //    test_numerology, multicell_CBR_UL, ue[9], t=0.001909132428, event #475 (HandoverPacketHolder), #476 (Ip2Nic)
        //    after: ueLteStack=true, ueNrStack=true, servingNodeId=1, nrServingNodeId=2, typeOfService=10 --> useNr = true
        //    before: ueLteStack=true, ueNrStack=true,servingNodeId=1, nrServingNodeId=0, typeOfService=10 --> useNr = false
        //
        auto handoverPacketHolder = check_and_cast<HandoverPacketHolderUe*>(getParentModule()->getSubmodule("handoverPacketHolder"));
        MacNodeId servingNodeId = handoverPacketHolder->getServingNodeId();
        MacNodeId nrServingNodeId = handoverPacketHolder->getNrServingNodeId();
        bool hasLteServing = (servingNodeId != NODEID_NONE);
        bool hasNrServing = (nrServingNodeId != NODEID_NONE);

        if (!hasLteServing && !hasNrServing) {
            EV << "TechnologyDecision: UE is not attached to any serving node. Delete packet." << endl;
            delete pkt;
            return;
        }
        else if (!hasNrServing)
            useNr = false;
        else if (!hasLteServing)
            useNr = true;
        else if (dualConnectivityEnabled_) {
            computePacketOrdinal();
            useNr = dcExpression_->boolValue();
        }
        else {
            computePacketOrdinal();
            useNr = useNrExpression_->boolValue();
        }
    }

    pkt->addTagIfAbsent<TechnologyReq>()->setUseNR(useNr);
    send(pkt, lowerLayerOut_);
}

cValue TechnologyDecision::PolicyResolver::readVariable(cExpression::Context *context, const char *name)
{
    if (!strcmp(name, "typeOfService")) return (intval_t)module_->currentTypeOfService_;
    if (!strcmp(name, "packetOrdinal")) return (intval_t)module_->currentPacketOrdinal_;
    throw cRuntimeError("TechnologyDecision: unknown variable '%s' in policy expression", name);
}

} //namespace

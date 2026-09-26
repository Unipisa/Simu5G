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

#include "simu5g/stack/pdcp/DcPdcpLegSplitter.h"

#include <inet/common/packet/Packet.h>
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/rlc/RlcTxEntityBase.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(DcPdcpLegSplitter);

simsignal_t DcPdcpLegSplitter::sentPacketToLowerLayerSignal_ = registerSignal("sentPacketToLowerLayer");
simsignal_t DcPdcpLegSplitter::pdcpSduSentSignal_ = registerSignal("pdcpSduSent");
simsignal_t DcPdcpLegSplitter::pdcpSduSentNrSignal_ = registerSignal("pdcpSduSentNr");

DcPdcpLegSplitter::~DcPdcpLegSplitter()
{
    delete legSelection_;
}

void DcPdcpLegSplitter::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);

        numLegs_ = par("numLegs");

        // The cell group of each leg: the id mapping and the liveness check go by cell
        // group, not by leg position (a split bearer's SCG leg is leg 1, an SCG bearer's
        // is its only leg, leg 0)
        cStringTokenizer tokenizer(par("legs"));
        while (tokenizer.hasMoreTokens()) {
            CellGroup group = aToCellGroup(tokenizer.nextToken());
            if (group == UNKNOWN_CELL_GROUP)
                throw cRuntimeError("DcPdcpLegSplitter: invalid cell group in the legs parameter, must be \"MCG\" or \"SCG\"");
            legGroups_.push_back(group);
        }
        if ((int)legGroups_.size() != numLegs_)
            throw cRuntimeError("DcPdcpLegSplitter: the legs parameter names %d legs, numLegs says %d",
                    (int)legGroups_.size(), numLegs_);

        legRlc_.assign(numLegs_, nullptr);

        cModule *node = inet::getContainingNode(this);
        nodeId_ = MacNodeId(node->par("macNodeId").intValue());
        if (node->hasPar("nrMacNodeId"))
            nrNodeId_ = MacNodeId(node->par("nrMacNodeId").intValue());
        isUe_ = (getNodeTypeById(nodeId_) == UE);

        WATCH(servingNodeId_);
        WATCH(nrServingNodeId_);
        WATCH(primaryPath_);
        WATCH(splitThreshold_);
        WATCH(currentPacketOrdinal_);
        WATCH(packetsSteered_);
    }
}

void DcPdcpLegSplitter::setServingNodeIds(MacNodeId servingNodeId, MacNodeId nrServingNodeId)
{
    Enter_Method_Silent("setServingNodeIds");
    servingNodeId_ = servingNodeId;
    nrServingNodeId_ = nrServingNodeId;
}

void DcPdcpLegSplitter::setSplitConfig(CellGroup primaryPath, int64_t ulDataSplitThreshold)
{
    Enter_Method_Silent("setSplitConfig");
    primaryPath_ = primaryPath;
    splitThreshold_ = ulDataSplitThreshold;
}

void DcPdcpLegSplitter::setLegRlc(int leg, RlcTxEntityBase *txEntity)
{
    Enter_Method_Silent("setLegRlc");
    ASSERT(leg >= 0 && leg < (int)legRlc_.size());
    legRlc_[leg] = txEntity;
}

int DcPdcpLegSplitter::primaryLeg() const
{
    for (int leg = 0; leg < numLegs_; leg++)
        if (legGroups_[leg] == primaryPath_)
            return leg;
    // A split bearer whose legs do not include its primary path is a configuration error
    // that establishment should have caught (see BearerConfigurator).
    throw cRuntimeError("DcPdcpLegSplitter: the primary path %s is not one of this bearer's legs",
            cellGroupToA(primaryPath_).c_str());
}

void DcPdcpLegSplitter::setLegSelection(const char *spec)
{
    Enter_Method("setLegSelection()");
    std::string src = spec;
    if (src.rfind("expr(", 0) != 0 || src.empty() || src.back() != ')')
        throw cRuntimeError("setLegSelection: '%s' is not an expression written as \"expr(...)\"", spec);
    auto *expr = new cDynamicExpression();
    try {
        expr->parse(src.substr(5, src.size() - 6).c_str());
    }
    catch (std::exception& e) {
        delete expr;
        throw;
    }
    expr->setResolver(new PolicyResolver(this));
    delete legSelection_;
    legSelection_ = expr;
}

bool DcPdcpLegSplitter::isLegLive(int leg, const FlowControlInfo *lteInfo)
{
    if (leg >= numLegs_)
        return false;   // not a leg of this bearer

    // Which technology serves this leg: the MCG leg is the anchor stack, whose technology
    // the flow's own (anchor) ids reveal; an SCG leg is the other one.
    bool anchorNr = isNrUe(isUe_ ? lteInfo->getSourceId() : lteInfo->getDestId());
    bool legNr = (legGroups_[leg] == MCG) ? anchorNr : !anchorNr;

    if (isUe_) {
        // this UE's own attachment on the leg's stack, as RRC pushed it -- current as of
        // handover start, ahead of the Binder (see BearerManagement::pushServingNodeIds())
        return (legNr ? nrServingNodeId_ : servingNodeId_) != NODEID_NONE;
    }

    // a base station: the UE's attachment on the stack this leg serves. The network
    // learns of a UE's handover by signaling, so the Binder is the authority here.
    MacNodeId peerId = binder_->getUeNodeId(lteInfo->getDestId(), legNr);
    return peerId != NODEID_NONE && binder_->getServingNodeOrSelf(peerId) != NODEID_NONE;
}

int DcPdcpLegSplitter::selectLeg(const FlowControlInfo *lteInfo)
{
    std::vector<bool> live(numLegs_, false);
    int liveLegs = 0, lastLiveLeg = 0;
    for (int leg = 0; leg < numLegs_; leg++)
        if (isLegLive(leg, lteInfo)) {
            live[leg] = true;
            liveLegs++;
            lastLiveLeg = leg;
        }

    if (liveLegs == 0) {
        EV_WARN << NOW << " DcPdcpLegSplitter - no leg of this bearer is available; falling back to leg 0" << endl;
        return 0;
    }
    if (liveLegs == 1)
        return lastLiveLeg;   // nothing to decide

    int primary = primaryLeg();

    // Downlink at a DC master: the secondary leg's RLC queue is at another node (across X2)
    // and cannot be weighed here, so there is no threshold or load-balancing -- the pushed
    // dlLegSelection (in legSelection_) decides each PDU's leg, or the bearer stays on its
    // primary leg. Real DL split flow control is X2-feedback-driven and is not modeled.
    if (!isUe_) {
        if (legSelection_ != nullptr)
            return applyLegSelection(live);
        return live[primary] ? primary : lastLiveLeg;
    }

    // Uplink at the UE, which sees both legs' RLC queues (TS 38.323 5.2.1): stay on the
    // primary path until the pending data reaches the split threshold, then use either leg.
    // The common "never split" bearer (infinite threshold) short-circuits before weighing
    // the legs.
    if (splitThreshold_ == SPLIT_THRESHOLD_INFINITY)
        return live[primary] ? primary : lastLiveLeg;

    int64_t pending = 0;
    for (int leg = 0; leg < numLegs_; leg++)
        if (live[leg] && legRlc_[leg] != nullptr)
            pending += legRlc_[leg]->getBufferOccupancy();

    if (pending < splitThreshold_)
        return live[primary] ? primary : lastLiveLeg;

    // At or above the threshold the spec allows either leg and leaves the choice to the
    // implementation (TS 38.323 5.2.1; ul-DataSplitThreshold b0 = "own algorithm"). The
    // pushed ulLegSelection is that algorithm, if the definition carries one; otherwise
    // offload to the least-occupied live leg.
    if (legSelection_ != nullptr)
        return applyLegSelection(live);
    int best = live[primary] ? primary : lastLiveLeg;
    int64_t bestOcc = legRlc_[best] != nullptr ? legRlc_[best]->getBufferOccupancy() : 0;
    for (int leg = 0; leg < numLegs_; leg++) {
        if (!live[leg])
            continue;
        int64_t occ = legRlc_[leg] != nullptr ? legRlc_[leg]->getBufferOccupancy() : 0;
        if (occ < bestOcc) {
            best = leg;
            bestOcc = occ;
        }
    }
    return best;
}

int DcPdcpLegSplitter::applyLegSelection(const std::vector<bool>& live)
{
    currentPacketOrdinal_ = packetsSteered_++;
    int leg = legSelection_->intValue();
    if (leg < 0 || leg >= numLegs_ || !live[leg])
        throw cRuntimeError("legSelection chose leg %d, which is not a live leg of this bearer", leg);
    return leg;
}

cValue DcPdcpLegSplitter::PolicyResolver::readVariable(cExpression::Context *context, const char *name)
{
    if (!strcmp(name, "packetOrdinal")) return (intval_t)module_->currentPacketOrdinal_;
    throw cRuntimeError("DcPdcpLegSplitter: unknown variable '%s' in the legSelection expression", name);
}

void DcPdcpLegSplitter::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    int leg = selectLeg(lteInfo.get());

    // Per-leg id adaptation + leg-flavored statistics (moved from NrPdcpTxEntity::deliverPdcpPdu).
    // The MCG leg is the anchor (master cell group) leg; an SCG leg is the UE's local
    // secondary stack, or a DC master's remote leg via X2. The flow's tags carry the anchor
    // stack's ids, so the anchor's technology can be read off them (isNrUe), and the
    // secondary stack is the other one -- under EN-DC the anchor is LTE and the secondary
    // NR, under NE-DC reversed.
    if (legGroups_[leg] == MCG) {
        // anchor leg: ids already correct
        EV << NOW << " DcPdcpLegSplitter - DRB ID[" << lteInfo->getDrbId() << "] - sending packet to the anchor leg's RLC" << endl;
        if (hasListeners(pdcpSduSentSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D)
            emit(pdcpSduSentSignal_, pkt);
        emit(sentPacketToLowerLayerSignal_, pkt);
    }
    else if (isUe_) {
        // UE's local secondary stack: translate to that stack's ids for its RLC (the
        // serving node is looked up per packet, so handover is honored)
        EV << NOW << " DcPdcpLegSplitter - DRB ID[" << lteInfo->getDrbId() << "] - sending packet to the secondary leg's RLC" << endl;
        MacNodeId scgNodeId = isNrUe(lteInfo->getSourceId()) ? nodeId_ : nrNodeId_;
        ASSERT(scgNodeId != NODEID_NONE);
        lteInfo->setSourceId(scgNodeId);
        lteInfo->setDestId(binder_->getServingNodeOrSelf(scgNodeId));
        if (hasListeners(pdcpSduSentNrSignal_) && lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D)
            emit(pdcpSduSentNrSignal_, pkt);
        emit(sentPacketToLowerLayerSignal_, pkt);
    }
    else {
        // DC master's remote leg: rewrite to (secondary node, the UE's secondary-stack id)
        // and address the X2 tunnel; the PDU leaves via the DcMux
        EV << NOW << " DcPdcpLegSplitter - DRB ID[" << lteInfo->getDrbId() << "] - the destination is under the control of a secondary node" << endl;
        MacNodeId secondaryNodeId = binder_->getSecondaryNode(nodeId_);
        ASSERT(secondaryNodeId != NODEID_NONE);
        ASSERT(secondaryNodeId != nodeId_);
        MacNodeId scgDestId = binder_->getUeNodeId(lteInfo->getDestId(), !isNrUe(lteInfo->getDestId()));
        ASSERT(scgDestId != NODEID_NONE);
        lteInfo->setSourceId(secondaryNodeId);
        lteInfo->setDestId(scgDestId);
        pkt->addTagIfAbsent<X2TargetReq>()->setTargetNode(secondaryNodeId);
    }

    send(pkt, "out", leg);
}

} // namespace simu5g

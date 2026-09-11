//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
#include "simu5g/stack/mac/scheduling_modules/QoSAwareScheduler.h"
#include "simu5g/stack/mac/scheduler/LteSchedulerEnb.h"

namespace simu5g {

using namespace omnetpp;

QoSAwareScheduler::QoSAwareScheduler(Binder* binder, double pfAlpha)
    : LteScheduler(binder), pfAlpha_(pfAlpha)
{
}

double QoSAwareScheduler::computeQosWeight(const DrbQosProfile& e)
{
    // The scheduler weighs by the priority level, so a profile without a usable
    // one cannot be scheduled meaningfully -- and the field's default 0 would
    // otherwise silently outrank every legitimate priority.
    if (e.priorityLevel < 1) {
        std::ostringstream os;
        os << e;
        throw cRuntimeError("QoSAwareScheduler: DRB QoS profile {%s} has no usable priority level "
                            "(the 3GPP range is 1..127): state qosPriorityLevel in the bearer's "
                            "QoS profile, or use a predefined qci-*/5qi-* profile", os.str().c_str());
    }
    double weight = 1.0;
    if (e.gbr) weight *= gbrMultiplier_;
    // Reciprocal in the priority level: lower level = higher weight, and the
    // weight RATIO of two bearers depends only on the ratio of their priority
    // values, so the QCI priority scale (1..9) and the 5QI scale (10..90, the
    // same relative standing at 10x the numbers) discriminate identically.
    weight *= priorityBase_ / e.priorityLevel;
    if (e.delayBudgetMs <= delayUrgentMs_) weight *= delayUrgentMultiplier_;
    else if (e.delayBudgetMs <= delayTightMs_) weight *= delayTightMultiplier_;
    else if (e.delayBudgetMs <= delayLooseMs_) weight *= delayLooseMultiplier_;
    return weight;
}

const DrbQosProfile *QoSAwareScheduler::getDrbQosForCid(MacCid cid)
{
    if (!drbQosMap_) return nullptr;

    // an uplink pseudo-connection stands for a whole logical channel group;
    // weigh it by the DRBs behind it
    if (isUlBsrLcid(cid.getLcid()))
        return getQosForUlGroup(cid.getNodeId(), lcgFromBsrLcid(cid.getLcid()));

    DrbKey key(cid.getNodeId(), eNbScheduler_->mac_->lcidToDrbId(cid.getLcid()));
    auto it = drbQosMap_->find(key);
    if (it != drbQosMap_->end())
        return &it->second;
    EV_WARN << "QoSAwareScheduler: No DRB QoS profile for CID " << cid << " (" << key << ")\n";
    return nullptr;
}

const DrbQosProfile *QoSAwareScheduler::getQosForUlGroup(MacNodeId ueId, Lcg lcg)
{
    // The MAC knows each configured DRB's QoS profile (the RRC push behind
    // drbQosMap_) and each established channel's group (lcConfig_); their join
    // is the group's membership. A configured-but-unestablished DRB has no
    // channel config yet and is skipped.
    std::vector<const DrbQosProfile *> members;
    for (const auto& [key, profile] : *drbQosMap_) {
        if (key.getNodeId() != ueId)
            continue;
        MacCid channel(ueId, LogicalCid(num(key.getDrbId())));
        const LogicalChannelConfig *lcConfig = eNbScheduler_->mac_->findLogicalChannelConfig(channel);
        if (lcConfig == nullptr || lcConfig->lcg != lcg)
            continue;
        members.push_back(&profile);
    }
    if (members.empty()) {
        EV_WARN << "QoSAwareScheduler: no DRB QoS profile behind LCG " << (int)num(lcg)
                << " of node " << ueId << "\n";
        return nullptr;
    }
    groupQos_ = aggregateQosProfiles(members);
    return &groupQos_;
}

DrbQosProfile QoSAwareScheduler::aggregateQosProfiles(const std::vector<const DrbQosProfile *>& members)
{
    DrbQosProfile aggregate = *members.front();
    for (const DrbQosProfile *member : members) {
        aggregate.priorityLevel = std::min(aggregate.priorityLevel, member->priorityLevel);
        aggregate.gbr = aggregate.gbr || member->gbr;
        aggregate.delayBudgetMs = std::min(aggregate.delayBudgetMs, member->delayBudgetMs);
        aggregate.packetErrorRate = std::min(aggregate.packetErrorRate, member->packetErrorRate);
    }
    return aggregate;
}

void QoSAwareScheduler::prepareSchedule()
{
    if (!drbQosMap_)
        throw cRuntimeError("QoSAwareScheduler requires DRB QoS profiles but none were configured. "
                            "Author them via the qos fields (gbr/delayBudget/per/priority) of the "
                            "bearerConfigurator.staticDrbs entries.");

    EV << NOW << " QoSAwareScheduler::prepareSchedule" << endl;

    grantedBytes_.clear();
    activeConnectionTempSet_ = *activeConnectionSet_;

    // --- Phase 1: Score all eligible CIDs ---

    struct CidInfo { double score; unsigned int availableBytes; };
    std::map<MacCid, CidInfo> cidInfo;

    for (const auto& cid : carrierActiveConnectionSet_) {
        EV << NOW << " QoSAwareScheduler::CID--->"<< cid << endl;
        MacNodeId nodeId = cid.getNodeId();
        grantedBytes_[cid] = 0;

        if (nodeId == NODEID_NONE || !binder_->nodeExists(nodeId)) {
            activeConnectionSet_->erase(cid);
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            continue;
        }

        Direction dir = (direction_ == UL) ? directionFromBsrLcid(cid.getLcid(), UL) : DL;

        if (dir != UL && dir != DL) continue;

        const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId, dir, carrierFrequency_);
        if (info.readCqiVector().empty() || info.readBands().empty()) continue;
        if (eNbScheduler_->allocatedCws(nodeId) == info.getLayers().size()) continue;

        bool cqiNull = std::any_of(info.readCqiVector().begin(), info.readCqiVector().end(), [](int cqi) { return cqi == 0; });
        if (cqiNull) continue;

        unsigned int availableBlocks = 0, availableBytes = 0;
        for (auto antenna : info.readAntennaSet()) {
            for (auto band : info.readBands()) {
                unsigned int blocks = eNbScheduler_->readAvailableRbs(nodeId, antenna, band);
                availableBlocks += blocks;
                availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, band, blocks, dir, carrierFrequency_);
            }
        }

        const DrbQosProfile *qos = getDrbQosForCid(cid);
        double qosWeight = qos ? computeQosWeight(*qos) : 1.0;

        EV << NOW << " QoSAwareScheduler::Cid: "<< cid << " QoS Weight: " << qosWeight << endl;
        if (!pfRate_.count(cid)) pfRate_[cid] = 0;

        double s = 0.0;
        if (pfRate_[cid] < scoreEpsilon_)
            s = qosWeight / scoreEpsilon_;
        else if (availableBlocks > 0)
            s = qosWeight * (static_cast<double>(availableBytes) / availableBlocks / pfRate_[cid]);
        else
            s = 0.0;

        cidInfo[cid] = {s, availableBytes};
    }

    // --- Phase 2: Select best CID per node, compute proportional quotas ---
    // Only one CID per node can be granted per TTI (scheduleGrant constraint),
    // so quotas are computed at the node level using the best CID's score.

    std::map<MacNodeId, MacCid> bestCidPerNode;
    for (const auto& [cid, info] : cidInfo) {
        MacNodeId node = cid.getNodeId();
        auto it = bestCidPerNode.find(node);
        if (it == bestCidPerNode.end() || info.score > cidInfo[it->second].score)
            bestCidPerNode[node] = cid;
    }

    double totalScore = 0;
    for (const auto& [node, cid] : bestCidPerNode)
        totalScore += cidInfo[cid].score;

    std::map<MacCid, unsigned int> grantQuota;
    for (const auto& [node, cid] : bestCidPerNode) {
        double share = (totalScore > scoreEpsilon_)
            ? cidInfo[cid].score / totalScore
            : 1.0 / bestCidPerNode.size();
        grantQuota[cid] = std::max(1u, static_cast<unsigned int>(cidInfo[cid].availableBytes * share));
        EV << NOW << " QoSAwareScheduler::Quota node=" << node << " cid=" << cid
           << " score=" << cidInfo[cid].score << " share=" << share
           << " quota=" << grantQuota[cid] << "B" << endl;
    }

    // --- Phase 3: Grant in score order, capped to proportional quota ---

    auto compare = [](const ScoredCid& a, const ScoredCid& b) { return a.second < b.second; };
    std::priority_queue<ScoredCid, std::vector<ScoredCid>, decltype(compare)> grantQueue(compare);
    for (const auto& [cid, info] : cidInfo)
        grantQueue.push({cid, info.score + uniform(getEnvir()->getRNG(0), -scoreEpsilon_ / 2.0, scoreEpsilon_ / 2.0)});

    while (!grantQueue.empty()) {
        ScoredCid current = grantQueue.top();
        grantQueue.pop();
        MacCid cid = current.first;

        auto qIt = grantQuota.find(cid);
        unsigned int limit = (qIt != grantQuota.end()) ? qIt->second : UINT32_MAX;

        bool terminate = false, active = true, eligible = true;
        unsigned int granted = requestGrant(cid, limit, terminate, active, eligible);
        grantedBytes_[cid] += granted;

        EV << NOW << " QoSAwareScheduler::Grant cid=" << cid << " limit=" << limit
           << " granted=" << granted << "B" << endl;

        if (terminate) break;
        if (!active) {
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
        }
    }
}

void QoSAwareScheduler::commitSchedule()
{
    for (const auto& [cid, granted] : grantedBytes_) {
        double& longTermRate = pfRate_[cid];
        longTermRate = (1.0 - pfAlpha_) * longTermRate + pfAlpha_ * static_cast<double>(granted);
    }
    *activeConnectionSet_ = activeConnectionTempSet_;
}

} // namespace simu5g






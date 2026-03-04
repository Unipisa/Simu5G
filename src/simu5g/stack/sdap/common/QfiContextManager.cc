//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
///

#include "simu5g/stack/sdap/common/QfiContextManager.h"
#include <omnetpp/cvaluearray.h>
#include <omnetpp/cvaluemap.h>

using namespace omnetpp;

namespace simu5g {

void QfiContextManager::loadFromJson(const cValueArray *arr)
{
    // First pass: count per-UE DRBs to derive LCIDs
    std::map<MacNodeId, int> ueLocalDrbCount;

    for (int i = 0; i < (int)arr->size(); i++) {
        const cValueMap *entry = check_and_cast<const cValueMap *>(arr->get(i).objectValue());

        DrbContext ctx;
        ctx.drbIndex = entry->get("drb").intValue();

        // "ue" field: numeric MacNodeId (gNB side); omitted on UE side
        if (entry->containsKey("ue"))
            ctx.ueNodeId = MacNodeId(entry->get("ue").intValue());
        else
            ctx.ueNodeId = NODEID_NONE; // UE side: "self"

        // qfiList
        const cValueArray *qfiArr = check_and_cast<const cValueArray *>(entry->get("qfiList").objectValue());
        for (int j = 0; j < (int)qfiArr->size(); j++)
            ctx.qfiList.push_back(qfiArr->get(j).intValue());

        // Derive LCID = local DRB index within this UE's DRB set
        ctx.lcid = ueLocalDrbCount[ctx.ueNodeId]++;

        drbMap_[ctx.drbIndex] = ctx;

        // Build derived lookup tables
        for (int qfi : ctx.qfiList) {
            if (ctx.ueNodeId != NODEID_NONE)
                ueQfiToDrb_[{ctx.ueNodeId, qfi}] = ctx.drbIndex;
            else
                qfiToDrb_[qfi] = ctx.drbIndex;
        }
    }
}

int QfiContextManager::getDrbIndex(MacNodeId ueNodeId, int qfi) const
{
    auto it = ueQfiToDrb_.find({ueNodeId, qfi});
    return it != ueQfiToDrb_.end() ? it->second : -1;
}

int QfiContextManager::getFirstDrbForUe(MacNodeId ueNodeId) const
{
    for (const auto& [drb, ctx] : drbMap_) {
        if (ctx.ueNodeId == ueNodeId)
            return drb;
    }
    return -1;
}

int QfiContextManager::getDrbIndexForQfi(int qfi) const
{
    auto it = qfiToDrb_.find(qfi);
    return it != qfiToDrb_.end() ? it->second : -1;
}

const DrbContext* QfiContextManager::getDrbContext(int drbIndex) const
{
    auto it = drbMap_.find(drbIndex);
    return it != drbMap_.end() ? &it->second : nullptr;
}

int QfiContextManager::getLcid(int drbIndex) const
{
    const DrbContext *ctx = getDrbContext(drbIndex);
    return ctx ? ctx->lcid : -1;
}

int QfiContextManager::getDrbIndexForMacCid(MacNodeId ueNodeId, LogicalCid lcid) const
{
    // Try exact (ueNodeId, lcid) match first (gNB side)
    for (const auto& [drb, ctx] : drbMap_) {
        if (ctx.ueNodeId == ueNodeId && ctx.lcid == (int)lcid)
            return drb;
    }
    // Fallback: try NODEID_NONE entries matching only on lcid (UE side)
    for (const auto& [drb, ctx] : drbMap_) {
        if (ctx.ueNodeId == NODEID_NONE && ctx.lcid == (int)lcid)
            return drb;
    }
    return -1;
}

void QfiContextManager::dump(std::ostream& os) const
{
    os << "=== QfiContextManager dump (" << drbMap_.size() << " DRBs) ===" << std::endl;
    for (const auto& [drb, ctx] : drbMap_) {
        os << "  DRB " << drb << ": " << ctx << std::endl;
    }
    os << "==============================" << std::endl;
}

} // namespace simu5g


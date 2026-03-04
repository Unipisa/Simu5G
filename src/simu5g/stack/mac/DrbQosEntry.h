#ifndef __SIMU5G_DRBQOSENTRY_H_
#define __SIMU5G_DRBQOSENTRY_H_

#include "simu5g/common/LteCommon.h"

namespace simu5g {

struct DrbQosEntry {
    int drbIndex = -1;
    MacNodeId ueNodeId = NODEID_NONE;
    int lcid = -1;  // derived: per-UE DRB counter
    bool gbr = false;
    double delayBudgetMs = 0;
    double packetErrorRate = 0;
    int priorityLevel = 0;
};

inline std::ostream& operator<<(std::ostream& os, const DrbQosEntry& e) {
    os << "drb=" << e.drbIndex << " ue=" << e.ueNodeId << " lcid=" << e.lcid
       << " gbr=" << e.gbr << " delay=" << e.delayBudgetMs
       << "ms per=" << e.packetErrorRate << " prio=" << e.priorityLevel;
    return os;
}

} // namespace simu5g

#endif

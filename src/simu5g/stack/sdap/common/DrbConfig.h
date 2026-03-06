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

#ifndef STACK_SDAP_COMMON_DRBCONFIG_H_
#define STACK_SDAP_COMMON_DRBCONFIG_H_

#include <vector>
#include "simu5g/common/LteCommon.h"

namespace simu5g {

// Per-DRB configuration entry, built from the drbConfig JSON parameter.
struct DrbConfig {
    DrbId drbId = DRBID_NONE;         // DRB identifier (= nrPdcp[]/nrRlc[] array index)
    MacNodeId ueNodeId = NODEID_NONE; // NODEID_NONE = "self" (UE side); numeric MacNodeId on gNB
    bool isDefault = false;           // true if this is the default DRB for this UE
    std::vector<Qfi> qfiList;       // QFIs mapped to this DRB
    LteRlcType rlcType = UM;        // RLC mode for this DRB (AM, UM, TM)
    PduSessionType pduSessionType = IP_V4;  // PDU session type (3GPP TS 23.501)
    std::string upperProtocol;  // INET protocol name for upper layer dispatch (empty = derive from pduSessionType)
};

inline std::ostream& operator<<(std::ostream& os, const DrbConfig& ctx) {
    os << "drbId=" << ctx.drbId << " ue=" << ctx.ueNodeId;
    if (ctx.isDefault) os << " DEFAULT";
    os << " qfi=[";
    for (int i = 0; i < (int)ctx.qfiList.size(); i++) {
        if (i) os << ",";
        os << ctx.qfiList[i];
    }
    os << "] rlc=" << rlcTypeToA(ctx.rlcType) << " pduSession=" << pduSessionTypeToA(ctx.pduSessionType);
    if (!ctx.upperProtocol.empty())
        os << " upperProto=" << ctx.upperProtocol;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::pair<MacNodeId,Qfi>& p) {
    os << "(" << p.first << "," << p.second << ")";
    return os;
}

} // namespace simu5g

#endif /* STACK_SDAP_COMMON_DRBCONFIG_H_ */

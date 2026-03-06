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


#ifndef STACK_SDAP_COMMON_DRBTABLE_H_
#define STACK_SDAP_COMMON_DRBTABLE_H_

#include <map>
#include <vector>
#include <iostream>
#include "DrbConfig.h"
#include "simu5g/common/LteCommon.h"

namespace omnetpp { class cValueArray; }

namespace simu5g {

class DrbTable
{
  protected:
    // Primary table: DrbKey(nodeId, drbId) -> DrbConfig (owns DrbConfig objects)
    std::map<DrbKey, DrbConfig> drbMap_;

    // Reverse lookup: (nodeId, qfi) -> DrbConfig*  (UE uses NODEID_NONE as nodeId)
    std::map<std::pair<MacNodeId,Qfi>, const DrbConfig*> qfiToDrb_;

    // Default DRB per nodeId (UE: NODEID_NONE key; gNB: per-UE keys)
    std::map<MacNodeId, const DrbConfig*> defaultDrb_;

    // UE shortcut: default DRB for NODEID_NONE (nullptr if not configured)
    const DrbConfig* ueDefaultDrb_ = nullptr;

  public:
    void loadFromJson(const omnetpp::cValueArray *arr);

    // Primary lookup by DrbKey
    const DrbConfig* getDrb(DrbKey key) const;

    // Reverse lookup: (nodeId, qfi) -> DrbConfig  (UE passes NODEID_NONE)
    const DrbConfig* getDrbForQfi(MacNodeId nodeId, Qfi qfi) const;

    // Default DRB for a given nodeId (gNB: per-UE; UE: NODEID_NONE)
    const DrbConfig* getDefaultDrb(MacNodeId nodeId) const;

    // UE shortcut: default DRB (NODEID_NONE)
    const DrbConfig* getDefaultDrb() const { return ueDefaultDrb_; }

    // Access full DRB map
    const std::map<DrbKey, DrbConfig>& getDrbMap() const { return drbMap_; }

    void dump(std::ostream& os = std::cout) const;
};

} // namespace simu5g

#endif /* STACK_SDAP_COMMON_DRBTABLE_H_ */

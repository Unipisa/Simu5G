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


#ifndef STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_
#define STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_

#include <map>
#include <vector>
#include <iostream>
#include "QfiContext.h"
#include "simu5g/common/LteCommon.h"

namespace omnetpp { class cValueArray; }

namespace simu5g {

class QfiContextManager
{
  protected:
    // Primary table: global drbIndex -> DrbContext
    std::map<int, DrbContext> drbMap_;

    // Derived lookup: (ueNodeId, qfi) -> global drbIndex  [gNB TX path]
    std::map<std::pair<MacNodeId,int>, int> ueQfiToDrb_;

    // Derived lookup: qfi -> global drbIndex  [UE path, ueNodeId==0]
    std::map<int, int> qfiToDrb_;

  public:
    void loadFromJson(const omnetpp::cValueArray *arr);

    // gNB TX: given dest UE nodeId + QFI -> global drbIndex (-1 if not found)
    int getDrbIndex(MacNodeId ueNodeId, int qfi) const;

    // UE TX: given QFI -> global drbIndex (-1 if not found)
    int getDrbIndexForQfi(int qfi) const;

    // gNB TX fallback: given dest UE nodeId -> first configured drbIndex for that UE (-1 if not found)
    int getFirstDrbForUe(MacNodeId ueNodeId) const;

    // Any: given global drbIndex -> DrbContext (nullptr if not found)
    const DrbContext* getDrbContext(int drbIndex) const;

    // PDCP: given global drbIndex -> LCID (-1 if not found)
    int getLcid(int drbIndex) const;

    // MacDrbMultiplexer: given (ueNodeId, lcid) -> global drbIndex (-1 if not found)
    int getDrbIndexForMacCid(MacNodeId ueNodeId, LogicalCid lcid) const;

    // Access full DRB map
    const std::map<int, DrbContext>& getDrbMap() const { return drbMap_; }

    void dump(std::ostream& os = std::cout) const;
};

} // namespace simu5g

#endif /* STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_ */

//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _PDCP_ENTITY_MANAGER_H_
#define _PDCP_ENTITY_MANAGER_H_

#include <set>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

using namespace omnetpp;

class PdcpTxEntityBase;
class PdcpRxEntityBase;
class UpperMux;
class LowerMux;
class DcMux;

/**
 * @class PdcpEntityManager
 * @brief Thin facade for PDCP entity management.
 *
 * Provides the public API for entity creation, deletion, and lookup.
 * Delegates to UpperMux (TX entities) and LowerMux (RX + bypass entities)
 * for actual storage and gate wiring. Does not handle packets itself.
 */
class PdcpEntityManager : public cSimpleModule
{
  protected:
    UpperMux *upperMux_ = nullptr;
    LowerMux *lowerMux_ = nullptr;
    DcMux *dcMux_ = nullptr;

  public:
    ~PdcpEntityManager() override;

    PdcpTxEntityBase *lookupTxEntity(DrbKey id);
    PdcpRxEntityBase *lookupRxEntity(DrbKey id);

    cModule *getPdcpCompoundModule() { return getParentModule(); }

    void deleteEntities(MacNodeId nodeId);
    void activeUeUL(std::set<MacNodeId> *ueSet);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} //namespace

#endif

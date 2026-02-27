//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _PDCP_MUX_H_
#define _PDCP_MUX_H_

#include <set>
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

using namespace omnetpp;

class PdcpTxEntityBase;
class PdcpRxEntityBase;
class UpperMux;
class LowerMux;

/**
 * @class PdcpMux
 * @brief Thin facade for PDCP entity management.
 *
 * Provides the public API for entity creation, deletion, and lookup.
 * Delegates to UpperMux (TX entities) and LowerMux (RX + bypass entities)
 * for actual storage and gate wiring. Does not handle packets itself.
 */
class PdcpMux : public cSimpleModule
{
  protected:
    UpperMux *upperMux_ = nullptr;
    LowerMux *lowerMux_ = nullptr;

  public:
    ~PdcpMux() override;

    PdcpTxEntityBase *lookupTxEntity(DrbKey id);
    PdcpTxEntityBase *createTxEntity(DrbKey id);
    PdcpRxEntityBase *lookupRxEntity(DrbKey id);
    PdcpRxEntityBase *createRxEntity(DrbKey id);
    PdcpTxEntityBase *createBypassTxEntity(DrbKey id);
    PdcpRxEntityBase *createBypassRxEntity(DrbKey id);

    void deleteEntities(MacNodeId nodeId);
    void activeUeUL(std::set<MacNodeId> *ueSet);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} //namespace

#endif

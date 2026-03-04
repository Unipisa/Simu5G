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

#ifndef _RLC_ENTITY_MANAGER_H_
#define _RLC_ENTITY_MANAGER_H_

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class RlcMux;

/**
 * @class RlcEntityManager
 * @brief Thin facade remaining from the old RLC entity manager.
 *
 * Most functionality has been moved to BearerManagement (entity lifecycle),
 * D2DModeController (D2D peer tracking), and RlcMux (throughput stats).
 * This module only provides getLowerMux() for BearerManagement and
 * NED parameters used by dynamically created RLC entities.
 */
class RlcEntityManager : public cSimpleModule
{
  protected:
    RlcMux *lowerMux_ = nullptr;

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    RlcMux *getLowerMux() { return lowerMux_; }
};

} //namespace

#endif

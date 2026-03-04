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
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/mec/utils/MecCommon.h"

namespace simu5g {

using namespace omnetpp;

class RlcMux;

/**
 * @class RlcEntityManager
 * @brief RLC entity lifecycle manager.
 *
 * Thin facade that delegates entity creation and lookup
 * to RlcUpperMux (TX entities) and RlcMux (RX entities).
 * Also manages per-UE throughput statistics and provides
 * deleteQueues() for handover cleanup.
 */
class RlcEntityManager : public cSimpleModule
{
  protected:
    RlcMux *lowerMux_ = nullptr;

    /**
    * @author Alessandro Noferi
    * Holds the throughput stats for each UE
    * identified by the srcId of the
    * FlowControlInfo in each entity
    *
    */
    typedef std::map<MacNodeId, Throughput> ULThroughputPerUE;
    ULThroughputPerUE ulThroughput_;

  public:
    void activeUeUL(std::set<MacNodeId> *ueSet);

    /**
     * @author Alessandro Noferi
     * Methods used by RlcEntity and EnodeBCollector respectively
     * in order to manage UL throughput stats.
     */

    void addUeThroughput(MacNodeId nodeId, Throughput throughput);
    double getUeThroughput(MacNodeId nodeId);
    void resetThroughputStats(MacNodeId nodeId);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    void finish() override
    {
    }

  public:
    RlcMux *getLowerMux() { return lowerMux_; }

};

} //namespace

#endif

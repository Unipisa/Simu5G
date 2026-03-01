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

class RlcUpperMux;
class RlcLowerMux;
class UmTxEntity;
class UmRxEntity;

/**
 * @class RlcEntityManager
 * @brief RLC entity lifecycle manager.
 *
 * Thin facade that delegates entity creation and lookup
 * to RlcUpperMux (TX entities) and RlcLowerMux (RX entities).
 * Also manages per-UE throughput statistics and provides
 * deleteQueues() for handover cleanup.
 */
class RlcEntityManager : public cSimpleModule
{
  protected:
    RlcUpperMux *upperMux_ = nullptr;
    RlcLowerMux *lowerMux_ = nullptr;

    // (TX entities are managed by RlcUpperMux, RX entities by RlcLowerMux)

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

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    void deleteQueues(MacNodeId nodeId);

    void resumeDownstreamInPackets(MacNodeId peerId);

    bool isEmptyingTxBuffer(MacNodeId peerId);

    /**
     * @author Alessandro Noferi
     * It fills the ueSet argument with the MacNodeIds that have
     * RLC data in the entities
     *
     */
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
    /**
     * lookupTxBuffer() searches for an existing TXBuffer for the given DrbKey.
     *
     * @param id DrbKey to lookup
     * @return pointer to the TXBuffer if found, nullptr otherwise
     */
    UmTxEntity *lookupTxBuffer(DrbKey id);

    /**
     * createTxBuffer() creates a new TXBuffer for the given DrbKey and flow info.
     *
     * @param id DrbKey for the new buffer
     * @param lteInfo flow-related info
     * @return pointer to the newly created TXBuffer
     */
    UmTxEntity *createTxBuffer(DrbKey id, FlowControlInfo *lteInfo);


    /**
     * lookupRxBuffer() searches for an existing RXBuffer for the given DrbKey.
     *
     * @param id DrbKey to lookup
     * @return pointer to the RXBuffer if found, nullptr otherwise
     */
    UmRxEntity *lookupRxBuffer(DrbKey id);

    /**
     * createRxBuffer() creates a new RXBuffer for the given DrbKey and flow info.
     *
     * @param id DrbKey for the new buffer
     * @param lteInfo flow-related info
     * @return pointer to the newly created RXBuffer
     */
    UmRxEntity *createRxBuffer(DrbKey id, FlowControlInfo *lteInfo);

};

} //namespace

#endif

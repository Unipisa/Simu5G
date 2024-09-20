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

#ifndef _LTE_LTERLCUMD2D_H_
#define _LTE_LTERLCUMD2D_H_

#include "common/utils/utils.h"
#include "stack/rlc/um/LteRlcUm.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class LteRlcUmD2D
 * @brief UM Module
 *
 * This is the UM Module of RLC (with support for D2D)
 *
 */
class LteRlcUmD2D : public LteRlcUm
{
  public:

    void resumeDownstreamInPackets(MacNodeId peerId) override;
    bool isEmptyingTxBuffer(MacNodeId peerId) override;

  protected:

    RanNodeType nodeType_;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * getTxBuffer() is used by the sender to gather the TXBuffer
     * for that CID. If TXBuffer was already present, a reference
     * is returned, otherwise a new TXBuffer is created,
     * added to the tx_buffers map and a reference is returned as well.
     *
     * @param lteInfo flow-related info
     * @return pointer to the TXBuffer for the CID of the flow
     *
     */
    UmTxEntity *getTxBuffer(inet::Ptr<FlowControlInfo> lteInfo) override;

    /**
     * UM Mode
     *
     * handler for traffic coming from
     * lower layer (DTCH, MTCH, MCCH).
     *
     * handleLowerMessage() performs the following tasks:
     *
     * - Search (or add) the proper RXBuffer, depending
     *   on the packet CID
     * - Calls the RXBuffer, which from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    void handleLowerMessage(cPacket *pkt) override;

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    void deleteQueues(MacNodeId nodeId) override;

  private:

    std::map<MacNodeId, std::set<UmTxEntity *, simu5g::utils::cModule_LessId>> perPeerTxEntities_;
};

} //namespace

#endif


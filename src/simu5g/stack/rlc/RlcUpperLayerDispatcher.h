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

#ifndef _LTE_RLCUPPERLAYERDISPATCHER_H_
#define _LTE_RLCUPPERLAYERDISPATCHER_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

using namespace omnetpp;

/**
 * \class RlcUpperLayerDispatcher
 * \brief Upper Layer Dispatcher for RLC
 *
 * This module functions as a (de)multiplexer between the upper layer (PDCP)
 * and the submodules of the LteRlc module (TM, UM, AM):
 * - Traffic coming from PDCP is dispatched to the appropriate RLC mode
 *   based on the RlcType field in the FlowControlInfo tag
 * - Traffic coming from TM/UM/AM modules is forwarded to PDCP
 *
 */
class RlcUpperLayerDispatcher : public cSimpleModule
{
  protected:

    /**
     * Initialize class structures
     */
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

  private:
    /*
     * Data structures
     */

    cGate *upperLayerInGate_ = nullptr;
    cGate *upperLayerOutGate_ = nullptr;
    cGate *tmSapInGate_ = nullptr;
    cGate *tmSapOutGate_ = nullptr;
    cGate *umSapInGate_ = nullptr;
    cGate *umSapOutGate_ = nullptr;
    cGate *amSapInGate_ = nullptr;
    cGate *amSapOutGate_ = nullptr;

    /*
     * Upper Layer Handler
     */

    /**
     * Handler for packets from PDCP - dispatch to appropriate RLC mode
     *
     * @param pkt packet to process
     */
    void fromUpperLayer(cPacket *pkt);

    /*
     * Lower Layer Handler
     */

    /**
     * Handler for packets from TM/UM/AM - forward to PDCP
     *
     * @param pkt packet to process
     */
    void toUpperLayer(cPacket *pkt);
};

} //namespace

#endif

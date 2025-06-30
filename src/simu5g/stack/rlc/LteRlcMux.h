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

#ifndef _LTE_LTERLCMUX_H_
#define _LTE_LTERLCMUX_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

namespace simu5g {

using namespace omnetpp;

/**
 * \class LteRLC
 * \brief RLC Layer
 *
 * This is the Muxer of the RLC layer of the LTE Stack.
 * It has the function of multiplexing/demultiplexing traffic:
 * - Traffic coming from TM/UM/AM modules is decoded
 *   and forwarded to MAC layer ports
 * - Traffic coming from MAC layer is decoded
 *   and forwarded to TM/UM/AM modules
 *
 */
class LteRlcMux : public cSimpleModule
{
  protected:

    /**
     * Initialize class structures
     * gate map and delay
     */
    void initialize() override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    void handleMessage(cMessage *msg) override;

    /**
     * Statistics recording
     */
    void finish() override;

  private:
    /*
     * Upper Layer Handler
     */

    /**
     * Handler for rlc2mac packets
     *
     * @param pkt packet to process
     */
    void rlc2mac(cPacket *pkt);

    /*
     * Lower Layer Handler
     */

    /**
     * Handler for mac2rlc packets
     *
     * @param pkt packet to process
     */
    void mac2rlc(cPacket *pkt);

    /*
     * Data structures
     */

    cGate *macSapInGate_ = nullptr;
    cGate *macSapOutGate_ = nullptr;
    cGate *tmSapInGate_ = nullptr;
    cGate *tmSapOutGate_ = nullptr;
    cGate *umSapInGate_ = nullptr;
    cGate *umSapOutGate_ = nullptr;
    cGate *amSapInGate_ = nullptr;
    cGate *amSapOutGate_ = nullptr;
};

} //namespace

#endif


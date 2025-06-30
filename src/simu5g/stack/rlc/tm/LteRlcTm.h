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

#ifndef _LTE_LTERLCTM_H_
#define _LTE_LTERLCTM_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "stack/rlc/LteRlcDefs.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class LteRlcTm
 * @brief TM Module
 *
 * This is the TM Module of RLC.
 * It implements the transparent mode (TM):
 *
 * - Transparent mode (TM):
 *   This mode is only used for control traffic, and
 *   the corresponding port is therefore accessed only from
 *   the RRC layer. Traffic on this port is simply forwarded
 *   to lower layers on ports BCCH/PCCH/CCCH
 *
 *   TM mode does not attach any header to the packet.
 *
 */
class LteRlcTm : public cSimpleModule
{
  protected:
    /**
     * Analyzes gate of incoming packet
     * and calls proper handler
     */
    void handleMessage(cMessage *msg) override;

    void initialize() override;
    void finish() override {}

  private:
    /**
     * Handler for traffic coming
     * from the upper layer (RRC)
     *
     * handleUpperMessage() simply forwards packets to lower layers.
     * An empty header is added so that the encapsulation
     * level is the same for all packets traversing the stack
     *
     * @param pkt packet to process
     */
    void handleUpperMessage(cPacket *pkt);

    /**
     * Handler for traffic coming from
     * lower layer
     *
     * handleLowerMessage() is the function that takes care
     * of TM traffic coming from lower layer.
     * After decapsulating the fictitious
     * header, the packet is simply forwarded to the upper layer
     *
     * @param pkt packet to process
     */
    void handleLowerMessage(cPacket *pkt);

    /**
     * Data structures
     */

    cGate *upInGate_ = nullptr;
    cGate *upOutGate_ = nullptr;
    cGate *downInGate_ = nullptr;
    cGate *downOutGate_ = nullptr;

    /*
     * Queue for storing PDUs to be delivered to MAC when LteMacSduRequest is received
     */
    inet::cPacketQueue queuedPdus_;

    /*
     * The maximum available queue size (in bytes)
     * (amount of data in sduQueue_ must not exceed this value)
     */
    int queueSize_;

    // statistics
    static inet::simsignal_t receivedPacketFromUpperLayerSignal_;
    static inet::simsignal_t receivedPacketFromLowerLayerSignal_;
    static inet::simsignal_t sentPacketToUpperLayerSignal_;
    static inet::simsignal_t sentPacketToLowerLayerSignal_;
    static inet::simsignal_t rlcPacketLossDlSignal_;
    static inet::simsignal_t rlcPacketLossUlSignal_;
};

} //namespace

#endif


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
 *   to lower layer on ports BCCH/PCCH/CCCH
 *
 *   TM mode does not attach any header to the packet.
 *
 */
class LteRlcTm : public omnetpp::cSimpleModule
{
  public:
    LteRlcTm()
    {
    }
    virtual ~LteRlcTm()
    {
    }

  protected:
    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void initialize() override;
    virtual void finish() override
    {
    }

  private:
    /**
     * handler for traffic coming
     * from the upper layer (RRC)
     *
     * handleUpperMessage() simply forwards packet to lower layers.
     * An empty header is added so that the encapsulation
     * level is the same for all packets transversing the stack
     *
     * @param pkt packet to process
     */
    void handleUpperMessage(omnetpp::cPacket *pkt);

    /**
     * handler for traffic coming from
     * lower layer
     *
     * handleLowerMessage() is the function that takes care
     * of TM traffic coming from lower layer.
     * After decapsulating the fictitious
     * header, packet is simply forwarded to the upper layer
     *
     * @param pkt packet to process
     */
    void handleLowerMessage(omnetpp::cPacket *pkt);

    /**
     * Data structures
     */

    omnetpp::cGate* up_[2];
    omnetpp::cGate* down_[2];


    /*
     * Queue for storing PDUs to be delivered to MAC when LteMacSduRequest is received
     */
    inet::cPacketQueue queuedPdus_;

    /*
     * The maximum available queue size (in bytes)
     * (amount of data in sduQueue_ must not exceed this value)
     */
    unsigned int queueSize_;

    // statistics
    inet::simsignal_t receivedPacketFromUpperLayer;
    inet::simsignal_t receivedPacketFromLowerLayer;
    inet::simsignal_t sentPacketToUpperLayer;
    inet::simsignal_t sentPacketToLowerLayer;
    inet::simsignal_t rlcPacketLossDl;
    inet::simsignal_t rlcPacketLossUl;
};

#endif

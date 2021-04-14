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

#ifndef _LTE_LTEHARQUNITTXD2D_H_
#define _LTE_LTEHARQUNITTXD2D_H_

#include "stack/mac/buffer/harq/LteHarqUnitTx.h"
#include "stack/mac/layer/LteMacUeD2D.h"
#include "stack/mac/layer/LteMacEnbD2D.h"

/**
 * An LteHarqUnit is an HARQ mac pdu container,
 * an harqBuffer is made of harq processes which is made of harq units.
 *
 * LteHarqUnit manages transmissions and retransmissions.
 * Contained PDU may be in one of four status:
 *
 *                            IDLE       PDU                    READY
 * TXHARQ_PDU_BUFFERED:        no        present locally        ready for rtx
 * TXHARQ_PDU_WAITING:         no        copy present           not ready for tx
 * TXHARQ_PDU_EMPTY:           yes       not present            not ready for tx
 * TXHARQ_PDU_SELECTED:        no        present                will be tx
 */
class LteHarqUnitTxD2D : public LteHarqUnitTx
{
  protected:

    // D2D Statistics
      omnetpp::simsignal_t macCellPacketLossD2D_;
      omnetpp::simsignal_t macPacketLossD2D_;
      omnetpp::simsignal_t harqErrorRateD2D_;
      omnetpp::simsignal_t harqErrorRateD2D_1_;
      omnetpp::simsignal_t harqErrorRateD2D_2_;
      omnetpp::simsignal_t harqErrorRateD2D_3_;
      omnetpp::simsignal_t harqErrorRateD2D_4_;

  public:
    /**
     * Constructor.
     *
     * @param id unit identifier
     */
    LteHarqUnitTxD2D(unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac);

    /**
     * Manages ACK/NACK.
     *
     * @param fb ACK or NACK for this H-ARQ unit
     * @return true if the unit has become empty, false if it is still busy
     */
    virtual bool pduFeedback(HarqAcknowledgment fb);

    /**
     * Returns the macPdu to be sent and increments transmissions_ counter.
     *
     * The H-ARQ process containing this unit, must call this method in order
     * to extract the pdu the Mac layer will send.
     * Before extraction, control info is updated with transmission counter and ndi.
     */
    virtual inet::Packet *extractPdu();

    virtual ~LteHarqUnitTxD2D();
};

#endif

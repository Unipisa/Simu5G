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
#include "stack/mac/LteMacUeD2D.h"
#include "stack/mac/LteMacEnbD2D.h"

namespace simu5g {

using namespace omnetpp;

/**
 * An LteHarqUnit is an HARQ mac pdu container,
 * a harqBuffer is made of harq processes which are made of harq units.
 *
 * LteHarqUnit manages transmissions and retransmissions.
 * Contained PDU may be in one of four statuses:
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
    static simsignal_t macCellPacketLossD2DSignal_;
    static simsignal_t macPacketLossD2DSignal_;
    static simsignal_t harqErrorRateD2DSignal_;
    static simsignal_t harqErrorRateD2D_1Signal_;
    static simsignal_t harqErrorRateD2D_2Signal_;
    static simsignal_t harqErrorRateD2D_3Signal_;
    static simsignal_t harqErrorRateD2D_4Signal_;

  public:
    /**
     * Constructor.
     *
     * @param id unit identifier
     */
    LteHarqUnitTxD2D(Binder *binder, unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac);

    /**
     * Manages ACK/NACK.
     *
     * @param fb ACK or NACK for this H-ARQ unit
     * @return true if the unit has become empty, false if it is still busy
     */
    bool pduFeedback(HarqAcknowledgment fb) override;

    /**
     * Returns the macPdu to be sent and increments transmissions_ counter.
     *
     * The H-ARQ process containing this unit must call this method in order
     * to extract the PDU the Mac layer will send.
     * Before extraction, control info is updated with transmission counter and ndi.
     */
    inet::Packet *extractPdu() override;

};

} //namespace

#endif


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

#ifndef _LTE_LTEHARQPROCESSRXD2D_H_
#define _LTE_LTEHARQPROCESSRXD2D_H_

#include "stack/mac/buffer/harq/LteHarqProcessRx.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"

namespace simu5g {

/**
 * H-ARQ RX processes contain PDUs received from the PHY layer for which
 * H-ARQ feedback must be sent.
 * These PDUs must be evaluated using H-ARQ correction functions.
 * H-ARQ RX process state machine has three states:
 * RXHARQ_PDU_EMPTY
 * RXHARQ_PDU_EVALUATING
 * RXHARQ_PDU_CORRECT
 */
class LteHarqProcessRxD2D : public LteHarqProcessRx
{
  public:

    /**
     * Constructor.
     *
     * @param acid process identifier
     * @param
     */
    LteHarqProcessRxD2D(unsigned char acid, LteMacBase *owner, Binder *binder);

    /**
     * Creates a feedback message based on the evaluation result for this PDU.
     *
     * @return feedback message to be sent.
     */
    inet::Packet *createFeedback(Codeword cw) override;

    /**
     * Creates a feedback message based on the evaluation result for this PDU.
     * This is the feedback sent to the eNB
     * @return feedback message to be sent.
     */
    virtual inet::Packet *createFeedbackMirror(Codeword cw);

};

} //namespace

#endif


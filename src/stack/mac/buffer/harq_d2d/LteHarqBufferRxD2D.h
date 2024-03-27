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

#ifndef _LTE_LTEHARQBUFFERRXD2D_H_
#define _LTE_LTEHARQBUFFERRXD2D_H_

#include "inet/common/packet/Packet.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqProcessRxD2D.h"


class LteHarqProcessRxD2D;


/**
 * H-ARQ RX buffer for D2D: messages coming from phy are stored in H-ARQ RX buffer.
 * When a new pdu is inserted, it is in EVALUATING state meaning that the hardware is
 * checking its correctness.
 * A feedback is sent after the pdu has been evaluated (HARQ_FB_EVALUATION_INTERVAL), and
 * in case of ACK the pdu moves to CORRECT state, else it is dropped from the process.
 * The operations of checking if a pdu is ready for feedback and if it is in correct state are
 * done in the extractCorrectPdu mehtod which must be called at every tti (it must be part
 * of the mac main loop).
 */
class LteHarqBufferRxD2D : public LteHarqBufferRx
{
  protected:

        // D2D Statistics
        inet::simsignal_t macDelayD2D_;
        inet::simsignal_t macCellThroughputD2D_;
        inet::simsignal_t macThroughputD2D_;

    /**
     * Checks for all processes if the pdu has been evaluated and sends
     * feedback if affirmative.
     */
    virtual void sendFeedback();

  public:
    LteHarqBufferRxD2D(unsigned int num, LteMacBase *owner, MacNodeId nodeId, bool isMulticast=false);

    /*
     * Insertion of a new pdu coming from phy layer into
     * RX H-ARQ buffer.
     *
     * @param pdu to be inserted
     */
    virtual void insertPdu(Codeword cw, inet::Packet *pdu);

    /**
     * Sends feedback for all processes which are older than
     * HARQ_FB_EVALUATION_INTERVAL, then extract the pdu in correct state (if any)
     *
     * @return uncorrupted pdus or empty list if none
     */
    virtual std::list<inet::Packet*> extractCorrectPdus();

    virtual ~LteHarqBufferRxD2D();
};

#endif

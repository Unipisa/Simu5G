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

#ifndef _LTE_LTEHARQPROCESSRX_H_
#define _LTE_LTEHARQPROCESSRX_H_

#include <omnetpp.h>

#include "common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

typedef std::pair<unsigned char, RxHarqPduStatus> RxUnitStatus;
typedef std::vector<std::vector<RxUnitStatus>> RxBufferStatus;

class LteMacBase;
class LteMacPdu;
class LteHarqFeedback;
class Binder;

/**
 * H-ARQ RX processes contain PDUs received from the PHY layer for which
 * H-ARQ feedback must be sent.
 * These PDUs must be evaluated using H-ARQ correction functions.
 * The H-ARQ RX process state machine has three states:
 * RXHARQ_PDU_EMPTY
 * RXHARQ_PDU_EVALUATING
 * RXHARQ_PDU_CORRECT
 */
// TODO: check the number of UL/DL processes, UE, ENB
// TODO: statistics for drops (add member?)
class LteHarqProcessRx
{
  protected:
    /// contained PDUs
    //std::vector<LteMacPdu *> pdu_;
    std::vector<inet::Packet *> pdu_;

    /// current status for each codeword
    std::vector<RxHarqPduStatus> status_;

    /// reception time timestamp
    std::vector<inet::simtime_t> rxTime_;

    // reception status of buffered PDUs
    std::vector<bool> result_;

    /// H-ARQ process identifier
    unsigned char acid_;

    /// MAC module to manage errors (endSimulation)
    opp_component_ptr<LteMacBase> macOwner_;

    /// reference to the binder
    opp_component_ptr<Binder> binder_;

    /// Number of (re)transmissions for current PDU (N.B.: values are 1,2,3,4)
    unsigned char transmissions_ = 0;

    unsigned char maxHarqRtx_;

    /// Number of slots for sending back HARQ Feedback
    unsigned short harqFbEvaluationTimer_;

  public:

    /**
     * Constructor.
     *
     * @param acid process identifier
     * @param
     */
    LteHarqProcessRx(unsigned char acid, LteMacBase *owner, Binder *binder);

    /**
     * Inserts a PDU into the process and evaluates it (corrupted or correct).
     *
     * @param pdu PDU to be inserted
     */
    virtual void insertPdu(Codeword cw, inet::Packet *);

    /**
     * Tells if contained PDUs have been evaluated and feedback responses can be
     * sent.
     *
     * A PDU is evaluated if 4ms has elapsed since it was put into the process.
     *
     * @return true if the PDU can be acknowledged/nacked, false if the evaluation is still in process.
     */
    virtual bool isEvaluated(Codeword cw);

    /**
     * Creates a feedback message based on the evaluation result for this PDU.
     *
     * @return feedback message to be sent.
     */
    //virtual LteHarqFeedback *createFeedback(Codeword cw);
    virtual inet::Packet *createFeedback(Codeword cw);

    /**
     * Tells if a PDU is in correct state (not corrupted, extractable).
     *
     * @return true if correct, false otherwise
     */
    virtual bool isCorrect(Codeword cw);

    /**
     * Returns process/codeword PDU size (bytes)  [only for buffered status] .
     *
     * @return size if PDU is present, 0 otherwise
     */
    virtual int64_t getByteLength(Codeword cw);

    /**
     * @return the codeword status
     */
    virtual RxHarqPduStatus getUnitStatus(Codeword cw)
    {
        return status_.at(cw);
    }

    /**
     * @return whole buffer status
     */

    virtual std::vector<RxUnitStatus> getProcessStatus();
    /**
     * Extracts a PDU that can be passed to the MAC layer and reset process status.
     *
     * @return PDU ready for upper layer
     */
    virtual inet::Packet *extractPdu(Codeword cw);

    /**
     * Purges a corrupted PDU that has been received on codeword <cw>
     */
    void purgeCorruptedPdu(Codeword cw);

    /**
     * Resets the status of a codeword containing a corrupted PDU
     */
    virtual void resetCodeword(Codeword cw);

    /**
     * @return number of codewords available for this process (set to MAX_CODEWORDS by default)
     */
    virtual unsigned int getNumHarqUnits()
    {
        return MAX_CODEWORDS;
    }

    /**
     * Tells if the process is empty: all of its units are in EMPTY state.
     *
     * @return true if the process is empty, false if it isn't
     */
    bool isEmpty();

    /**
     * Returns a list of ids of empty units inside this process.
     *
     * @return empty units ids list.
     */
    CwList emptyUnitsIds();

    /**
     * @author Alessandro Noferi
     *
     * Check if the process is active
     *
     * @return true if at least one unit status is not RXHARQ_PDU_EMPTY
     */

    bool isHarqProcessActive();

    virtual ~LteHarqProcessRx();
};

} //namespace

#endif


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

#ifndef _LTE_LTEHARQUNITTX_H_
#define _LTE_LTEHARQUNITTX_H_

#include <omnetpp.h>

#include "stack/mac/packet/LteMacPdu.h"
#include "common/LteControlInfo.h"
#include "common/LteCommon.h"
#include "stack/mac/LteMacBase.h"

namespace simu5g {

using namespace omnetpp;

class LteMacBase;

/**
 * An LteHarqUnit is an HARQ mac pdu container,
 * a harqBuffer is made of harq processes which are made of harq units.
 *
 * LteHarqUnit manages transmissions and retransmissions.
 * Contained PDU may be in one of four statuses:
 *
 *                            IDLE    PDU                    READY
 * TXHARQ_PDU_BUFFERED:       no      present locally        ready for rtx
 * TXHARQ_PDU_WAITING:        no      copy present           not ready for tx
 * TXHARQ_PDU_EMPTY:          yes     not present            not ready for tx
 * TXHARQ_PDU_SELECTED:       no      present                will be tx
 */
class LteHarqUnitTx : noncopyable
{
  protected:

    /// Carried sub-burst
    Packet *pdu_ = nullptr;

    /// Omnet ID of the pdu
    long pduId_ = -1;

    /// PDU size in bytes
    int64_t pduLength_;

    // H-ARQ process identifier
    unsigned char acid_;

    /// H-ARQ codeword identifier
    Codeword cw_;

    /// Number of (re)transmissions for current pdu (N.B.: values are 1,2,3,4)
    unsigned char transmissions_ = 0;

    TxHarqPduStatus status_ = TXHARQ_PDU_EMPTY;

    /// TTI at which the pdu has been transmitted
    simtime_t txTime_;

    // reference to the eNB module
    opp_component_ptr<cModule> nodeB_;

    opp_component_ptr<LteMacBase> macOwner_;
    //used for statistics
    opp_component_ptr<LteMacBase> dstMac_;
    //Maximum number of H-ARQ retransmissions
    unsigned int maxHarqRtx_;

    // Statistics

    Direction dir_ = UNKNOWN_DIRECTION;

    static simsignal_t macCellPacketLossSignal_[2];
    static simsignal_t macPacketLossSignal_[2];
    static simsignal_t harqErrorRateSignal_[2];
    static simsignal_t harqErrorRate_1Signal_[2];
    static simsignal_t harqErrorRate_2Signal_[2];
    static simsignal_t harqErrorRate_3Signal_[2];
    static simsignal_t harqErrorRate_4Signal_[2];
    static simsignal_t harqTxAttemptsSignal_[2];

  public:
    /**
     * Constructor.
     *
     * @param id unit identifier
     */
    LteHarqUnitTx(Binder *binder, unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac);

    /**
     * Inserts a pdu in this harq unit.
     *
     * When a new pdu is inserted into an H-ARQ unit, its status is TX_HARQ_PDU_SELECTED,
     * so it will be extracted and sent at this same TTI.
     *
     * @param pdu MacPdu to be inserted
     */
    virtual void insertPdu(Packet *pdu);

    /**
     * Transition from BUFFERED to SELECTED status: the pdu will be extracted when the
     * buffer is inspected.
     */
    virtual void markSelected();

    /**
     * Returns the macPdu to be sent and increments transmissions_ counter.
     *
     * The H-ARQ process containing this unit must call this method in order
     * to extract the pdu the Mac layer will send.
     * Before extraction, control info is updated with transmission counter and ndi.
     */
    virtual Packet *extractPdu();

    /**
     * Manages ACK/NACK.
     *
     * @param fb ACK or NACK for this H-ARQ unit
     * @return true if the unit has become empty, false if it is still busy
     */
    virtual bool pduFeedback(HarqAcknowledgment fb);

    /**
     * Tells if this unit is currently managing a pdu or not.
     */
    virtual bool isEmpty();

    /**
     * Tells if the PDU is ready for retransmission (the pdu can then be marked to be sent)
     */
    virtual bool isReady();

    /**
     * If, after evaluating the pdu, it cannot be retransmitted because there isn't
     * enough frame space, a selfNack can be issued to advance the unit status.
     * This avoids a large pdu that cannot be retransmitted (because the channel changed),
     * from locking an H-ARQ unit indefinitely.
     * Must simulate selection, extraction and nack reception.
     * N.B.: txTime is also updated so firstReadyForRtx returns a different pdu
     *
     * @result true if the unit reset as a result of self nack, false otherwise
     */
    virtual bool selfNack();

    /**
     * Resets unit but throws an error if the unit is not in
     * BUFFERED state.
     */
    virtual void dropPdu();

    virtual void forceDropUnit();

    virtual Packet *getPdu();

    virtual unsigned char getAcid()
    {
        return acid_;
    }

    virtual Codeword getCodeword()
    {
        return cw_;
    }

    virtual unsigned char getTransmissions()
    {
        return transmissions_;
    }

    virtual int64_t getPduLength()
    {
        return pduLength_;
    }

    virtual simtime_t getTxTime()
    {
        return txTime_;
    }

    virtual bool isMarked()
    {
        return status_ == TXHARQ_PDU_SELECTED;
    }

    virtual long getMacPduId()
    {
        return pduId_;
    }

    virtual TxHarqPduStatus getStatus()
    {
        return status_;
    }

    virtual ~LteHarqUnitTx();

  protected:

    virtual void resetUnit();
};

} //namespace

#endif


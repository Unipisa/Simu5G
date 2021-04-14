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
#include "stack/mac/layer/LteMacBase.h"

class LteMacBase;

/**
 * An LteHarqUnit is an HARQ mac pdu container,
 * an harqBuffer is made of harq processes which is made of harq units.
 *
 * LteHarqUnit manages transmissions and retransmissions.
 * Contained PDU may be in one of four status:
 *
 *                            IDLE    PDU                    READY
 * TXHARQ_PDU_BUFFERED:       no      present locally        ready for rtx
 * TXHARQ_PDU_WAITING:        no      copy present           not ready for tx
 * TXHARQ_PDU_EMPTY:          yes     not present            not ready for tx
 * TXHARQ_PDU_SELECTED:       no      present                will be tx
 */
class LteHarqUnitTx
{
  protected:

    /// Carried sub-burst
    Packet *pdu_;

    /// Omnet ID of the pdu
    long pduId_;

    /// PDU size in bytes
    inet::int64 pduLength_;

    // H-ARQ process identifier
    unsigned char acid_;

    /// H-ARQ codeword identifier
    Codeword cw_;

    /// Number of (re)transmissions for current pdu (N.B.: values are 1,2,3,4)
    unsigned char transmissions_;

    TxHarqPduStatus status_;

    /// TTI at which the pdu has been transmitted
    omnetpp::simtime_t txTime_;

    // reference to the eNB module
    omnetpp::cModule* nodeB_;

    LteMacBase *macOwner_;
    //used for statistics
    LteMacBase *dstMac_;
    //Maximum number of H-ARQ retransmission
    unsigned int maxHarqRtx_;

    // Statistics


    omnetpp::simsignal_t macCellPacketLoss_;
    omnetpp::simsignal_t macPacketLoss_;
    omnetpp::simsignal_t harqErrorRate_;
    omnetpp::simsignal_t harqErrorRate_1_;
    omnetpp::simsignal_t harqErrorRate_2_;
    omnetpp::simsignal_t harqErrorRate_3_;
    omnetpp::simsignal_t harqErrorRate_4_;
    omnetpp::simsignal_t harqTxAttempts_;

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
    LteHarqUnitTx(unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac);

    /**
     * Copy constructor and operator=
     */
    LteHarqUnitTx(const LteHarqUnitTx& other)
    {
        operator=(other);
    }
    LteHarqUnitTx& operator=(const LteHarqUnitTx& other);

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
     * buffer will be inspected.
     */
    virtual void markSelected();

    /**
     * Returns the macPdu to be sent and increments transmissions_ counter.
     *
     * The H-ARQ process containing this unit, must call this method in order
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
     * This avoids a big pdu that cannot be retransmitted (because the channel changed),
     * to lock an H-ARQ unit indefinitely.
     * Must simulate selection, extraction and nack reception.
     * N.B.: txTime is also updated so firstReadyForRtx returns a different pdu
     *
     * @result true if the unit reset as effect of self nack, false otherwise
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

    virtual inet::int64 getPduLength()
    {
        return pduLength_;
    }

    virtual omnetpp::simtime_t getTxTime()
    {
        return txTime_;
    }

    virtual bool isMarked()
    {
        return (status_ == TXHARQ_PDU_SELECTED);
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

#endif

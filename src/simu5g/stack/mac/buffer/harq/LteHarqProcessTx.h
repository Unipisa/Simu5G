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

#ifndef _LTE_LTEHARQPROCESSTX_H_
#define _LTE_LTEHARQPROCESSTX_H_

#include "stack/mac/buffer/harq/LteHarqUnitTx.h"
#include "stack/mac/LteMacBase.h"

namespace simu5g {

using namespace omnetpp;

typedef std::vector<LteHarqUnitTx *> UnitVector;
typedef std::pair<unsigned char, TxHarqPduStatus> UnitStatus;
typedef std::vector<std::vector<UnitStatus>> BufferStatus;

/**
 * Container of H-ARQ units.
 * An H-ARQ process contains the units with id (acid + totalNumberOfProcesses * cw[i]), for each i.
 *
 * An H-ARQ buffer is made of H-ARQ processes.
 * An H-ARQ process is atomic for transmission, while H-ARQ units are atomic for
 * H-ARQ feedback.
 */

class LteHarqProcessTx : noncopyable
{
  protected:

    /// reference to mac module, used to handle errors
    opp_component_ptr<LteMacBase> macOwner_;

    /// contained units vector
    UnitVector units_;

    /// total number of processes in this H-ARQ buffer
    unsigned int numProcesses_;

    /// number of contained H-ARQ units
    unsigned char numHarqUnits_;

    /// H-ARQ process identifier
    unsigned char acid_;

    /// Number of empty units inside this process
    unsigned char numEmptyUnits_; //++ @ insert, -- @ unit reset (ack or fourth nack)

    /// Number of selected units inside this process
    unsigned int numSelected_; //++ @ markSelected and insert, -- @ extract/sendDown

    /// Set this flag when a handover or a D2D switch occurs, so that the HARQ process was interrupted.
    /// This is useful in case the process receives a feedback after reset.
    bool dropped_;

  public:

    /*
     * Default Constructor
     */
    LteHarqProcessTx() {}

    /**
     * Creates a new H-ARQ process, which is a container of H-ARQ units.
     *
     * @param acid H-ARQ process identifier
     * @param numUnits number of units contained in this process (MAX_CODEWORDS)
     * @param numProcesses number of processes contained in the H-ARQ buffer.
     * @return
     */
    LteHarqProcessTx(Binder *binder, unsigned char acid, unsigned int numUnits, unsigned int numProcesses, LteMacBase *macOwner,
            LteMacBase *dstMac);

    /**
     * Insert a PDU into an H-ARQ unit contained in this process.
     *
     * @param pdu PDU to be inserted
     * @param unitId id of destination unit
     */
    void insertPdu(inet::Packet *pdu, Codeword cw);

    void markSelected(Codeword cw);

    virtual inet::Packet *extractPdu(Codeword cw);

    bool pduFeedback(HarqAcknowledgment fb, Codeword cw);

    bool selfNack(Codeword cw);

    /**
     *
     *
     * @returns
     */
    std::vector<UnitStatus> getProcessStatus();

    /**
     * Checks if this process has one or more H-ARQ units ready for retransmission.
     *
     * @return true if there is at least one unit ready for RTX, false if none
     */
    bool hasReadyUnits();

    /**
     * Returns the TX time of the unit which is not retransmitting for
     * the longest period of time, inside this process (the oldest
     * PDU TX time).
     *
     * @return TX time of the oldest unit in this process
     */
    simtime_t getOldestUnitTxTime();

    /**
     * Returns a list of IDs of ready for retransmission units of
     * this process.
     *
     * @return list of unit IDs which are ready for RTX
     */
    CwList readyUnitsIds();

    /**
     * Returns a list of IDs of empty units inside this process.
     *
     * @return empty units IDs list.
     */
    CwList emptyUnitsIds();

    /**
     * Returns a list of IDs of selected units.
     *
     * @return selected units IDs list.
     */
    CwList selectedUnitsIds();

    /**
     * Tells if the process is empty: all of its units are in EMPTY state.
     *
     * @return true if the process is empty, false if it isn't
     */
    bool isEmpty();

    inet::Packet *getPdu(Codeword cw);

    /**
     * This is necessary because when a PDU is in CORRECT state at the
     * corresponding H-ARQ RX process, it may be extracted and potentially
     * deleted, so PDU reference cannot be used to retrieve the PDU ID
     * when feedback is received, because feedback is received 1 TTI after
     * PDU extraction at the RX process!
     *
     * @param unitId unit ID
     * @return PDU ID
     */
    long getPduId(Codeword cw);

    void forceDropProcess();

    bool forceDropUnit(Codeword cw);

    unsigned int getNumHarqUnits()
    {
        return numHarqUnits_;
    }

    TxHarqPduStatus getUnitStatus(Codeword cw);

    void dropPdu(Codeword cw);
    bool isUnitEmpty(Codeword cw);
    bool isUnitReady(Codeword cw);
    unsigned char getTransmissions(Codeword cw);
    int64_t getPduLength(Codeword cw);
    simtime_t getTxTime(Codeword cw);
    bool isUnitMarked(Codeword cw);
    bool isDropped();

    /**
     * @author Alessandro Noferi
     *
     * Check if the process is active
     *
     * @return true if at least one unit status is not TXHARQ_PDU_EMPTY
     */

    bool isHarqProcessActive();

    virtual ~LteHarqProcessTx();
};

} //namespace

#endif


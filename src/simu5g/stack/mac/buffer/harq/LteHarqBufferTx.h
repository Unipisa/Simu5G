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

#ifndef _LTE_LTEHARQBUFFERTX_H_
#define _LTE_LTEHARQBUFFERTX_H_

#include <vector>
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/buffer/harq/LteHarqProcessTx.h"
#include "stack/mac/LteMacBase.h"

namespace simu5g {

using namespace omnetpp;

/*
 * NOTE: it is the UL MAC's task to only use the current process; there is no control.
 * TODO: add support for uplink: functions that specify the process to use
 * TODO: comments
 */

class LteHarqBufferTx : noncopyable
{
  protected:
    opp_component_ptr<LteMacBase> macOwner_;
    std::vector<LteHarqProcessTx *> processes_;
    unsigned int numProc_;
    unsigned int numEmptyProc_; // @ fb on reset, @ insert
    unsigned char selectedAcid_; // @ insert, @ marksel, @ sendseldn
    MacNodeId nodeId_; // UE nodeId for which this buffer has been created

  protected:
    /**
     * Protected Base Constructor.
     *
     * @param binder Binder module.
     * @param numProc number of H-ARQ processes in the buffer.
     * @param owner simple module instantiating an H-ARQ TX buffer
     */
    LteHarqBufferTx(Binder *binder, unsigned int numProc, LteMacBase *owner);

  public:
    /**
     * Constructor.
     *
     * @param numProc number of H-ARQ processes in the buffer.
     * @param owner simple module instantiating an H-ARQ TX buffer
     * @param nodeId UE nodeId for which this buffer has been created
     */
    LteHarqBufferTx(Binder *binder, unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac);

    /*
     * Get a reference to the specified process
     */
    LteHarqProcessTx *getProcess(unsigned char acid);

    /*
     * Get a reference to the selected process
     */
    LteHarqProcessTx *getSelectedProcess();

    /**
     * Finds the H-ARQ process which contains the PDU that is not
     * transmitting for the longest period of time and returns
     * a list of unitIds of the units that belong to this same
     * process which are in ready state (the ids of the units which will
     * be retransmitted).
     * N.B.: inside an H-ARQ process containing ready units, there will
     * be only units ready for RTX or empty units, because H-ARQ feedback
     * is always received simultaneously for all the units of the same
     * process (a process is atomic for transmission).
     *
     * @return list of unit ids to be retransmitted
     */
    UnitList firstReadyForRtx();

    /**
     * Returns the identifier of the H-ARQ process containing the unit with
     * passed id.
     * @param unitId
     * @return PDU length in bytes
     */
    int64_t pduLength(unsigned char acid, Codeword cw);

    /**
     * Takes a list of ids of the units that must retransmit and the number
     * of available TBs for the user (which depends on the txmode), and
     * prepares, whenever possible, the PDUs for retransmission.
     * If the process has more ready units than unitIds list size,
     * then excluded units' PDUs will be dropped (not all units of
     * the process have been selected for retransmission).
     * If more units have been selected for retransmission than the number
     * of available TBs, then a single PDU containing all of the SDUs will
     * be created and selected for retransmission. All the other units
     * will be reset.
     * If the number of available TBs is sufficient, then simply all the
     * units whose ids are in the unitIds list are selected for retransmission.
     *
     * @param unitIds list of ids of the units that must retransmit.
     * @param availableTbs number of available TBs for the user.
     */
    void markSelected(UnitList unitIds, unsigned char availableTbs);

    /**
     * Inserts a PDU in the unit with specified id.
     * A newly inserted PDU automatically marks the process as selected.
     *
     * @param unitId id of the destination unit
     * @param pdu PDU to be inserted
     */
    void insertPdu(unsigned char acid, Codeword cw, Packet *pdu);

    /**
     * Returns a pair with H-ARQ process id and a list of its empty units to be used for transmission.
     * If a process has already been selected (e.g. for RTX), then the same
     * process must be used for transmission; otherwise, the unit IDs of an
     * empty process are returned.
     *
     * @return  a list of acid and their units  to be used for transmission
     */
    UnitList firstAvailable();

    /*
     * Returns a pair with the specified H-ARQ process id and a list of its empty units to be used for transmission.
     */
    UnitList getEmptyUnits(unsigned char acid);

    /**
     * Manages H-ARQ feedback sent to a certain H-ARQ unit and checks if
     * the corresponding process becomes empty.
     *
     * @param fbpkt received feedback packet
     */
    void receiveHarqFeedback(Packet *fbpkt);

    /**
     * Sends all PDUs contained in units of selected process down
     */
    void sendSelectedDown();

    /**
     * Drops all of the PDUs of the units belonging to the process.
     * @param acid the H-ARQ process
     */
    void dropProcess(unsigned char acid);

    /**
     * Sends simulated HARQNACK to all units of the process which contains
     * the one whose id is specified as a parameter.
     * All units must be NACKed because a process is atomic for
     * transmission.
     *
     * This method should be used when a unit cannot retransmit
     * (e.g. too large PDU) and a dropping policy is desired: the process
     * status will advance and the PDU will eventually be dropped,
     * depending on the number of already performed transmissions.
     *
     * @param acid the H-ARQ process
     * @param cw the codeword to be NACKed
     */
    void selfNack(unsigned char acid, Codeword cw);

    /**
     * Drops a whole process; as usual, unitId or acid can be passed.
     *
     * @param unitId unit id.
     */
    void forceDropProcess(unsigned char acid);

    /**
     * Drops a single unit.
     *
     * @param unitId unit id.
     */
    void forceDropUnit(unsigned char acid, Codeword cw);

    BufferStatus getBufferStatus();

    std::vector<LteHarqProcessTx *> *getHarqProcesses() { return &processes_; }
    unsigned int getNumProcesses() { return numProc_; }

    /**
     * @author Alessandro Noferi
     *
     * Check if the buffer is active
     *
     * @return true if the TxHarqPduStatus of all units of all processes is not TXHARQ_PDU_EMPTY
     */

    bool isHarqBufferActive() const;

    virtual ~LteHarqBufferTx();

  protected:

    /**
     * Checks if a given unitId is present in a unitIds list.
     *
     * @param unitId unit id to be found in the list.
     * @param unitIds list where to search for the given unitId.
     * @return true if the id is in the list, false otherwise.
     */
    bool isInUnitList(unsigned char acid, Codeword cw, UnitList unitIds);
};

} //namespace

#endif


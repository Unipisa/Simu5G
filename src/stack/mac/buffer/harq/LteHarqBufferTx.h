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

/*
 * NOTA: e' compito del mac ul usare solo il processo di turno, non c'e' nessun controllo.
 * TODO: aggiungere supporto all'uplink: funzioni in cui si specifica il processo da usare
 * TODO: commenti
 */

class LteHarqBufferTx
{
  protected:
    LteMacBase *macOwner_;
    std::vector<LteHarqProcessTx *> *processes_;
    unsigned int numProc_;
    unsigned int numEmptyProc_; // @ fb on reset, @ insert
    unsigned char selectedAcid_; // @ insert, @ marksel, @ sendseldn
    MacNodeId nodeId_; // UE nodeId for which this buffer has been created

  public:

    /*
     * Default Constructor
     */
    LteHarqBufferTx() {}

    /**
     * Constructor.
     *
     * @param numProc number of H-ARQ processes in the buffer.
     * @param owner simple module instantiating an H-ARQ TX buffer
     * @param nodeId UE nodeId for which this buffer has been created
     */
    LteHarqBufferTx(unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac);

    /**
     * Copy constructor and operator=
     */
    LteHarqBufferTx(const LteHarqBufferTx& other)
    {
        operator=(other);
    }
    LteHarqBufferTx& operator=(const LteHarqBufferTx& other);

    /*
     * Get a reference to the specified process
     */
    LteHarqProcessTx * getProcess(unsigned char acid);

    /*
     * Get a reference to the selected process
     */
    LteHarqProcessTx * getSelectedProcess();

    /**
     * Finds the H-ARQ process which contains the pdu which is not
     * trasmitting for the longest period of time and returns
     * a list of unitIds of the units that belongs to this same
     * process which are in ready state (the ids of the units which will
     * be retransmitted).
     * N.B.: inside an H-ARQ process containing ready units, there will
     * be only units ready for rtx or empty units, because H-ARQ feedback
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
     * @return pdu length in bytes
     */
    inet::int64 pduLength(unsigned char acid, Codeword cw);

    /**
     * Takes a list of ids of the units that must retransmit and the number
     * of available TBs for the user (which depends on the txmode), and
     * prepares, whenever possible, the pdus for retransmission.
     * If the process has more ready units than unitIds list size,
     * then excluded units' pdus will be dropped (not all units of
     * the process have been selected for retransmission).
     * If more units have been selected for retransmission than the number
     * of available TBs, then a single pdu containing all of the sdus, will
     * be created and selected for retransmissions. All the other units
     * will be reset.
     * If the number of available TBs is sufficient, then simply all the
     * units whose ids are in unitIds list are selected for retransmission.
     *
     * @param unitIds list of ids of the units that must retransmit.
     * @param availableTbs number of available TBs for the user.
     */
    void markSelected(UnitList unitIds, unsigned char availableTbs);

    /**
     * Inserts a pdu in the unit with specified id.
     * A newly inserted pdu automatically marks the process as selected.
     *
     * @param unitId id of the destination unit
     * @param pdu pdu to be inserted
     */
    void insertPdu(unsigned char acid, Codeword cw, Packet *pdu);

    /**
     * Returns a pair with h-arq process id and a list of its empty units to be used for transmission.
     * If a process has already been selected (e.g. for rtx), then the same
     * process must be used for transmission, otherwise the units ids of an
     * empty process are returned.
     *
     * @return  a list of acid and their units  to be used for transmission
     */
    UnitList firstAvailable();

    /*
     * Returns a pair with the specified h-arq process id and a list of its empty units to be used for transmission.
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
     * Sends all pdus contained in units of selected process down
     */
    void sendSelectedDown();

    /**
     * Drops all of the pdus of the units belonging to the process.
     * @param acid the H-arq process
     */
    void dropProcess(unsigned char acid);

    /**
     * Sends simulated HARQNACK to all units of the process which contains
     * the one whose id is specified as parameter.
     * All units must be nacked because a process is atomic for
     * transmission.
     *
     * This method should be used when an unit cannot retransmit
     * (eg. too large pdu) and a dropping policy is desired: the process
     * status will advance and the pdu will eventually be dropped,
     * depending on the number of already performed transmissions.
     *
     * @param acid the H-arq process
     * @param cw the codeword to be n-acked
     */
    void selfNack(unsigned char acid, Codeword cw);

    /**
     * Drops a whole process, as usual unitId or acid can be passed.
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

    std::vector<LteHarqProcessTx *> * getHarqProcesses(){ return processes_ ; }
    unsigned int getNumProcesses() { return numProc_; }

    virtual ~LteHarqBufferTx();

  protected:

    /**
     * Checks if a given unitId is present in an unitIds list.
     *
     * @param unitId unit id to be found in list.
     * @param unitIds list where to search for the given unitId.
     * @return true if the id is in the list, false otherwise.
     */
    bool isInUnitList(unsigned char acid, Codeword cw, UnitList unitIds);
};

#endif

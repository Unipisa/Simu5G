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

#ifndef _LTE_LTEHARQBUFFERRX_H_
#define _LTE_LTEHARQBUFFERRX_H_

#include "stack/mac/LteMacBase.h"
#include "stack/mac/buffer/harq/LteHarqProcessRx.h"

namespace simu5g {

using namespace omnetpp;

class LteMacBase;
class LteHarqProcessRx;

/**
 * H-ARQ RX buffer: messages coming from PHY are stored in the H-ARQ RX buffer.
 * When a new PDU is inserted, it is in EVALUATING state, meaning that the hardware is
 * checking its correctness.
 * A feedback is sent after the PDU has been evaluated (HARQ_FB_EVALUATION_INTERVAL), and
 * in case of ACK, the PDU moves to CORRECT state; otherwise, it is dropped from the process.
 * The operations of checking if a PDU is ready for feedback and if it is in the correct state are
 * done in the extractCorrectPdu method, which must be called at every TTI (it must be part
 * of the MAC main loop).
 */
class LteHarqBufferRx
{
  protected:
    /// binder module reference
    opp_component_ptr<Binder> binder_;

    /// MAC module reference
    opp_component_ptr<LteMacBase> macOwner_;

    /// number of contained H-ARQ processes
    unsigned int numHarqProcesses_;

    /// ID of the source node for which this buffer has been created
    MacNodeId srcId_;

    /// processes vector
    std::vector<LteHarqProcessRx *> processes_;

    /// flag for multicast flows
    bool isMulticast_;

    // Statistics
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalRcvdBytes_ = 0;
    Direction dir = UNKNOWN_DIRECTION;

    static simsignal_t macCellThroughputSignal_[2];
    static simsignal_t macDelaySignal_[2];
    static simsignal_t macThroughputSignal_[2];

    // reference to the eNB module
    opp_component_ptr<cModule> nodeB_;

  private:
    // LteMacBase* of the UE for which this buffer has been created (whose ID is srcId_).
    // Only access via methods. This can be nullptr if node is removed from simulation
    opp_component_ptr<LteMacBase> macUe_;

  protected:
    LteHarqBufferRx(Binder *binder, LteMacBase *owner, unsigned int num, MacNodeId srcId);

  public:
    LteHarqBufferRx(unsigned int num, LteMacBase *owner, Binder *binder, MacNodeId srcId);

    /**
     * Insertion of a new PDU coming from PHY layer into
     * RX H-ARQ buffer.
     *
     * @param PDU to be inserted
     */
    virtual void insertPdu(Codeword cw, inet::Packet *pkt);

    /**
     * Sends feedback for all processes which are older than
     * HARQ_FB_EVALUATION_INTERVAL, then extracts the PDU in correct state (if any)
     *
     * @return uncorrupted PDUs or empty list if none
     */
    virtual std::list<inet::Packet *> extractCorrectPdus();

    /**
     * Purges PDUs in corrupted state (if any)
     *
     * @return number of purged corrupted PDUs or zero if none
     */
    unsigned int purgeCorruptedPdus();

    /*
     * Returns pointer to <acid> process.
     */
    LteHarqProcessRx *getProcess(unsigned char acid)
    {
        return processes_.at(acid);
    }

    //* Returns the number of contained H-ARQ processes
    unsigned int getProcesses()
    {
        return numHarqProcesses_;
    }

    // @return whole buffer status {RXHARQ_PDU_EMPTY, RXHARQ_PDU_EVALUATING, RXHARQ_PDU_CORRECT, RXHARQ_PDU_CORRUPTED }
    RxBufferStatus getBufferStatus();

    /**
     * Returns a pair with H-ARQ process ID and a list of its empty {RXHARQ_PDU_EMPTY} units to be used for reception of new H-ARQ sub-bursts.
     *
     * @return a list of acid and their units to be used for reception
     */
    UnitList firstAvailable();

    /*
     * Returns a pair with the specified H-ARQ process ID and a list of its empty units to be used for reception.
     */
    UnitList getEmptyUnits(unsigned char acid);

    /*
     * returns true if the corresponding flow is a multicast one
     */
    bool isMulticast() { return isMulticast_; }

    /*
     *  Corresponding cModule node will be removed from simulation.
     */
    virtual void unregister_macUe() {
        macUe_ = nullptr;
    }

    /**
     * @author Alessandro Noferi
     *
     * Check if the buffer is active
     *
     * @return true if the RxHarqPduStatus of all units of all processes is not RXHARQ_PDU_EMPTY
     */

    bool isHarqBufferActive() const;

    virtual ~LteHarqBufferRx();

  protected:
    /**
     * Checks for all processes if the PDU has been evaluated and sends
     * feedback if affirmative.
     */
    virtual void sendFeedback();

    /**
     *  Only emit signals from macUe_ if the node still exists.
     *  It is possible that the source node (e.g., a UE) left the simulation, but the
     *  packets from the node still reside in the simulation.
     */
    virtual void macUe_emit(simsignal_t signal, double val)
    {
        if (macUe_ != nullptr) {
            macUe_->emit(signal, val);
        }
    }

    /**
     * macSource_ is a private member, so derived classes need this member function to
     * initialize it. macSource_ being a private member forces one to use the macSource_emit
     * function to emit statistics (hence, checking against nullptr)
     */
    void initMacUe() {
        if (macOwner_->getNodeType() == ENODEB || macOwner_->getNodeType() == GNODEB)
            macUe_ = check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, srcId_));
        else
            macUe_ = macOwner_;
    }

};

} //namespace

#endif


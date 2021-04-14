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

#ifndef _LTE_LTEMACENB_H_
#define _LTE_LTEMACENB_H_

#include "common/cellInfo/CellInfo.h"
#include "stack/mac/layer/LteMacBase.h"
#include "stack/mac/amc/LteAmc.h"
#include "common/LteCommon.h"

class MacBsr;
class LteSchedulerEnbDl;
class LteSchedulerEnbUl;
class ConflictGraph;

class LteMacEnb : public LteMacBase
{
  protected:
    /// Local CellInfo
    CellInfo *cellInfo_;

    /// Lte AMC module
    LteAmc *amc_;

    /// Number of antennas (MACRO included)
    int numAntennas_;

    /// List of scheduled users (one per carrier) - Downlink
    std::map<double, LteMacScheduleList>* scheduleListDl_;

    int eNodeBCount;

    /**
     * Variable used for Downlink energy consumption computation
     */
    /*******************************************************************************************/

    // Current subframe type
    LteSubFrameType currentSubFrameType_;
    //frame index in the mbsfn pattern
    int frameIndex_;
    //number of resource block allcated in last tti
    unsigned int lastTtiAllocatedRb_;

    /*******************************************************************************************/
    // Resource Elements per Rb
    std::vector<double> rePerRb_;

    // Resource Elements per Rb - MBSFN frames
    std::vector<double> rePerRbMbsfn_;

    /// Buffer for the BSRs
    LteMacBufferMap bsrbuf_;

    /// Lte Mac Scheduler - Downlink
    LteSchedulerEnbDl* enbSchedulerDl_;

    /// Lte Mac Scheduler - Uplink
    LteSchedulerEnbUl* enbSchedulerUl_;

    /**
     * Reads MAC parameters for eNb and performs initialization.
     */
    virtual void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * creates scheduling grants (one for each nodeId) according to the Schedule List.
     * It sends them to the  lower layer
     */
    virtual void sendGrants(std::map<double, LteMacScheduleList>* scheduleList);

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List (stored after scheduling).
     * It sends them to H-ARQ
     */
    virtual void macPduMake(MacCid cid) override;

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * On ENB it also extracts the BSR Control Element
     * and stores it in the BSR buffer (for the cid from
     * which packet was received)
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(omnetpp::cPacket* pkt) override;

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual void macSduRequest();

    /**
     * bufferizeBsr() works much alike bufferizePacket()
     * but only saves the BSR in the corresponding virtual
     * buffer, eventually creating it if a queue for that
     * cid does not exists yet.
     *
     * @param bsr bsr to store
     * @param cid connection id for this bsr
     */
    void bufferizeBsr(MacBsr* bsr, MacCid cid);

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     *
     * @param pkt Packet to be buffered
     * @return TRUE if packet was buffered successfully, FALSE otherwise.
     */
    virtual bool bufferizePacket(omnetpp::cPacket* pkt) override;

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(omnetpp::cPacket* pkt) override;

    /**
     * Main loop
     */
    virtual void handleSelfMessage() override;
    /**
     * macHandleFeedbackPkt is called every time a feedback pkt arrives on MAC
     */
    virtual void macHandleFeedbackPkt(omnetpp::cPacket* pkt) override;

    /*
     * Receives and handles RAC requests
     */
    virtual void macHandleRac(omnetpp::cPacket* pkt) override;

    /*
     * Update UserTxParam stored in every lteMacPdu when an rtx change this information
     */
    virtual void updateUserTxParam(omnetpp::cPacket* pkt) override;

    /**
     * Flush Tx H-ARQ buffers for all users
     */
    virtual void flushHarqBuffers();

  public:

    LteMacEnb();
    virtual ~LteMacEnb();

    /// Returns the BSR virtual buffers
    LteMacBufferMap* getBsrVirtualBuffers()
    {
        return &bsrbuf_;
    }

    /**
     * deleteQueues() on ENB performs actions
     * from base class and also deletes the BSR buffer
     *
     * @param nodeId id of node performig handover
     */
    virtual void deleteQueues(MacNodeId nodeId) override;

    /**
     * Getter for AMC module
     */
    virtual LteAmc *getAmc()
    {
        return amc_;
    }

    /**
     * Getter for cellInfo.
     */
    virtual CellInfo* getCellInfo();

    /**
     * Returns the number of system antennas (MACRO included)
     */
    virtual int getNumAntennas();

    /**
     * Returns the scheduling discipline for the given direction.
     * @par dir link direction
     */
    SchedDiscipline getSchedDiscipline(Direction dir);

    //! Lte Advanced Scheduler support

    /*
     * Getter for allocation RBs
     * @par dir link direction
     */
    unsigned int getAllocationRbs(Direction dir);

    /*
     * Getter for deadline
     * @par tClass traffic class
     * @par dir link direction
     */

    double getLteAdeadline(LteTrafficClass tClass, Direction dir);

    /*
     * Getter for slack-term
     * @par tClass traffic class
     * @par dir link direction
     */

    double getLteAslackTerm(LteTrafficClass tClass, Direction dir);

    /*
     * Getter for maximum urgent burst size
     * @par tClass traffic class
     * @par dir link direction
     */
    int getLteAmaxUrgentBurst(LteTrafficClass tClass, Direction dir);

    /*
     * Getter for maximum fairness burst size
     * @par tClass traffic class
     * @par dir link direction
     */
    int getLteAmaxFairnessBurst(LteTrafficClass tClass, Direction dir);

    /*
     * Getter for fairness history size
     * @par dir link direction
     */
    int getLteAhistorySize(Direction dir);
    /*
     * Getter for fairness TTI threshold
     * @par dir link direction
     */
    int getLteAgainHistoryTh(LteTrafficClass tClass, Direction dir);

    // Power Model Parameters
    /* minimum depleted power (W)
     * @par dir link direction
     */
    double getZeroLevel(Direction dir, LteSubFrameType type);
    /* idle state depleted power (W)
     * @par dir link direction
     */
    double getIdleLevel(Direction dir, LteSubFrameType type);
    /* per-block depletion unit (W)
     * @par dir link direction
     */
    double getPowerUnit(Direction dir, LteSubFrameType type);
    /* maximumum depletable power (W)
     * @par dir link direction
     */
    double getMaxPower(Direction dir, LteSubFrameType type);

    /* getter for the flag that enable PF legacy in LTEA scheduler
     *  @par dir link direction
     */
    bool getPfTmsAwareFlag(Direction dir);

    /*
     * getter for current sub-frame type.
     * It sould be NORMAL_FRAME_TYPE, MBSFN, SYNCRO
     * BROADCAST, PAGING and ABS
     * It is used in order to compute the energy consumption
     */
    LteSubFrameType getCurrentSubFrameType() const
    {
        return currentSubFrameType_;
    }

    /*
     * Update the number of allocatedRb in last tti
     * @par number of resource block
     */
    void allocatedRB(unsigned int rb);

    /*
     * Return the current active set (active connections)
     * @par direction
     */
    ActiveSet* getActiveSet(Direction dir);

    void cqiStatistics(MacNodeId id, Direction dir, LteFeedback fb);

    // get band occupation for this/previous TTI. Used for interference computation purposes
    unsigned int getDlBandStatus(Band b);
    unsigned int getDlPrevBandStatus(Band b);
    virtual bool isReuseD2DEnabled()
    {
        return false;
    }

    virtual bool isReuseD2DMultiEnabled()
    {
        return false;
    }

    virtual ConflictGraph* getConflictGraph();

};

#endif

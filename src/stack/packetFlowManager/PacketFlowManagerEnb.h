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

#ifndef _LTE_PACKETFLOWMANAGERENB_H_
#define _LTE_PACKETFLOWMANAGERENB_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "nodes/mec/utils/MecCommon.h"

#include "PacketFlowManagerBase.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

namespace simu5g {

using namespace omnetpp;

/*
 * This module is responsible for keeping track of all PDCP SDUs.
 * A PDCP SDU passes through the following states while it is going down
 * through the LTE NIC layers:
 *
 * PDCP SDU
 * few operations
 * PDCP PDU
 * RLC SDU
 * RLC PDU or fragmented into more than one RLC PDU
 * MAC SDU
 * inserted into one TB
 * MAC PDU (aka TB)
 *
 * Each PDCP has its own sequence number, managed by the corresponding LCID
 *
 * The main functions of this module are:
 *  - detect PDCP SDU discarded (no part transmitted)
 *  - calculate the delay time of a packet, from PDCP SDU to last Harq ACK of the
 *    corresponding sequence number.
 */

class LteRlcUmDataPdu;

class PacketFlowManagerEnb : public PacketFlowManagerBase
{
  protected:

    struct Grant {
        unsigned int grantId;
        simtime_t sendTimestamp;
    };

    struct BurstStatus {
        std::map<unsigned int, unsigned int> rlcPdu; // RLC PDU of the burst and the relative RLC SDU size
        simtime_t startBurstTransmission; // moment of the first transmission of the burst
        unsigned int burstSize; // PDCP SDU size of the burst
        bool isCompleted;
    };

    struct PacketLoss {
        int lastPdpcSno;
        int totalLossPdcp;
        unsigned int totalPdcpArrived;
        unsigned int totalPdcpSno;

        void clear()
        {
            lastPdpcSno = 0;
            totalLossPdcp = 0;
            totalPdcpArrived = 0;
            totalPdcpSno = 0;
        }

        // resets the counters at the end of each period, i.e., lastPdcpSno remains
        void reset()
        {
            totalLossPdcp = 0;
            totalPdcpArrived = 0;
            totalPdcpSno = 0;
        }
    };

    /*
     * The node can have different active connections (LCID) at the same time, hence we need to
     * maintain the status for each of them
     */
    struct StatusDescriptor {
        MacNodeId nodeId_; // destination node of this LCID
        bool burstState_; // control variable that controls one burst active at a time
        BurstId burstId_; // separates the bursts
        std::map<unsigned int, PdcpStatus> pdcpStatus_; // a PDCP PDU can be fragmented into many RLC PDUs that could be sent and acknowledged at different times (this prevents early removal on acknowledgment)
        std::map<BurstId, BurstStatus> burstStatus_; // for each burst, stores relative information
        std::map<unsigned int, SequenceNumberSet> rlcPdusPerSdu_;  // for each RLC SDU, stores the RLC PDUs where the former was fragmented
        std::map<unsigned int, SequenceNumberSet> rlcSdusPerPdu_;  // for each RLC PDU, stores the included RLC SDUs
        std::map<unsigned int, SequenceNumberSet> macSdusPerPdu_;  // for each MAC PDU, stores the included MAC SDUs (should be a 1:1 association)
        //std::vector<unsigned int> macPduPerProcess_;               // for each HARQ process, stores the included MAC PDU
    };

    typedef  std::map<LogicalCid, StatusDescriptor> ConnectionMap;
    ConnectionMap connectionMap_; // LCID to the corresponding StatusDescriptor

    opp_component_ptr<LtePdcpRrcEnb> pdcp_;

    std::map<MacNodeId, Delay> ULPktDelay_;
    std::map<MacNodeId, std::vector<Grant>> ulGrants_;
    std::map<MacNodeId, PacketLoss> packetLossRate_;
    std::map<MacNodeId, Delay> pdcpDelay_; // map that sums all the delay times of a destination NodeId (UE) and the corresponding counter
    std::map<MacNodeId, Throughput> pdcpThroughput_; // map that sums all the bytes sent by a destination NodeId (UE) and the corresponding time elapsed
    std::map<MacNodeId, DiscardedPkts> pktDiscardCounterPerUe_; // discard counter per NodeId (UE)
    std::map<MacNodeId, DataVolume> sduDataVolume_;
    short int harqProcesses_; // number of HARQ processes

    // debug variable that calculates DL delay of a UE (with id 2053)
    // used to evaluate the delay with respect to the one reported by Simu5G
    cOutVector timesUe_;

    /*
     * This method checks if a PDCP PDU of a LCID is part of a burst of data.
     * In a positive case, according to the ack boolean its size is counted in the
     * total of the burst size.
     * It is called by macPduArrived (ack true) and rlcPduDiscarded (ack false)
     * @param desc LCID descriptor
     * @param pdcpSno PDCP sequence number
     * @bool ack PDCP acknowledgment flag
     */
    void removePdcpBurstRLC(StatusDescriptor *desc, unsigned int rlcSno, bool ack);

    /*
     * This method creates a pdcpStatus structure when a PDCP SDU arrives at the PDCP layer.
     */
    void initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int sduHeaderSize, simtime_t& arrivalTime);

    void initialize(int stage) override;

  public:
    // return true if a structure for this LCID is present
    bool hasLcid(LogicalCid lcid) override;
    // initialize a new structure for this LCID
    void initLcid(LogicalCid lcid, MacNodeId nodeId) override;
    // reset the structure for this LCID
    void clearLcid(LogicalCid lcid) override;
    // reset structures for all connections
    void clearAllLcid() override;

    void insertPdcpSdu(inet::Packet *pdcpPkt) override;
    void receivedPdcpSdu(inet::Packet *pdcpPkt) override;

    void insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) override;

    void insertMacPdu(inet::Ptr<const LteMacPdu>) override;

    /*
     * This method checks if the HARQ acknowledgment relative to a macPduId acknowledges an ENTIRE
     * PDCP SDU
     * @param lcid
     * @param macPduId Omnet ID of the MAC PDU
     */
    void macPduArrived(inet::Ptr<const LteMacPdu>) override;

    void ulMacPduArrived(MacNodeId nodeId, unsigned int grantId) override;

    /*
     * This method is called after maxHarqTransmission of a MAC PDU ID has been
     * reached. The PDCP, RLC, SN referred to the macPdu are cleared from the
     * data structures
     * @param lcid
     * @param macPduId Omnet ID of the MAC PDU to be discarded
     */
    void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;

    /*
     * This method is used to keep track of all discarded RLC PDUs. If all RLC PDUs
     * that compose a PDCP SDU have been discarded the discarded counters are updated
     * @param lcid
     * @param rlcSno sequence number of the RLC PDU
     * @param fromMac used when this method is called by discardMacPdu
     */
    void discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac = false) override;

    void insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId) override;

    void grantSent(MacNodeId nodeId, unsigned int grantId) override;

    /*
     * invoked by the MAC layer to notify that harqProcId is completed.
     * This method needs to go back up the chain of sequence numbers to identify which
     * PDCP SDUs have been transmitted in this process.
     */
    // void notifyHarqProcess(LogicalCid lcid, unsigned int harqProcId);

    /*
     * deletes all the LCID structures related to the UE
     * called upon handover
     */
    virtual void deleteUe(MacNodeId id);

    /*
     * The methods are called for a specific UE
     */

    virtual uint64_t getDataVolume(MacNodeId nodeId, Direction dir);
    virtual void resetDataVolume(MacNodeId nodeId, Direction dir);
    virtual void resetDataVolume(MacNodeId nodeId);

    virtual double getPdpcLossRate();
    virtual double getPdpcLossRatePerUe(MacNodeId id);
    virtual void resetPdpcLossRates();
    virtual void resetPdpcLossRatePerUe(MacNodeId id);

    virtual double getDiscardedPktPerUe(MacNodeId id);
    virtual double getDiscardedPkt();
    virtual void resetDiscardCounterPerUe(MacNodeId id);

    virtual double getDelayStatsPerUe(MacNodeId id);
    virtual void resetDelayCounterPerUe(MacNodeId id);

    virtual double getUlDelayStatsPerUe(MacNodeId id);
    virtual void resetUlDelayCounterPerUe(MacNodeId id);

    virtual double getThroughputStatsPerUe(MacNodeId id);
    virtual void resetThroughputCounterPerUe(MacNodeId id);
    void finish() override;
};

} //namespace

#endif


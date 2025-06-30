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

#ifndef _LTE_PACKETFLOWMANAGERUE_H_
#define _LTE_PACKETFLOWMANAGERUE_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "PacketFlowManagerBase.h"

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
 * Each PDCP has its own sequence number, managed by the corresponding LCID.
 *
 * The main functions of this module are:
 *  - detect PDCP SDU discarded (no part transmitted)
 *  - calculate the delay time of a packet, from PDCP SDU to last HARQ ACK of the
 *    corresponding sequence number.
 */

class LteRlcUmDataPdu;
class LtePdcpRrcUe;
class PacketFlowManagerUe : public PacketFlowManagerBase
{

    /*
     * The node can have different active connections (LCID) at the same time, hence we need to
     * maintain the status for each of them.
     */
    struct StatusDescriptor {
        MacNodeId nodeId_; // destination node of this LCID
        std::map<unsigned int, PdcpStatus> pdcpStatus_; // a PDCP PDU can be fragmented into many RLC that could be sent and acknowledged at different times (this prevents early removal on acknowledgment)
        std::map<unsigned int, SequenceNumberSet> rlcPdusPerSdu_;  // for each RLC SDU, stores the RLC PDUs where the former was fragmented
        std::map<unsigned int, SequenceNumberSet> rlcSdusPerPdu_;  // for each RLC PDU, stores the included RLC SDUs
        std::map<unsigned int, SequenceNumberSet> macSdusPerPdu_;  // for each MAC PDU, stores the included MAC SDUs (should be a 1:1 association)
        std::vector<unsigned int> macPduPerProcess_;               // for each HARQ process, stores the included MAC PDU
    };

    typedef std::map<LogicalCid, StatusDescriptor> ConnectionMap;
    ConnectionMap connectionMap_; // LCID to the corresponding StatusDescriptor

    Delay pdcpDelay;

    // Debug variables to be deleted
    cOutVector times_;
    std::set<unsigned int> myset;

  protected:

    void initialize(int stage) override;
    void initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int sduHeaderSize, simtime_t& arrivalTime);

  public:
    // return true if a structure for this LCID is present
    bool hasLcid(LogicalCid lcid) override;
    // initialize a new structure for this LCID
    void initLcid(LogicalCid lcid, MacNodeId nodeId) override;
    // reset the structure for this LCID
    void clearLcid(LogicalCid lcid) override;
    // reset structures for all connections
    void clearAllLcid() override;
    virtual void clearStats();

    void insertPdcpSdu(inet::Packet *pdcpPkt) override;
    /*
     * This method inserts a new RLC sequence number and the corresponding PDCP PDUs inside it.
     * @param lcid
     * @param rlcSno sequence number of the RLC PDU
     * @param pdcpSnoSet list of PDCP PDUs inside the RLC PDU
     * @param lastIsFrag used to inform if the last PDCP is fragmented or not.
     */

    void insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) override;

    /*
     * This method inserts a new MAC PDU ID Omnet ID and the corresponding RLC PDUs inside it.
     * @param lcid
     * @param macPdu packet pointer
     */
    void insertMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;

    /*
     * This method checks if the HARQ ACK sequence number set relative to a MAC PDU ID acknowledges an ENTIRE
     * PDCP SDU.
     * @param lcid
     * @param macPduId Omnet ID of the MAC PDU
     */
    void macPduArrived(const inet::Ptr<const LteMacPdu> macPdu) override;

    /*
     * This method is called after the maximum HARQ transmission of a MAC PDU ID has been
     * reached. The PDCP, RLC, sequence number referred to the MAC PDU are cleared from the
     * data structures.
     * @param lcid
     * @param macPduId Omnet ID of the MAC PDU to be discarded
     */
    void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;

    /*
     * This method is used to keep track of all discarded RLC PDUs. If all RLC PDUs
     * that compose a PDCP SDU have been discarded, the discard counters are updated.
     * @param lcid
     * @param rlcSno sequence number of the RLC PDU
     * @param fromMac used when this method is called by discardMacPdu
     */
    void discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac = false) override;

    void insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId) override;

    /*
     * Invoked by the MAC layer to notify that harqProcId is completed.
     * This method needs to go back up the chain of sequence numbers to identify which
     * PDCP SDUs have been transmitted in this process.
     */
    // void notifyHarqProcess(LogicalCid lcid, unsigned int harqProcId);

    DiscardedPkts getDiscardedPkt();

    double getDelayStats();
    void resetDelayCounter();

    void finish() override;

};

} //namespace

#endif

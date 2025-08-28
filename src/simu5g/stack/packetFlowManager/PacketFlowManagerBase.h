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

#ifndef _LTE_PACKETFLOWMANAGERBASE_H_
#define _LTE_PACKETFLOWMANAGERBASE_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/mec/utils/MecCommon.h"

namespace simu5g {

using namespace omnetpp;

typedef std::set<unsigned int> SequenceNumberSet;
typedef unsigned int BurstId;

class LteRlcUmDataPdu;
struct StatusDescriptor;

/**
 * This module is responsible for keeping tracking the flow of SDUs in the stack,
 * and compute statistics from them. It is passive observer of the packet flow.
 *
 * A PDCP SDU is encapsulated in the following packets while it is going down
 * through the LTE NIC layers:
 *
 * - PDCP SDU
 * - some operations
 * - PDCP PDU
 * - RLC SDU
 * - RLC PDU or fragmented into more than one RLC PDUs
 * - MAC SDU
 * - inserted into one TB
 * - MAC PDU (aka TB)
 *
 * Each PDCP has its own sequence number, managed by the corresponding LCID
 *
 * The main functions of this module are:
 *  - detect PDCP SDUs discarded (no part transmitted)
 *  - calculate the delay time of a packet, from PDCP SDU to last HARQ ACK of the
 *    corresponding seq number.
 */
class PacketFlowManagerBase : public cSimpleModule
{
  protected:

    /*
     * this structure maintains the state of a PDCP
     * used to calculate L2Meas of RNI service
     */
    struct PdcpStatus {
        bool hasArrivedAll;
        bool discardedAtRlc;
        bool discardedAtMac;
        bool sentOverTheAir;
        unsigned int pdcpSduSize;
        simtime_t entryTime;
    };

    DiscardedPkts pktDiscardCounterTotal_; // total discarded packets counter of the node

    RanNodeType nodeType_; // UE or ENODEB (used for set MACROS)
    short int harqProcesses_; // number of HARQ processes

    std::string pfmType;

    int headerCompressedSize_;

  protected:
    int numInitStages() const override { return 2; }
    void initialize(int stage) override;
    void finish() override;

    // Return true if a data structure for this LCID is present
    virtual bool hasLcid(LogicalCid lcid) = 0;

    // Initialize a new data structure for this LCID. Abstract since in eNodeB case,
    // it initializes different structures with respect to the UE
    virtual void initLcid(LogicalCid lcid, MacNodeId nodeId) = 0;

    // Reset the data structure for this LCID. Abstract since in eNodeB case,
    // it clears different structures with respect to the UE
    virtual void clearLcid(LogicalCid lcid) = 0;

    // Reset data structures for all connections
    virtual void clearAllLcid() = 0;

  public:
    /**
     * This method is called when a PDCP SDU enters the PDCP layer for downlink transmission.
     * It records the PDCP sequence number and entry timestamp in the tracking data structures
     * to enable delay measurement and packet flow monitoring. Used only by the eNodeB.
     */
    virtual void insertPdcpSdu(inet::Packet *pdcpPkt) = 0;

    /**
     * This method is called when a PDCP SDU is received on the uplink.
     * It tracks uplink data volume and packet loss rate statistics.
     */
    virtual void receivedPdcpSdu(inet::Packet *pdcpPkt) = 0;

    /**
     * This method is called when an RLC PDU is created from PDCP SDUs for transmission.
     * It records the mapping between the RLC PDU and its contained PDCP SDUs in the tracking
     * data structures, along with burst status information for throughput measurement.
     */
    virtual void insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) = 0;

    /**
     * This method is called when a MAC PDU is inserted into the HARQ buffer for transmission.
     * It records the mapping between the MAC PDU and its contained RLC PDUs in the tracking
     * data structures for downlink packet flow management.
     */
    virtual void insertMacPdu(const inet::Ptr<const LteMacPdu> macPdu) = 0;

    /**
     * This method is called when a downlink MAC PDU is successfully acknowledged by the UE.
     * It processes the acknowledgment to determine if complete PDCP SDUs have been delivered,
     * calculates packet delays, and updates statistics for downlink transmission.
     */
    virtual void macPduArrived(const inet::Ptr<const LteMacPdu> macPdu) = 0;

    /**
     * This method is called when an uplink MAC PDU is received from a UE at the eNodeB.
     * It measures uplink packet delay by matching the received PDU with the corresponding
     * grant that was previously sent to the UE (identified by grantId).
     */
    virtual void ulMacPduArrived(MacNodeId nodeId, unsigned int grantId) {};

    /**
     * This method is called after maxHarqTransmission of a MAC PDU ID has been
     * reached. The PDCP, RLC sequence numbers referred to the MAC PDU are cleared from the
     * data structures.
     */
    virtual void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) = 0;

    /**
     * This method is used to keep track of all discarded RLC PDUs. If all RLC PDUs
     * that compose a PDCP SDU have been discarded, the discarded counters are updated.
     * The fromMac parameter is used when this method is called by discardMacPdu.
     */
    virtual void discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac = false) = 0;

    /**
     * This method is called when an uplink grant is sent to a UE.
     * It records the grant transmission timestamp to enable uplink delay
     * measurement when the corresponding MAC PDU is received.
     */
    virtual void grantSent(MacNodeId nodeId, unsigned int grantId) {}

    virtual void resetDiscardCounter();
};

} //namespace

#endif

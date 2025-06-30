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

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "nodes/mec/utils/MecCommon.h"

namespace simu5g {

using namespace omnetpp;

typedef std::set<unsigned int> SequenceNumberSet;
typedef unsigned int BurstId;

/*
 * This module is responsible for keeping track of all PDCP SDUs.
 * A PDCP SDU is encapsulated in the following packets while it is going down
 * through the LTE NIC layers:
 *
 * PDCP SDU
 * some operations
 * PDCP PDU
 * RLC SDU
 * RLC PDU or fragmented into more than one RLC PDUs
 * MAC SDU
 * inserted into one TB
 * MAC PDU (aka TB)
 *
 * Each PDCP has its own seq number, managed by the corresponding LCID
 *
 * The main functions of this module are:
 *  - detect PDCP SDUs discarded (no part transmitted)
 *  - calculate the delay time of a pkt, from PDCP SDU to last Harq ACK of the
 *    corresponding seq number.
 */

class LteRlcUmDataPdu;
struct StatusDescriptor;

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
    short int harqProcesses_; // number of harq processes

    // when a mode switch occurs, the PDCP and RLC entities should be informed about the next SN to use,
    // i.e., the first SN not transmitted due to the mode switch
    unsigned int nextPdcpSno_;
    unsigned int nextRlcSno_;
    std::string pfmType;

    int headerCompressedSize_;

    int numInitStages() const override { return 2; }
    void initialize(int stage) override;

  public:


    // return true if a structure for this lcid is present
    virtual bool hasLcid(LogicalCid lcid) = 0;
    /*
     * initialize a new structure for this lcid
     * abstract since in eNodeB case, it initializes
     * different structure with respect to the UE
     */
    virtual void initLcid(LogicalCid lcid, MacNodeId nodeId) = 0;

    /* reset the structure for this lcid
     * abstract since in eNodeB case, it clears
     * different structure with respect to the UE
     */
    virtual void clearLcid(LogicalCid lcid) = 0;

    // reset structures for all connections
    virtual void clearAllLcid() = 0;

    /*
     * This method inserts a new pdcp seqnum and the corresponding entry time
     * @param lcid
     * @param pdcpSno sequence number of the pdcp pdu
     * @param entryTime the time the packet enters the PDCP layer
     *
     * Used only by the eNodeB packetFlowManager
     */
    virtual void insertPdcpSdu(inet::Packet *pdcpPkt) { EV << "PacketFlowManagerBase:insertPdcpSdu" << endl; }
    virtual void receivedPdcpSdu(inet::Packet *pdcpPkt) { EV << "PacketFlowManagerBase:receivedPdcpSdu" << endl; }

    /*
     * This method inserts a new rlc seqnum and the corresponding pdcp pdus inside it
     * @param lcid
     * @param rlcPdu packet pointer
     */

    virtual void insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) = 0;

    /*
     * This method inserts a new macPduId Omnet id and the corresponding rlc pdus inside it
     * @param lcid
     * @param macPdu packet pointer
     */
    virtual void insertMacPdu(const inet::Ptr<const LteMacPdu> macPdu) = 0;

    /*
     * This method checks if the HARQ aSequenceNumberSet related to a macPduId acknowledges an ENTIRE
     * pdcp sdu
     * @param lcid
     * @param macPduId Omnet id of the mac pdu
     */
    virtual void macPduArrived(const inet::Ptr<const LteMacPdu> macPdu) = 0;

    virtual void ulMacPduArrived(MacNodeId nodeId, unsigned int grantId) {};
    /*
     * This method is called after maxHarqTransmission of a MAC PDU ID has been
     * reached. The PDCP, RLC, seq numbers referred to the macPdu are cleared from the
     * data structures
     * @param lcid
     * @param macPduId Omnet id of the mac pdu to be discarded
     */
    virtual void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) = 0;

    /*
     * This method is used to keep track of all discarded RLC pdus. If all rlc pdus
     * that compose a PDCP SDU have been discarded the discarded counters are updated
     * @param lcid
     * @param rlcSno sequence number of the rlc pdu
     * @param fromMac used when this method is called by discardMacPdu
     */
    virtual void discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac = false) = 0;

    virtual void insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId) = 0;

    virtual void grantSent(MacNodeId nodeId, unsigned int grantId) {}

    /*
     * invoked by the MAC layer to notify that harqProcId is completed.
     * This method needs to go back up the chain of sequence numbers to identify which
     * PDCP SDUs have been transmitted in this process.
     */
    // void notifyHarqProcess(LogicalCid lcid, unsigned int harqProcId) = 0;

    virtual void resetDiscardCounter();

    virtual unsigned int getNextRlcSno() { return nextRlcSno_; }
    virtual unsigned int getNextPdcpSno() { return nextPdcpSno_; }

    void finish() override;

};

} //namespace

#endif


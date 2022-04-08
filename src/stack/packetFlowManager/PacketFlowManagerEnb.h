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
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

/*
 * This module is responsible for keep trace of all PDCP SDUs.
 * A PDCP SDU passes the following state while it is going down
 * through the LTE NIC layers:
 * 
 * PDCP SDU
 * few operations
 * PDCP PDU
 * RLC SDU
 * RLC PDU or fragmented in more then one RLC PDUs
 * MAC SDU
 * inserted into one TB
 * MAC PDU (aka TB)
 *
 * Each PDCP has its own seq number, managed by the corresponding LCID
 * 
 * The main functions of this module are:
 *  - detect PDCP SDU discarded (no part transmitted)
 *  - calculate the delay time of a pkt, from PDCP SDU to last Harq ACK of the
 *    corresponding seq number.  
 */

class LteRlcUmDataPdu;

class PacketFlowManagerEnb : public PacketFlowManagerBase
{
    protected:

        typedef struct
        {
           unsigned int grantId;
           simtime_t sendTimestamp;

        }Grant;

        typedef struct
        {
            std::map<unsigned int, unsigned int> rlcPdu; // RLC PDU of the burst and the relative Rlc sdu size
            simtime_t startBurstTransmission; // instant of the first trasmission of the burst
            unsigned int burstSize; // PDCP sdu size of the burst
            bool isComplited;
        } BurstStatus;

        typedef struct
        {
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

            // resets the counters at the end of each period, i.e lastPdcpSno remains
            void reset()
            {
                totalLossPdcp = 0;
                totalPdcpArrived = 0;
                totalPdcpSno = 0;
            }
        } PacketLoss;


        /*
        * The node can have different active connections (lcid) at the same time, hence we need to
        * maintain the status for each of them
        */
        typedef struct
        {
            MacNodeId nodeId_; // dest node of this lcid
            bool burstState_; // control variable that controls one burst active at a time
            BurstId burstId_; // separates the bursts
            std::map<unsigned int, PdcpStatus> pdcpStatus_; // a pdcp pdu can be fragmented in many rlc that could be sent and ack in different time (this prevent early remove on ack)
            std::map<BurstId, BurstStatus> burstStatus_; // for each burst, stores relative infos
            std::map<unsigned int, SequenceNumberSet> rlcPdusPerSdu_;  // for each RLC SDU, stores the RLC PDUs where the former was fragmented
            std::map<unsigned int, SequenceNumberSet> rlcSdusPerPdu_;  // for each RLC PDU, stores the included RLC SDUs
            std::map<unsigned int, SequenceNumberSet> macSdusPerPdu_;  // for each MAC PDU, stores the included MAC SDUs (should be a 1:1 association)
            //std::vector<unsigned int> macPduPerProcess_;               // for each HARQ process, stores the included MAC PDU
        } StatusDescriptor;

        typedef  std::map<LogicalCid, StatusDescriptor> ConnectionMap;
        ConnectionMap connectionMap_; // lcid to the corresponding StatusDescriptor

        LtePdcpRrcEnb * pdcp_;

        typedef std::map<MacNodeId, PacketLoss> packetLossRateMap;
        typedef std::map<MacNodeId, DataVolume> dataVolume; // discard counter per NodeId (UE)
        typedef std::map<MacNodeId, DiscardedPkts> pktDiscardMap; // discard counter per NodeId (UE)
        typedef std::map<MacNodeId, Delay> delayMap;
        typedef std::map<MacNodeId, Throughput> throughputMap;
        typedef std::map<MacNodeId, std::vector<Grant>> ULGrants;

        std::map<MacNodeId, Delay> ULPktDelay_ ;

        ULGrants ulGrants_;
        packetLossRateMap packetLossRate_;
        delayMap pdcpDelay_; // map that sums all the delay times of a dest NodeId (UE) and the corresponding counter
        throughputMap pdcpThroughput_; // map that sums all the bytes sent by a dest NodeId (UE) and the corresponding time elapsed
        pktDiscardMap pktDiscardCounterPerUe_;
        dataVolume sduDataVolume_;
        short int harqProcesses_; // number of harq processes

        // debug var that calculates DL delay of a UE (with id 2053)
        // used to evaluate the delay with respect to the one reported by Simu5G
        cOutVector timesUe_;

        /*
        * This method checks if a PDCP PDU of a Lcid is part of a burst of data.
        * In a positive case, according to the ack boolen its size it is counted in the
        * total og burst size.
        * It is called by macPduArrived (ack true) and rlcPduDsicard (ack false)
        * @param desc lcid descriptor
        * @param pdcpSno pdcp sequence number
        * @bool ack pdcp pdcp arrived flag
        */

        virtual void removePdcpBurstRLC(StatusDescriptor* desc, unsigned int rlcSno, bool ack);

        /*
         * This method creates a pdcpStatus structure when a pdcpSdu arrives at the PDCP layer.
         */
        virtual void initPdcpStatus(StatusDescriptor* desc, unsigned int pdcp, unsigned int sduHeaderSize, omnetpp::simtime_t& arrivalTime);

        virtual void initialize(int stage) override;

//    bool hasFragments(LogicalCid lcid, unsigned int pdcp);

    public:
        PacketFlowManagerEnb();
        // return true if a structure for this lcid is present
        virtual bool checkLcid(LogicalCid lcid) override;
        // initialize a new structure for this lcid
        virtual void initLcid(LogicalCid lcid, MacNodeId nodeId) override;
        // reset the structure for this lcid
        virtual void clearLcid(LogicalCid lcid) override;
        // reset structures for all connections
        virtual void clearAllLcid() override;

        virtual void insertPdcpSdu(inet::Packet* pdcpPkt) override;
        virtual void receivedPdcpSdu(inet::Packet* pdcpPkt) override;

        virtual void insertRlcPdu(LogicalCid lcid,  const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) override;

        virtual void insertMacPdu(inet::Ptr<const LteMacPdu>) override;


        /*
        * This method checks if the HARQ acSequenceNumberSetk relative to a macPduId acknowledges an ENTIRE
        * pdcp sdu
        * @param lcid
        * @param macPduId Omnet id of the mac pdu
        */
        virtual void macPduArrived(inet::Ptr<const LteMacPdu>) override;

        virtual void ulMacPduArrived(MacNodeId nodeId, unsigned int grantId) override;




        /*
        * This method is called after maxHarqTrasmission of a MAC PDU ID has been
        * reached. The PDCP, RLC, sno referred to the macPdu are cleared from the
        * data structures
        * @param lcid
        * @param macPduId Omnet id of the mac pdu to be discarded
        */
        virtual void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;

        /*
        * This method is used to take trace of all discarded RLC pdus. If all rlc pdus
        * that compose a PCDP SDU have been discarded the discarded counters are updated
        * @param lcid
        * @param rlcSno sequence number of the rlc pdu
        * @param fromMac used when this method is called by discardMacPdu
        */
        virtual void discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac = false) override;

        virtual void insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId) override;

        virtual void grantSent(MacNodeId nodeId, unsigned int grantId) override;


        /*
        * invoked by the MAC layer to notify that harqProcId is completed.
        * This method need to go back up the chain of sequence numbers to identify which
        * PDCP SDUs have been transmitted in this process.
        */
        // void notifyHarqProcess(LogicalCid lcid, unsigned int harqProcId);

        /*
         * deletes all the LCID structures related to the UE
         * called upon handover
         */
        virtual void deleteUe(MacNodeId id);

       /*
        * The methods are called per specific UE
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
        virtual ~PacketFlowManagerEnb();
        virtual void finish() override;
};
#endif

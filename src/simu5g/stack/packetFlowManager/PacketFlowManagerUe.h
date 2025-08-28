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

#include "simu5g/common/LteCommon.h"
#include "PacketFlowManagerBase.h"

namespace simu5g {

using namespace omnetpp;

class LteRlcUmDataPdu;
class LtePdcpUe;

/**
 * The UE-specific version of Packet Flow Manager.
 */
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

  protected:
    void initialize(int stage) override;
    void finish() override;

    void initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int sduHeaderSize, simtime_t& arrivalTime);
    // return true if a structure for this LCID is present
    bool hasLcid(LogicalCid lcid) override;
    // initialize a new structure for this LCID
    void initLcid(LogicalCid lcid, MacNodeId nodeId) override;
    // reset the structure for this LCID
    void clearLcid(LogicalCid lcid) override;
    // reset structures for all connections
    void clearAllLcid() override;

  public:
    void insertPdcpSdu(inet::Packet *pdcpPkt) override;
    void receivedPdcpSdu(inet::Packet *pdcpPkt) override { /*TODO*/ }
    void insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) override;
    void insertMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;
    void macPduArrived(const inet::Ptr<const LteMacPdu> macPdu) override;
    void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;
    void discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac = false) override;

    DiscardedPkts getDiscardedPkt();
    double getDelayStats();
    void resetDelayCounter();
    virtual void clearStats();
};

} //namespace

#endif

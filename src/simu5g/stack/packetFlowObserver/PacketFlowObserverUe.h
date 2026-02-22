//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_PacketFlowObserverUE_H_
#define _LTE_PacketFlowObserverUE_H_

#include "simu5g/common/LteCommon.h"
#include "PacketFlowObserverBase.h"

namespace simu5g {

using namespace omnetpp;

class LteRlcUmDataPdu;
class LtePdcpUe;

/**
 * The UE-specific version of Packet Flow Manager.
 */
class PacketFlowObserverUe : public PacketFlowObserverBase
{
    /*
     * The node can have different active connections (DRB ID) at the same time, hence we need to
     * maintain the status for each of them.
     */
    struct StatusDescriptor {
        MacNodeId nodeId_; // destination node of this DRB ID
        std::map<unsigned int, PdcpStatus> pdcpStatus_; // a PDCP PDU can be fragmented into many RLC that could be sent and acknowledged at different times (this prevents early removal on acknowledgment)
        std::map<unsigned int, SequenceNumberSet> rlcPdusPerSdu_;  // for each RLC SDU, stores the RLC PDUs where the former was fragmented
        std::map<unsigned int, SequenceNumberSet> rlcSdusPerPdu_;  // for each RLC PDU, stores the included RLC SDUs
        std::map<unsigned int, SequenceNumberSet> macSdusPerPdu_;  // for each MAC PDU, stores the included MAC SDUs (should be a 1:1 association)
        std::vector<unsigned int> macPduPerProcess_;               // for each HARQ process, stores the included MAC PDU
    };

    typedef std::map<DrbId, StatusDescriptor> ConnectionMap;
    ConnectionMap connectionMap_; // DRB ID to the corresponding StatusDescriptor

    Delay pdcpDelay;

  protected:
    void initialize(int stage) override;

    void initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int sduHeaderSize, simtime_t& arrivalTime);
    // return true if a structure for this DRB ID is present
    bool hasDrbId(DrbId drbId) override;
    // initialize a new structure for this DRB ID
    void initDrbId(DrbId drbId, MacNodeId nodeId) override;
    // reset the structure for this DRB ID
    void clearDrbId(DrbId drbId) override;
    // reset structures for all connections
    void clearAllDrbIds() override;

  public:
    void insertPdcpSdu(inet::Packet *pdcpPkt) override;
    void receivedPdcpSdu(inet::Packet *pdcpPkt) override { /*TODO*/ }
    void insertRlcPdu(DrbId drbId, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) override;
    void insertMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;
    void macPduArrived(const inet::Ptr<const LteMacPdu> macPdu) override;
    void discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu) override;
    void discardRlcPdu(DrbId drbId, unsigned int rlcSno, bool fromMac = false) override;

    DiscardedPkts getDiscardedPkt();
    double getDelayStats();
    void resetDelayCounter();
    virtual void clearStats();
};

} //namespace

#endif

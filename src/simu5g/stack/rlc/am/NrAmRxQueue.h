//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __SIMU5G_NRAMRXQUEUE_H_
#define __SIMU5G_NRAMRXQUEUE_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/Packet.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/rlc/am/RlcSduSlidingWindowReceptionBuffer.h"

namespace simu5g {

class NrRlcAm;
class Binder;

/**
 * @class NrAmRxQueue
 * @brief NR RLC AM Reception Queue (3GPP TS 38.322).
 *
 * Handles PDU reception, SDU reassembly, and status report generation.
 * Created dynamically by NrRlcAm per logical channel.
 */
class NrAmRxQueue : public omnetpp::cSimpleModule
{
  public:
    ~NrAmRxQueue() override;

    void enque(inet::Packet *pkt);
    void handleControlPdu(inet::Packet *pkt);
    void setRemoteEntity(MacNodeId id);

  protected:
    void initialize() override;
    void handleMessage(omnetpp::cMessage *msg) override;

    void passUp(int seqNum);
    void sendStatusReport();

    // Parent RLC AM module
    inet::ModuleRefByPar<NrRlcAm> lteRlc_;
    inet::ModuleRefByPar<Binder> binder_;

    // FlowControlInfo used for building status reports
    FlowControlInfo *flowControlInfo_ = nullptr;

    // Debug identifier
    std::string nameEntity_;

    // Statistics
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalRcvdBytes_ = 0;
    Direction dir_ = UNKNOWN_DIRECTION;

    static omnetpp::simsignal_t rlcCellPacketLossSignal_[2];
    static omnetpp::simsignal_t rlcPacketLossSignal_[2];
    static omnetpp::simsignal_t rlcPduPacketLossSignal_[2];
    static omnetpp::simsignal_t rlcDelaySignal_[2];
    static omnetpp::simsignal_t rlcPduDelaySignal_[2];
    static omnetpp::simsignal_t rlcCellThroughputSignal_[2];
    static omnetpp::simsignal_t rlcThroughputSignal_[2];
    static omnetpp::simsignal_t rlcPduThroughputSignal_[2];
    static omnetpp::simsignal_t rlcThroughputSampleSignal_[2];
    static omnetpp::simsignal_t rlcCellThroughputSampleSignal_[2];
    static omnetpp::simsignal_t rxWindowOccupationSignal_;
    static omnetpp::simsignal_t rxWindowFullSignal_;

    omnetpp::cModule *remoteEntity_ = nullptr;
    omnetpp::simtime_t lastTputSample_;
    unsigned int tpSample_ = 0;
    omnetpp::simtime_t lastCellTputSample_;
    unsigned int cellTpSample_ = 0;

    // Reassembly buffer
    RlcSduSlidingWindowReceptionBuffer *rxBuffer_ = nullptr;
    std::set<unsigned int> passedUpSdus_;

    // Timers
    omnetpp::cMessage *tReassemblyTimer_ = nullptr;
    omnetpp::simtime_t tReassembly_;
    omnetpp::cMessage *tStatusProhibitTimer_ = nullptr;
    omnetpp::simtime_t tStatusProhibit_;
    omnetpp::simtime_t lastSentAck_;
    omnetpp::cMessage *throughputTimer_ = nullptr;
    omnetpp::simtime_t throughputInterval_;

    // RX state
    unsigned int rxNextStatusTrigger_ = 0;
    unsigned int amWindowSize_ = 0;
    bool statusReportPending_ = false;
};

} //namespace

#endif

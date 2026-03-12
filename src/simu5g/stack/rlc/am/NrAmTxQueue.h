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

#ifndef __SIMU5G_NRAMTXQUEUE_H_
#define __SIMU5G_NRAMTXQUEUE_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/common/packet/Packet.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/rlc/am/RlcSduSlidingWindowTransmissionBuffer.h"
#include "simu5g/stack/rlc/am/RlcSduRetransmissionBuffer.h"

namespace simu5g {

class LteRlcAm;
class LteMacBase;

/**
 * @class NrAmTxQueue
 * @brief NR RLC AM Transmission Queue (3GPP TS 38.322).
 *
 * Manages SDU segmentation, ARQ retransmissions and polling.
 * Created dynamically by NrRlcAm per logical channel.
 */
class NrAmTxQueue : public omnetpp::cSimpleModule
{
  protected:
    struct SduInfo {
        inet::Packet *sdu = nullptr;
        int currentOffset = 0;
        ~SduInfo() { delete sdu; }
    };

    // SDU and PDU buffers
    std::list<SduInfo *> sduBuffer_;
    RlcSduSlidingWindowTransmissionBuffer *txBuffer_ = nullptr;
    RlcSduRetransmissionBuffer *rtxBuffer_ = nullptr;
    std::list<omnetpp::cPacket *> controlBuffer_;

    // Set on Radio Link Failure to stop transmission
    bool radioLinkFailureDetected_ = false;

    // Reference to the parent RLC AM module
    inet::ModuleRefByPar<LteRlcAm> lteRlc_;

    // Flow control info (one per logical channel)
    FlowControlInfo *lteInfo_ = nullptr;
    MacCid infoCid_;

    // MAC reference for buffer status reporting
    LteMacBase *mac_ = nullptr;

    // Debug identifier
    std::string nameEntity_;

    // TX state variables
    unsigned int sn_ = 0;
    unsigned int txNextAck_ = 0;
    unsigned int amWindowSize_ = 0;

    // Polling state
    unsigned int pduWithoutPoll_ = 0;
    unsigned int byteWithoutPoll_ = 0;
    unsigned int pollPdu_ = 0;
    unsigned int pollByte_ = 0;
    unsigned int pollSn_ = 0;
    unsigned int maxRtxThreshold_ = 0;
    bool pollPending_ = false;
    omnetpp::cMessage *tPollRetransmitTimer_ = nullptr;
    omnetpp::simtime_t tPollRetransmit_;

    // Statistics
    static omnetpp::simsignal_t wastedGrantedBytesSignal_;
    static omnetpp::simsignal_t enqueuedSduSizeSignal_;
    static omnetpp::simsignal_t enqueuedSduRateSignal_;
    static omnetpp::simsignal_t requestedPduSizeSignal_;
    static omnetpp::simsignal_t txWindowOccupationSignal_;
    static omnetpp::simsignal_t txWindowFullSignal_;
    static omnetpp::simsignal_t retransmissionPduSignal_;
    omnetpp::simtime_t lastSduSample_;
    unsigned int sduSampleBytes_ = 0;
    unsigned int receivedSdus_ = 0;

    void initialize() override;
    void finish() override;
    void handleMessage(omnetpp::cMessage *msg) override;

    bool sendRetransmission(int size);
    void reportBufferStatus();
    bool checkPolling();
    void sendSegment(PendingSegment segment);

  public:
    ~NrAmTxQueue() override;

    void enque(inet::Packet *sdu);
    void sendPdus(int size);
    void handleControlPacket(omnetpp::cPacket *pkt);
    void bufferControlPdu(omnetpp::cPacket *pkt);
    unsigned int getPendingDataVolume() const;
};

} //namespace

#endif

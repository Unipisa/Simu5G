//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad PolitÃ©cnica de Cartagena), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _SIMU5G_LTERLCAMTXENTITY_H_
#define _SIMU5G_LTERLCAMTXENTITY_H_

#include <list>
#include <map>

#include <inet/common/packet/Packet.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/rlc/am/RlcAmTxEntityBase.h"
#include "simu5g/stack/rlc/am/RlcRetransmissionBuffer.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

/**
 * @brief LTE (TS 36.322) RLC AM transmission entity.
 *
 * Builds one AMD PDU per MAC grant by concatenating queued SDUs and SDU
 * fragments (FI framing, as in the LTE UM entity), retains the built PDUs in
 * the transmission window as the unit of ARQ, and retransmits on STATUS NACKs,
 * re-segmenting a retained PDU into AMD PDU segments (byte ranges) when the
 * grant is smaller than the PDU. Polling, retransmission counting and radio
 * link failure follow TS 36.322 5.2.2 / 5.2.1.
 *
 * The ARQ skeleton deliberately mirrors NrRlcAmTxEntity (TS 38.322): the two
 * specs share it, only the framing differs. Fixes to the poll/retransmission
 * logic in one entity almost certainly apply to the other -- keep them in sync.
 */
class LteRlcAmTxEntity : public RlcAmTxEntityBase
{
    // configuration
    unsigned int amWindowSize_ = 512;   // TS 36.322: 512 (10-bit SN space)
    unsigned int pollPdu_ = 0;
    unsigned int pollByte_ = 0;
    simtime_t tPollRetransmit_;

    // SDUs not yet (fully) put into a PDU; frontOffset_ bytes of the front SDU
    // are already carried by previously built PDUs (TS 36.322 concatenation).
    std::list<inet::Packet *> sduQueue_;
    unsigned int frontOffset_ = 0;
    unsigned int sduQueueBytes_ = 0;    // pending payload bytes, incl. the partial front SDU

    // Built AMD PDUs retained until acknowledged: the ARQ unit. VT(A) =
    // txNextAck_, VT(S) = txNext_ (TS 36.322 5.1.3.1). An entry is erased when
    // its SN is acknowledged, so map presence == not yet acknowledged.
    struct TxPdu {
        inet::Packet *pdu = nullptr;    // the built LteRlcAmDataPdu packet
        uint32_t payloadLength = 0;     // data-field length in bytes

        friend std::ostream& operator<<(std::ostream& os, const TxPdu& e) {
            os << "pdu=" << (e.pdu ? e.pdu->getName() : "null") << " len=" << e.payloadLength;
            return os;
        }
    };
    std::map<uint32_t, TxPdu> txWindow_;
    uint32_t txNext_ = 0;
    uint32_t txNextAck_ = 0;

    RlcRetransmissionBuffer *rtxBuffer_ = nullptr;
    std::list<omnetpp::cPacket *> controlBuffer_;

    // polling state (TS 36.322 5.2.2)
    unsigned int pduWithoutPoll_ = 0;
    unsigned int byteWithoutPoll_ = 0;
    uint32_t pollSn_ = 0;
    bool pollPending_ = false;
    omnetpp::cMessage *tPollRetransmitTimer_ = nullptr;

    // enqueued-SDU rate sampling (statistics)
    simtime_t lastSduSample_;
    unsigned int sduSampleBytes_ = 0;

  public:
    ~LteRlcAmTxEntity() override;

    void enque(inet::Packet *sdu) override;
    void sendPdus(int size) override;
    void bufferControlPdu(cPacket *pkt) override;
    void handleControlPacket(cPacket *pkt) override;
    unsigned int getPendingDataVolume() const override;

  protected:
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

  private:
    virtual void processControlPacket(inet::Packet *pkt);
    virtual void bufferControlPduInternal(inet::Packet *pkt);
    virtual bool sendRetransmission(int pduSize);
    virtual void buildAndSendPdu(int pduSize);
    virtual bool checkPolling(uint32_t sn);
    virtual void sendEmptyPdu();
    virtual void sendPduToMac(inet::Packet *pkt);
    virtual void sendNewDataNotification(inet::Packet *pkt);
    virtual void reportBufferStatus();

    bool windowFull() const { return txNext_ - txNextAck_ >= amWindowSize_; }
};

} //namespace

#endif

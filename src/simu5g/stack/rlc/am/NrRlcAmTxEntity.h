//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad Politecnica de Cartagena), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _SIMU5G_NRRLCAMTXENTITY_H_
#define _SIMU5G_NRRLCAMTXENTITY_H_

#include <list>
#include <string>

#include <inet/common/packet/Packet.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/rlc/am/RlcAmTxEntityBase.h"
#include "simu5g/stack/rlc/am/RlcRetransmissionBuffer.h"
#include "simu5g/stack/rlc/am/RlcSduSlidingWindowTransmissionBuffer.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

/**
 * @brief NR (TS 38.322) RLC AM transmission entity.
 *
 * Byte-offset (SO) segmentation with re-segmentation on retransmission via a
 * task queue, pollByte/pollPDU-driven status polling, emits NrRlcAmDataPdu.
 */
class NrRlcAmTxEntity : public RlcAmTxEntityBase
{
  protected:
    // Protected rather than private so a profile subclass can re-derive it after this class has
    // read its NED default -- see ~NtnNrRlcAmTxEntity, which replaces it with a value computed
    // from the satellite round-trip delay. Nothing else about the timer changes.
    omnetpp::simtime_t tPollRetransmit_;

  private:
    unsigned int snFieldLength_ = 12;  // NR-SO AM SN bits, derived from AM_Window_Size (12 or 18)

    struct SduInfo {
        inet::Packet *sdu = nullptr;
        int currentOffset = 0;
        ~SduInfo() { delete sdu; }
    };
    std::list<SduInfo *> sduBuffer_;
    RlcSduSlidingWindowTransmissionBuffer *txBuffer_ = nullptr;
    RlcRetransmissionBuffer *rtxBuffer_ = nullptr;
    std::list<omnetpp::cPacket *> controlBuffer_;
    std::string nameEntity_;
    unsigned int sn_ = 0;
    unsigned int amWindowSize_ = 0;
    unsigned int pduWithoutPoll_ = 0;
    unsigned int byteWithoutPoll_ = 0;
    unsigned int pollPdu_ = 0;
    unsigned int pollByte_ = 0;
    unsigned int pollSn_ = 0;
    bool pollPending_ = false;
    omnetpp::cMessage *tPollRetransmitTimer_ = nullptr;
    omnetpp::simtime_t lastSduSample_;
    unsigned int sduSampleBytes_ = 0;
    unsigned int receivedSdus_ = 0;

  public:
    ~NrRlcAmTxEntity() override;

    void enque(Packet *sdu) override;
    void sendPdus(int size) override;
    void bufferControlPdu(cPacket *pkt) override;
    void handleControlPacket(cPacket *pkt) override;

    // gate-delivery variants (no Enter_Method/take: packet already owned via handleMessage)
    void processControlPacket(inet::Packet *pkt);
    void bufferControlPduInternal(inet::Packet *pkt);
    unsigned int getPendingDataVolume() const override;

  protected:

    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

  private:

    bool sendRetransmission(int pduSize);
    void reportBufferStatus();
    bool checkPolling();
    void sendSegment(PendingSegment segment);
    void sendPduToMac(inet::Packet *pkt);
    void sendNewDataNotificationNr(inet::Packet *pkt);
};

} //namespace

#endif

//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _SIMU5G_LTERLCUMRXENTITY_H_
#define _SIMU5G_LTERLCUMRXENTITY_H_


#include "simu5g/stack/rlc/um/RlcUmRxEntityBase.h"
#include "simu5g/common/timer/TTimer.h"

namespace simu5g {

using namespace omnetpp;

class LteMacBase;
class LteRlcUmDataPdu;

/**
 * @brief LTE (TS 36.322) RLC UM receiving entity.
 *
 * FI-walk reassembly across concatenated PDUs, PDU-SN reordering window with a
 * t-Reordering timer. Delivers in-sequence SDUs to PDCP.
 */
class LteRlcUmRxEntity : public RlcUmRxEntityBase
{
  protected:
    cArray pduBuffer_;
    RlcUmRxWindowDesc rxWindowDesc_;
    TTimer t_reordering_;
    double timeout_;
    std::vector<bool> received_;
    struct Buffered {
        inet::Packet *pkt = nullptr;
        size_t size = 0;
        unsigned int currentPduSno = 0;

        friend std::ostream& operator<<(std::ostream& os, const Buffered& e) {
            os << "pkt=" << (e.pkt ? e.pkt->getName() : "null") << " size=" << e.size
               << " currentPduSno=" << e.currentPduSno;
            return os;
        }
    } buffered_;
    unsigned int lastPduReassembled_ = 0;
    bool init_ = false;
    unsigned int totalPduRcvdBytes_ = 0;
    unsigned int totalRcvdBytes_ = 0;
    Direction dir_ = UNKNOWN_DIRECTION;

  public:
    LteRlcUmRxEntity();
    ~LteRlcUmRxEntity() override;

    void enque(cPacket *pkt) override;
    bool isEmpty() const override;

    void handleMessage(cMessage *msg) override;

  protected:
    void initMode(LteMacBase *mac) override;

    /*
     * Hook invoked when the first PDU of the connection is enqueued, to
     * initialize the reordering window. The base implementation applies the
     * D2D_MULTI special case (single-PDU window; the first received PDU is
     * treated as the first valid one).
     */
    virtual void onFirstPduEnqueued(unsigned int pduSno);

    /*
     * Hook invoked at the start of reassembling a PDU. If a reassembly reset
     * is pending (e.g. after a D2D mode switch), it is consumed so that the PDU
     * and its first extracted SDU are treated as in-sequence, and true is
     * returned; false is returned otherwise.
     */
    virtual bool consumeReassemblyReset(unsigned int pduSno);

    /*
     * Hook that emits the direction-indexed per-PDU statistics (throughput and
     * delay) at PDU enqueue time.
     */
    virtual void emitPduStats(Direction dir, double tputSample, simtime_t creationTime);

    /*
     * Hook that emits the direction-indexed per-SDU statistics (throughput and
     * delay) when an SDU is delivered to PDCP.
     */
    virtual void emitSduStats(Direction dir, double tputSample, simtime_t creationTime);

    virtual void moveRxWindow(int pos);
    virtual void reassemble(unsigned int index);
    virtual void toPdcpLte(inet::Packet *rlcSdu);
    virtual void clearBufferedSdu();
};

} //namespace

#endif

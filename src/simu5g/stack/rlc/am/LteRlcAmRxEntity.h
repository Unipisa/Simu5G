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

#ifndef _SIMU5G_LTERLCAMRXENTITY_H_
#define _SIMU5G_LTERLCAMRXENTITY_H_

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/rlc/am/RlcAmRxEntityBase.h"
#include "simu5g/stack/rlc/am/RlcSduSlidingWindowReceptionBuffer.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @brief LTE (TS 36.322) RLC AM receiving entity.
 *
 * PDU-SN receiving window with per-PDU byte-interval coverage (for AMD PDU
 * segments), t-Reordering + t-StatusProhibit, ACK_SN + NACK-list STATUS
 * reports, and in-order FI-walk reassembly of the concatenated SDU stream.
 *
 * The window/status machinery deliberately mirrors NrRlcAmRxEntity (TS 38.322,
 * where the analogous timer is t-Reassembly): the two specs share it, only the
 * framing differs -- keep the two implementations in sync. Unlike NR (one SDU
 * per PDU, delivered as each completes), the FI walk needs the PDU stream in
 * sequence, since an SDU may span adjacent PDUs; delivery is therefore strictly
 * in SN order.
 */
class LteRlcAmRxEntity : public RlcAmRxEntityBase
{
    // The receiving window; the unit the SN denotes is the AMD PDU
    // (TS 36.322), not the SDU as in the NR entity.
    RlcSduSlidingWindowReceptionBuffer *rxBuffer_ = nullptr;
    unsigned int amWindowSize_ = 512;    // TS 36.322: 512 (10-bit SN space)
    omnetpp::cMessage *tReorderingTimer_ = nullptr;
    omnetpp::simtime_t tReordering_;
    omnetpp::cMessage *tStatusProhibitTimer_ = nullptr;
    omnetpp::simtime_t tStatusProhibit_;
    unsigned int rxNextStatusTrigger_ = 0;
    bool statusReportPending_ = false;

    // An SDU cut across a PDU boundary: the whole-SDU dup carried by the last
    // fragment seen, and how many of its bytes have arrived so far.
    struct PendingSdu {
        inet::Packet *pkt = nullptr;
        size_t accumulated = 0;

        friend std::ostream& operator<<(std::ostream& os, const PendingSdu& e) {
            os << "pkt=" << (e.pkt ? e.pkt->getName() : "null") << " accumulated=" << e.accumulated;
            return os;
        }
    } pendingSdu_;

  public:
    ~LteRlcAmRxEntity() override;

    void enque(inet::Packet *pkt) override;
    void handleMessage(cMessage *msg) override;

  protected:
    void initMode() override;

  private:
    virtual void passUpPdu(uint32_t sn);
    virtual void deliverSdu(inet::Packet *sdu);
    virtual void sendStatusReport();
};

} //namespace

#endif

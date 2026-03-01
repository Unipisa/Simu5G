#ifndef _TM_TX_ENTITY_H_
#define _TM_TX_ENTITY_H_

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/rlc/RlcTxEntityBase.h"

namespace simu5g {

/**
 * @class TmTxEntity
 * @brief Transmission entity for Transparent Mode (TM).
 *
 * Buffers SDUs from the upper layer without segmentation.
 * Sends queued PDUs to MAC upon request.
 */
class TmTxEntity : public RlcTxEntityBase
{
  protected:
    inet::cPacketQueue queuedPdus_;
    int queueSize_ = 0;   // max queue length in packets (0: unlimited)

    static inet::simsignal_t rlcPacketLossDlSignal_;
    static inet::simsignal_t rlcPacketLossUlSignal_;

    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;

    void handleSdu(inet::Packet *pkt);
    void handleMacSduRequest(inet::Packet *pkt);
};

} // namespace simu5g

#endif

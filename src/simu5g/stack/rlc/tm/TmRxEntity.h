#ifndef _TM_RX_ENTITY_H_
#define _TM_RX_ENTITY_H_

#include <inet/common/packet/Packet.h>

#include "simu5g/stack/rlc/RlcRxEntityBase.h"

namespace simu5g {

/**
 * @class TmRxEntity
 * @brief Receiver entity for Transparent Mode (TM).
 *
 * Simply forwards received PDUs to the upper layer without
 * reassembly or reordering.
 */
class TmRxEntity : public RlcRxEntityBase
{
  protected:
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;
};

} // namespace simu5g

#endif

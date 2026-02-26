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

#ifndef _BYPASS_TX_PDCP_ENTITY_H_
#define _BYPASS_TX_PDCP_ENTITY_H_

#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"

namespace simu5g {

class LtePdcp;

/**
 * @class BypassTxPdcpEntity
 * @brief Bypass TX PDCP entity for DC secondary nodes.
 *
 * Receives already-processed PDCP PDUs (from master via X2) and forwards
 * them directly to RLC without any PDCP processing (no compression,
 * no PDCP header, no sequence numbering).
 */
class BypassTxPdcpEntity : public PdcpTxEntityBase
{
    static omnetpp::simsignal_t sentPacketToLowerLayerSignal_;
    static omnetpp::simsignal_t pdcpSduSentSignal_;

  protected:
    LtePdcp *pdcp_ = nullptr;

  public:
    void initialize(int stage) override;
    void handlePacketFromUpperLayer(inet::Packet *pkt) override;
};

} // namespace simu5g

#endif

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

#ifndef _BYPASS_RX_PDCP_ENTITY_H_
#define _BYPASS_RX_PDCP_ENTITY_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

class LtePdcp;

/**
 * @class BypassRxPdcpEntity
 * @brief Bypass RX PDCP entity for DC secondary nodes.
 *
 * Receives PDCP PDUs from RLC (UL from UE) and forwards them
 * directly to the master node via X2 without any PDCP processing
 * (no decompression, no PDCP header removal, no reordering).
 */
class BypassRxPdcpEntity : public PdcpRxEntityBase
{
  protected:
    LtePdcp *pdcp_ = nullptr;
    inet::ModuleRefByPar<Binder> binder_;
    MacNodeId nodeId_ = NODEID_NONE;

  public:
    void initialize(int stage) override;
    void handlePacketFromLowerLayer(inet::Packet *pkt) override;
};

} // namespace simu5g

#endif

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

#include "simu5g/stack/pdcp/BypassTxPdcpEntity.h"
#include "simu5g/stack/pdcp/LtePdcp.h"

namespace simu5g {

Define_Module(BypassTxPdcpEntity);

void BypassTxPdcpEntity::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        pdcp_ = check_and_cast<LtePdcp *>(getParentModule());
    }
}

void BypassTxPdcpEntity::handlePacketFromUpperLayer(inet::Packet *pkt)
{
    // DC bypass: packet is already a PDCP PDU from the master node.
    // Forward directly to RLC without any PDCP processing.
    EV << NOW << " BypassTxPdcpEntity::handlePacketFromUpperLayer - forwarding packet " << pkt->getName() << " to RLC (DC bypass)" << endl;
    pdcp_->sendToRlc(pkt);
}

} // namespace simu5g

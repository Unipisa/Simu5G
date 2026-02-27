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

#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"

#include <inet/common/packet/Packet.h>

namespace simu5g {

using namespace omnetpp;

void PdcpRxEntityBase::handleMessage(cMessage *msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    handlePacketFromLowerLayer(pkt);
}

} // namespace simu5g

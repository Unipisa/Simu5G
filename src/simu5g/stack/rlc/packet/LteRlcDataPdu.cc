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

#include "simu5g/stack/rlc/packet/LteRlcDataPdu.h"

namespace simu5g {

Register_Class(LteRlcDataPdu);
Register_Class(LteRlcUmDataPdu);
Register_Class(LteRlcAmDataPdu);

void LteRlcDataPdu::pushSdu(inet::Packet *pkt)
{
    pushSdu(pkt, pkt->getByteLength());
}

void LteRlcDataPdu::pushSdu(inet::Packet *pkt, int size)
{
    appendSdu(pkt);
    appendSduSize(size);
    rlcPduLength += size;
}

inet::Packet *LteRlcDataPdu::popSdu(size_t& size)
{
    auto pkt = removeSdu(0);
    eraseSdu(0);

    size = getSduSize(0);
    eraseSduSize(0);
    rlcPduLength -= size;
    return pkt;
}

} //namespace


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

#ifndef _IPDCP_GATEWAY_H_
#define _IPDCP_GATEWAY_H_

namespace inet { class Packet; }

namespace simu5g {

/**
 * @class IPdcpGateway
 * @brief Minimal interface for PDCP entities to send packets through the
 *        PDCP module's output gates.
 *
 * PDCP entities (TX and RX, both normal and bypass) use this interface
 * to forward processed packets to the appropriate next layer (RLC, NR RLC,
 * upper layer, or X2/DC manager) without depending on the concrete PDCP
 * module class.
 */
class IPdcpGateway
{
  public:
    virtual ~IPdcpGateway() = default;

    virtual void sendToUpperLayer(inet::Packet *pkt) = 0;
    virtual void sendToRlc(inet::Packet *pkt) = 0;
    virtual void sendToNrRlc(inet::Packet *pkt) = 0;
    virtual void sendToX2(inet::Packet *pkt) = 0;
};

} // namespace simu5g

#endif

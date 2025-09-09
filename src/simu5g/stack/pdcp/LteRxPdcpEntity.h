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

#ifndef _LTE_LTERXPDCPENTITY_H_
#define _LTE_LTERXPDCPENTITY_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/LtePdcp.h"

namespace simu5g {

class LtePdcpBase;
class LtePdcpHeader;

using namespace inet;

/**
 * @class LteRxPdcpEntity
 * @brief Entity for PDCP Layer
 *
 * This is the PDCP RX entity of the LTE Stack.
 */
class LteRxPdcpEntity : public cSimpleModule
{
  protected:
    // reference to the PDCP layer
    LtePdcpBase *pdcp_ = nullptr;

    // whether headers are compressed
    bool headerCompressionEnabled_;

    // Logical CID for this connection
    LogicalCid lcid_;

    // handler for PDCP SDU
    virtual void handlePdcpSdu(Packet *pkt);

    virtual void decompressHeader(inet::Packet *pkt);

    bool isCompressionEnabled() { return headerCompressionEnabled_; }

  public:


    void initialize() override;

    // obtain the IP datagram from the PDCP PDU
    void handlePacketFromLowerLayer(Packet *pkt);

    /*
     * @author Alessandro Noferi
     *
     * This method is used with NrRxPdcpEntity that has
     * an SDU buffer. In particular, it is used when the
     * RNI service requests the number of active users
     * in UL, that also counts buffered UL data in PDCP.
     */
    virtual bool isEmpty() const { return true; }
};

} //namespace

#endif


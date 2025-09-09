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

#ifndef _LTE_LTETXPDCPENTITY_H_
#define _LTE_LTETXPDCPENTITY_H_

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/pdcp/LtePdcp.h"

namespace simu5g {

class LtePdcpBase;
class LtePdcpHeader;

using namespace inet;


#define LTE_PDCP_HEADER_COMPRESSION_DISABLED    B(-1)

/**
 * @class LteTxPdcpEntity
 * @brief Entity for PDCP Layer
 *
 * This is the PDCP entity of the LTE Stack.
 *
 * The PDCP entity performs the following tasks:
 * - maintain numbering of one logical connection
 *
 */
class LteTxPdcpEntity : public cSimpleModule
{
  protected:
    // reference to the PDCP layer
    LtePdcpBase *pdcp_ = nullptr;

    // Header size after ROHC (RObust Header Compression)
    inet::B headerCompressedSize_;

    // next sequence number to be assigned
    unsigned int sno_ = 0;

    // deliver the PDCP PDU to the lower layer
    virtual void deliverPdcpPdu(Packet *pdcpPkt);

    virtual void compressHeader(inet::Packet *pkt);

    bool isCompressionEnabled() { return headerCompressedSize_ != LTE_PDCP_HEADER_COMPRESSION_DISABLED; }


  public:


    void initialize() override;

    // create a PDCP PDU from the IP datagram
    void handlePacketFromUpperLayer(Packet *pkt);
};

} //namespace

#endif


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

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"

namespace simu5g {

class LtePdcpRrcBase;
class LtePdcpPdu;

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
    LtePdcpRrcBase *pdcp_ = nullptr;

    // Logical CID for this connection
    LogicalCid lcid_;

    // handler for PDCP SDU
    virtual void handlePdcpSdu(Packet *pkt);

  public:


    void initialize() override;

    // obtain the IP datagram from the PDCP PDU
    void handlePacketFromLowerLayer(Packet *pkt);

    /*
     * @author Alessandro Noferi
     *
     * This method is used with NRRxPdcpEntity that has
     * an SDU buffer. In particular, it is used when the
     * RNI service requests the number of active users
     * in UL, that also counts buffered UL data in PDCP.
     */
    virtual bool isEmpty() const { return true; }
};

} //namespace

#endif


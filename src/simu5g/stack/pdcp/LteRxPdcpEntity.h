//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERXPDCPENTITY_H_
#define _LTE_LTERXPDCPENTITY_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/pdcp/PdcpRxEntityBase.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

class LtePdcpHeader;

using namespace inet;

/**
 * @class LteRxPdcpEntity
 * @brief Entity for PDCP Layer
 *
 * This is the PDCP RX entity of the LTE Stack.
 */
class LteRxPdcpEntity : public PdcpRxEntityBase
{
    static simsignal_t receivedPacketFromLowerLayerSignal_;
    static simsignal_t pdcpSduReceivedSignal_;
    static simsignal_t sentPacketToUpperLayerSignal_;

  protected:
    // Modules references
    inet::ModuleRefByPar<Binder> binder_;

    // Identifier for this node
    MacNodeId nodeId_;

    // whether headers are compressed
    bool headerCompressionEnabled_;

    // DRB ID for this connection
    DrbId drbId_;

    // handler for PDCP SDU
    virtual void handlePdcpSdu(Packet *pkt, unsigned int sequenceNumber);

    void deliverSduToUpperLayer(inet::Packet *pkt);

    virtual void decompressHeader(inet::Packet *pkt);

    bool isCompressionEnabled() { return headerCompressionEnabled_; }

  public:


    void initialize(int stage) override;

    // obtain the IP datagram from the PDCP PDU
    void handlePacketFromLowerLayer(Packet *pkt) override;

    /*
     * @author Alessandro Noferi
     *
     * This method is used with NrRxPdcpEntity that has
     * an SDU buffer. In particular, it is used when the
     * RNI service requests the number of active users
     * in UL, that also counts buffered UL data in PDCP.
     */
    bool isEmpty() const override { return true; }
};

} //namespace

#endif


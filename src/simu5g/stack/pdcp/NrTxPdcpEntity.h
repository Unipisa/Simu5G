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

#ifndef _NRTXPDCPENTITY_H_
#define _NRTXPDCPENTITY_H_

#include "simu5g/stack/pdcp/LteTxPdcpEntity.h"
#include "simu5g/stack/dcX2Forwarder/DcX2Forwarder.h"

namespace simu5g {

/**
 * @class NrTxPdcpEntity
 * @brief Entity for New Radio PDCP Layer
 *
 * This is the PDCP entity of LTE/NR Stack.
 *
 * PDCP entity performs the following tasks:
 * - maintain numbering of one logical connection
 *
 */
class NrTxPdcpEntity : public LteTxPdcpEntity
{
    static simsignal_t pdcpSduSentNrSignal_;

  protected:

    // Dual Connectivity support
    bool dualConnectivityEnabled_ = false;

    // NR node ID (for UEs with dual connectivity)
    MacNodeId nrNodeId_ = NODEID_NONE;

    // deliver the PDCP PDU to the lower layer or to the X2
    void deliverPdcpPdu(Packet *pkt) override;

  public:


    void initialize(int stage) override;
};

} //namespace

#endif

